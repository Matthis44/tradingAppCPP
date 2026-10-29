"""Streamlit dashboard to load Alpaca data and inspect the securities master.

streamlit run ingestion/dashboard.py
"""

from __future__ import annotations

from datetime import date, datetime

import pandas as pd
import plotly.express as px
import streamlit as st

from alpaca_ingest import INTERVALS, connect, ingest

st.set_page_config(page_title="Securities master", layout="wide")
st.title("Securities master")


@st.cache_data(ttl=60)
def load_symbols() -> pd.DataFrame:
    conn = connect()
    try:
        return pd.read_sql("SELECT id, ticker, instrument FROM symbol ORDER BY ticker", conn)
    finally:
        conn.close()


@st.cache_data(ttl=60)
def load_bars(symbol_id: int, interval: str) -> pd.DataFrame:
    conn = connect()
    try:
        return pd.read_sql(
            """SELECT timestamp, open_price, high_price, low_price, close_price, volume, vwap
               FROM price_bar WHERE symbol_id = %s AND `interval` = %s ORDER BY timestamp""",
            conn,
            params=(int(symbol_id), interval),
        )
    finally:
        conn.close()


# --- Ingestion ------------------------------------------------------------------
st.header("Load data from Alpaca")
with st.form("ingest"):
    col1, col2 = st.columns(2)
    tickers = col1.text_input("Tickers (comma separated)", "AAPL, BTC/USD")
    interval = col1.selectbox("Interval", INTERVALS, index=1)
    start = col2.date_input("Start", date(2024, 1, 1))
    end = col2.date_input("End", date(2024, 2, 1))
    if st.form_submit_button("Load"):
        symbols = [s.strip() for s in tickers.split(",") if s.strip()]
        try:
            with st.spinner("Loading..."):
                ingest(
                    symbols,
                    datetime.combine(start, datetime.min.time()),
                    datetime.combine(end, datetime.min.time()),
                    interval,
                )
            st.success(f"Loaded {', '.join(symbols)}")
            st.cache_data.clear()
        except Exception as exc:  # noqa: BLE001 - show any failure in the UI
            st.error(f"Load failed: {exc}")

# --- Exploration ----------------------------------------------------------------
st.header("Explore")
symbols = load_symbols()
if symbols.empty:
    st.info("No symbols yet: load some data above.")
    st.stop()

col1, col2 = st.columns(2)
ticker = col1.selectbox("Symbol", symbols["ticker"])
view_interval = col2.selectbox("Interval ", INTERVALS, index=1)
bars = load_bars(symbols.loc[symbols["ticker"] == ticker, "id"].iloc[0], view_interval)

if bars.empty:
    st.warning("No bars for this symbol and interval.")
else:
    c1, c2, c3 = st.columns(3)
    c1.metric("Bars", f"{len(bars):,}")
    c2.metric("First", str(bars["timestamp"].min()))
    c3.metric("Last", str(bars["timestamp"].max()))
    st.plotly_chart(
        px.line(bars, x="timestamp", y=["close_price", "vwap"], title=f"{ticker} price"), use_container_width=True
    )
    st.plotly_chart(px.bar(bars, x="timestamp", y="volume", title="Volume"), use_container_width=True)
    st.dataframe(bars.tail(200), use_container_width=True)
