import streamlit as st
import pandas as pd
import mysql.connector
import plotly.express as px
from datetime import datetime
from pathlib import Path
from dotenv import load_dotenv
import os
from alpaca.data.historical import StockHistoricalDataClient, CryptoHistoricalDataClient
from alpaca.data.requests import StockBarsRequest, CryptoBarsRequest
from alpaca.data.timeframe import TimeFrame, TimeFrameUnit

# 📦 Import depuis db_utils
from db_utils import fetch_and_store, get_or_create_symbol, get_or_create_data_vendor,determine_asset_type

# 🌱 Charger .env
env_path = Path(__file__).resolve().parent.parent / ".env"
load_dotenv(dotenv_path=env_path)

DB_CONFIG = {
    "host": os.getenv("MYSQL_HOST"),
    "user": os.getenv("MYSQL_USER"),
    "password": os.getenv("MYSQL_PASSWORD"),
    "database": os.getenv("MYSQL_DATABASE")
}

API_KEY = os.getenv("API_KEY")
API_SECRET = os.getenv("SECRET_KEY")

@st.cache_resource
def get_connection():
    return mysql.connector.connect(**DB_CONFIG)

conn = get_connection()

# ------------------------
# 🎯 Interface Streamlit
# ------------------------

st.set_page_config(page_title="Dashboard Alpaca", layout="wide")
st.title("📈 Dashboard - Données Alpaca")

# 📥 Section : Ajouter des données Alpaca
st.header("📡 Ajouter des données depuis Alpaca")

with st.form("import_form"):
    col1, col2 = st.columns(2)
    with col1:
        symbols_input = st.text_input("🔤 Tickers (ex: AAPL, BTC/USD)", "AAPL, BTC/USD")
        interval = st.selectbox("⏱️ Intervalle", ["1Min", "5Min", "15Min", "1H", "1Day"])
    with col2:
        start_date = st.date_input("🗓️ Date de début", datetime(2023, 1, 1))
        end_date = st.date_input("🗓️ Date de fin", datetime(2024, 1, 1))

    submitted = st.form_submit_button("📥 Importer")


if submitted:
    symbols = [symbol.strip() for symbol in symbols_input.split(",")]
    try:
        with st.spinner("Importation des données..."):
            for symbol in symbols:
                asset_type = determine_asset_type(symbol)
                data_vendor_id = get_or_create_data_vendor()
                symbol_id = get_or_create_symbol(symbol, instrument=asset_type)
                fetch_and_store(symbol, start_date, end_date, interval, asset_type)
            st.success(f"✅ Données ajoutées pour les symboles : {', '.join(symbols)}")
            
            # Forcer le rafraîchissement de l'interface pour afficher les données mises à jour
            st.rerun()  # Remplace st.experimental_rerun() par st.rerun()

    except Exception as e:
        st.error(f"❌ Erreur lors de l'importation des données : {e}")


# 🔌 Déconnexion propre
def close_connection():
    if conn.is_connected():
        conn.close()

def ensure_connection():
    # Vérifie si la connexion est valide, sinon en crée une nouvelle
    global conn
    if conn is None or not conn.is_connected():
        st.warning("Connexion MySQL perdue. Reconnexion...")
        conn = get_new_connection()  # Fonction pour ouvrir une nouvelle connexion
    return conn

def get_new_connection():
    try:
        return mysql.connector.connect(**DB_CONFIG)  # Connexion MySQL
    except mysql.connector.Error as err:
        st.error(f"Erreur de connexion MySQL: {err}")
        return None

st.divider()

# 📊 Section : Visualisation des données
st.header("🔍 Visualisation des données")

@st.cache_data
def get_symbols():
    query = "SELECT id, ticker, name, instrument FROM symbol ORDER BY ticker"
    with conn.cursor(dictionary=True) as cursor:
        cursor.execute(query)
        result = cursor.fetchall()
    return pd.DataFrame(result)

symbols_df = get_symbols()

if symbols_df.empty:
    st.warning("Aucun symbole disponible.")
else:
    symbol_choice = st.selectbox("🎯 Choisissez un symbole :", symbols_df["ticker"])
    symbol_id = symbols_df[symbols_df["ticker"] == symbol_choice]["id"].values[0]

    @st.cache_data
    def get_price_data(symbol_id):
        conn = ensure_connection()
        query = """
            SELECT timestamp, open_price, close_price, high_price, low_price, volume
            FROM price_bar
            WHERE symbol_id = %s
            ORDER BY timestamp
        """
        
        # Convertir symbol_id en entier (au cas où il serait en int64)
        symbol_id = int(symbol_id)  # Convertir symbol_id en int

        with conn.cursor(dictionary=True) as cursor:
            cursor.execute(query, (symbol_id,))
            result = cursor.fetchall()
        
        return pd.DataFrame(result)


    price_df = get_price_data(symbol_id)

    if price_df.empty:
        st.warning("Aucune donnée pour ce symbole.")
    else:
        st.subheader("📊 Données brutes")
        st.dataframe(price_df.tail(200), use_container_width=True)

        st.subheader("📉 Graphique des prix")
        fig = px.line(price_df, x="timestamp", y=["open_price", "close_price"],
                      title=f"Prix - {symbol_choice}")
        st.plotly_chart(fig, use_container_width=True)

        st.subheader("🔁 Volume échangé")
        fig_volume = px.area(price_df, x="timestamp", y="volume", title="Volume")
        st.plotly_chart(fig_volume, use_container_width=True)




close_connection()

