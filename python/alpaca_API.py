import os
import mysql.connector
from datetime import datetime, timezone
import alpaca
from alpaca.data.timeframe import TimeFrame, TimeFrameUnit
from alpaca.data.historical import StockHistoricalDataClient, CryptoHistoricalDataClient,OptionHistoricalDataClient
from alpaca.data.requests import StockBarsRequest, CryptoBarsRequest,OptionBarsRequest
from alpaca.trading.client import TradingClient, GetAssetsRequest
from alpaca.trading.requests import GetOptionContractsRequest, LimitOrderRequest, MarketOrderRequest
from alpaca.trading.enums import AssetStatus, ContractType, OrderSide,OrderType,TimeInForce,QueryOrderStatus
import pytz

from pathlib import Path
from dotenv import load_dotenv
import os

env_path = Path(__file__).resolve().parent.parent / ".env"
load_dotenv(dotenv_path=env_path)
            

# Récupère les clés Alpaca
API_KEY = os.getenv("API_KEY")
API_SECRET = os.getenv("SECRET_KEY")

# Connexion MySQL
DB_CONFIG = {
    "host": os.getenv("MYSQL_HOST"),
    "user": os.getenv("MYSQL_USER"),
    "password": os.getenv("MYSQL_PASSWORD"),
    "database": os.getenv("MYSQL_DATABASE")
}


ALPACA_DATA_VENDOR_NAME = "Alpaca"
ALPACA_WEBSITE = "https://alpaca.markets"


# CONNECT TO MYSQL
conn = mysql.connector.connect(**DB_CONFIG)
cursor = conn.cursor(dictionary=True)

def get_or_create_data_vendor():
    cursor.execute("SELECT id FROM data_vendor WHERE name = %s", (ALPACA_DATA_VENDOR_NAME,))
    result = cursor.fetchone()
    if result:
        return result['id']
    now = datetime.now(timezone.utc)
    cursor.execute("""
        INSERT INTO data_vendor (name, website_url, created_date, last_updated_date)
        VALUES (%s, %s, %s, %s)
    """, (ALPACA_DATA_VENDOR_NAME, ALPACA_WEBSITE, now, now))
    conn.commit()
    return cursor.lastrowid

def get_or_create_symbol(symbol):
    cursor.execute("SELECT id FROM symbol WHERE ticker = %s", (symbol,))
    result = cursor.fetchone()
    if result:
        return result['id']
    now = datetime.now(timezone.utc)
    cursor.execute("""
        INSERT INTO symbol (ticker, instrument, created_date, last_updated_date)
        VALUES (%s, %s, %s, %s)
    """, (symbol, 'equity', now, now))
    conn.commit()
    return cursor.lastrowid

def insert_price_bar(data_vendor_id, symbol_id, interval, bars):
    now = datetime.now(timezone.utc)
    insert_sql = """
        INSERT INTO price_bar (
            data_vendor_id, symbol_id, timestamp, `interval`,
            open_price, high_price, low_price, close_price, volume,
            trade_count, vwap,
            created_date, last_updated_date
        )
        VALUES (%s, %s, %s, %s,
                %s, %s, %s, %s, %s,
                %s, %s,
                %s, %s)
        ON DUPLICATE KEY UPDATE
            last_updated_date = VALUES(last_updated_date),
            open_price = VALUES(open_price),
            high_price = VALUES(high_price),
            low_price = VALUES(low_price),
            close_price = VALUES(close_price),
            volume = VALUES(volume),
            trade_count = VALUES(trade_count),
            vwap = VALUES(vwap);
    """

    data = []
    for bar in bars:
        ts = bar.timestamp.replace(tzinfo=pytz.UTC).astimezone(pytz.UTC)
        data.append((
            data_vendor_id, symbol_id, ts, interval,
            bar.open, bar.high, bar.low, bar.close, bar.volume,
            getattr(bar, 'trade_count', None),
            getattr(bar, 'vwap', None),
            now, now
        ))

    cursor = conn.cursor()
    cursor.executemany(insert_sql, data)
    conn.commit()
    cursor.close()

def fetch_and_store(symbol, start_date, end_date, interval_str, asset_type='stock'):
    print(f"📥 Fetching {interval_str} {asset_type.upper()} data for {symbol}...")

    symbol_id = get_or_create_symbol(symbol)
    data_vendor_id = get_or_create_data_vendor()

    tf_map = {
        '1Min': TimeFrame.Minute,
        '5Min': TimeFrame(5, TimeFrameUnit.Minute),
        '15Min': TimeFrame(15, TimeFrameUnit.Minute),
        '1H': TimeFrame.Hour,
        '1Day': TimeFrame.Day,
    }

    if interval_str not in tf_map:
        raise ValueError(f"Interval invalide : {interval_str}")

    timeframe = tf_map[interval_str]

    if asset_type == 'stock':
        client = StockHistoricalDataClient(API_KEY, API_SECRET)
        request = StockBarsRequest(
            symbol_or_symbols=[symbol],
            timeframe=timeframe,
            start=start_date,
            end=end_date
        )
        bars = client.get_stock_bars(request).data.get(symbol, [])

    elif asset_type == 'crypto':
        client = CryptoHistoricalDataClient(API_KEY, API_SECRET)
        request = CryptoBarsRequest(
            symbol_or_symbols=[symbol],
            timeframe=timeframe,
            start=start_date,
            end=end_date
        )
        bars = client.get_crypto_bars(request).data.get(symbol, [])
    
        '''
    elif asset_type=='option':
        client = CryptoHistoricalDataClient(API_KEY, API_SECRET)
        request = OptionBarsRequest(
            symbol_or_symbols=[symbol],
            timeframe=timeframe,
            start=start_date,
            end=end_date
        )
        '''
    else:
        raise ValueError(f"Type d'actif non supporté : {asset_type}")

    if not bars:
        print(f"⚠️  Aucun bar trouvé pour {symbol} ({asset_type})")
    else:
        insert_price_bar(data_vendor_id, symbol_id, interval_str, bars)
        print(f"✅ {len(bars)} bars insérés pour {symbol} ({asset_type})")


if __name__ == "__main__":
    try:
        interval = '5Min'
        start_date = datetime(2023, 1, 1)
        end_date = datetime(2025, 1, 1)

        # Stocks
        for sym in ['AAPL']:
            
            fetch_and_store(sym, start_date, end_date, interval, asset_type='stock')

        # Crypto
        #for sym in ['BTC/USD', 'ETH/USD']:
           #fetch_and_store(sym, start_date, end_date, interval, asset_type='crypto')

    except Exception as e:
        print("❌ Une erreur est survenue :", e)

    finally:
        conn.close()
        print("🔌 Connexion MySQL fermée.")
