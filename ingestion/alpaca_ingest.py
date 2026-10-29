"""Download intraday bars from Alpaca and upsert them into the ``price_bar`` table.

    python ingestion/alpaca_ingest.py AAPL BTC/USD --interval 5Min --start 2024-01-01 --end 2024-03-01

Credentials are read from ``.env`` (see ``.env.example``). Rows are upserted on
(vendor, symbol, timestamp, interval), so reruns refresh data instead of duplicating it.
"""

from __future__ import annotations

import argparse
import logging
import os
from datetime import datetime, timezone
from pathlib import Path

import mysql.connector
from dotenv import load_dotenv

logger = logging.getLogger(__name__)

load_dotenv(Path(__file__).resolve().parent.parent / ".env")

VENDOR_NAME = "Alpaca"
VENDOR_URL = "https://alpaca.markets"
ETFS = {
    "SPY",
    "QQQ",
    "DIA",
    "VTI",
    "IWM",
    "EEM",
    "XLF",
    "XLY",
    "XLI",
    "VOO",
    "SCHX",
    "SPLG",
    "SPYG",
    "GLD",
    "SLV",
    "USO",
    "DBC",
}
INTERVALS = ("1Min", "5Min", "15Min", "1H", "1Day")

UPSERT_SQL = """
    INSERT INTO price_bar (data_vendor_id, symbol_id, timestamp, `interval`, open_price, high_price, low_price,
                           close_price, volume, trade_count, vwap, created_date, last_updated_date)
    VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
    ON DUPLICATE KEY UPDATE open_price = VALUES(open_price), high_price = VALUES(high_price),
        low_price = VALUES(low_price), close_price = VALUES(close_price), volume = VALUES(volume),
        trade_count = VALUES(trade_count), vwap = VALUES(vwap), last_updated_date = VALUES(last_updated_date)
"""


def db_config() -> dict:
    missing = [v for v in ("MYSQL_USER", "MYSQL_PASSWORD") if not os.getenv(v)]
    if missing:
        raise RuntimeError(f"Missing environment variables: {', '.join(missing)} (see .env.example)")
    # Accept the C++ connector format (tcp://host:port) as well as a plain host name.
    host, _, port = os.getenv("MYSQL_HOST", "localhost").removeprefix("tcp://").partition(":")
    return {
        "host": host,
        "port": int(port or 3306),
        "user": os.environ["MYSQL_USER"],
        "password": os.environ["MYSQL_PASSWORD"],
        "database": os.getenv("MYSQL_DATABASE", "securities_master"),
    }


def connect():
    return mysql.connector.connect(**db_config())


def asset_type(symbol: str) -> str:
    """'crypto' for pairs like BTC/USD, 'etf' for known ETFs, otherwise 'stock'."""
    if "/" in symbol:
        return "crypto"
    return "etf" if symbol in ETFS else "stock"


def _now() -> datetime:
    return datetime.now(timezone.utc).replace(tzinfo=None)


def get_or_create_vendor(conn) -> int:
    with conn.cursor() as cur:
        cur.execute("SELECT id FROM data_vendor WHERE name = %s", (VENDOR_NAME,))
        row = cur.fetchone()
        if row:
            return row[0]
        cur.execute(
            "INSERT INTO data_vendor (name, website_url, created_date, last_updated_date) VALUES (%s, %s, %s, %s)",
            (VENDOR_NAME, VENDOR_URL, _now(), _now()),
        )
        conn.commit()
        return cur.lastrowid


def get_or_create_symbol(conn, symbol: str) -> int:
    with conn.cursor() as cur:
        cur.execute("SELECT id FROM symbol WHERE ticker = %s", (symbol,))
        row = cur.fetchone()
        if row:
            return row[0]
        cur.execute(
            "INSERT INTO symbol (ticker, instrument, created_date, last_updated_date) VALUES (%s, %s, %s, %s)",
            (symbol, asset_type(symbol), _now(), _now()),
        )
        conn.commit()
        return cur.lastrowid


def fetch_bars(symbol: str, start: datetime, end: datetime, interval: str) -> list:
    """Query the Alpaca market data API (stocks and ETFs use the same endpoint)."""
    from alpaca.data.historical import CryptoHistoricalDataClient, StockHistoricalDataClient
    from alpaca.data.requests import CryptoBarsRequest, StockBarsRequest
    from alpaca.data.timeframe import TimeFrame, TimeFrameUnit

    timeframes = {
        "1Min": TimeFrame.Minute,
        "5Min": TimeFrame(5, TimeFrameUnit.Minute),
        "15Min": TimeFrame(15, TimeFrameUnit.Minute),
        "1H": TimeFrame.Hour,
        "1Day": TimeFrame.Day,
    }
    if interval not in timeframes:
        raise ValueError(f"Invalid interval {interval!r}, expected one of {INTERVALS}")

    # older .env files used API_KEY / SECRET_KEY
    key = os.getenv("ALPACA_API_KEY") or os.getenv("API_KEY")
    secret = os.getenv("ALPACA_SECRET_KEY") or os.getenv("SECRET_KEY")
    params = {"symbol_or_symbols": [symbol], "timeframe": timeframes[interval], "start": start, "end": end}
    if asset_type(symbol) == "crypto":
        response = CryptoHistoricalDataClient(key, secret).get_crypto_bars(CryptoBarsRequest(**params))
    else:
        response = StockHistoricalDataClient(key, secret).get_stock_bars(StockBarsRequest(**params))
    return response.data.get(symbol, [])


def store_bars(conn, symbol: str, interval: str, bars: list) -> int:
    vendor_id = get_or_create_vendor(conn)
    symbol_id = get_or_create_symbol(conn, symbol)
    now = _now()
    rows = [
        (
            vendor_id,
            symbol_id,
            b.timestamp.astimezone(timezone.utc).replace(tzinfo=None),
            interval,
            b.open,
            b.high,
            b.low,
            b.close,
            b.volume,
            getattr(b, "trade_count", None),
            getattr(b, "vwap", None),
            now,
            now,
        )
        for b in bars
    ]
    with conn.cursor() as cur:
        cur.executemany(UPSERT_SQL, rows)
    conn.commit()
    return len(rows)


def ingest(symbols: list[str], start: datetime, end: datetime, interval: str) -> None:
    conn = connect()
    try:
        for symbol in symbols:
            bars = fetch_bars(symbol, start, end, interval)
            if not bars:
                logger.warning("%s: no bars returned", symbol)
                continue
            logger.info("%s: %d %s bars stored", symbol, store_bars(conn, symbol, interval, bars), interval)
    finally:
        conn.close()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("symbols", nargs="+")
    parser.add_argument("--interval", default="5Min", choices=INTERVALS)
    parser.add_argument("--start", required=True, type=datetime.fromisoformat)
    parser.add_argument("--end", required=True, type=datetime.fromisoformat)
    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO, format="%(levelname)s %(message)s")
    ingest(args.symbols, args.start, args.end, args.interval)


if __name__ == "__main__":
    main()
