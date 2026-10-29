-- Securities master for intraday bars.
-- Usage: mysql -u <user> -p < sql/schema.sql

CREATE DATABASE IF NOT EXISTS securities_master;
USE securities_master;

CREATE TABLE IF NOT EXISTS data_vendor (
    id                INT          NOT NULL AUTO_INCREMENT,
    name              VARCHAR(64)  NOT NULL,
    website_url       VARCHAR(255) NULL,
    created_date      DATETIME     NOT NULL,
    last_updated_date DATETIME     NOT NULL,
    PRIMARY KEY (id),
    UNIQUE KEY uq_vendor_name (name)
);

CREATE TABLE IF NOT EXISTS symbol (
    id                INT          NOT NULL AUTO_INCREMENT,
    ticker            VARCHAR(32)  NOT NULL,
    instrument        VARCHAR(64)  NOT NULL,   -- stock, etf, crypto
    name              VARCHAR(255) NULL,
    sector            VARCHAR(255) NULL,
    currency          VARCHAR(32)  NULL,
    created_date      DATETIME     NOT NULL,
    last_updated_date DATETIME     NOT NULL,
    PRIMARY KEY (id),
    UNIQUE KEY uq_symbol_ticker (ticker)
);

CREATE TABLE IF NOT EXISTS price_bar (
    id                BIGINT         NOT NULL AUTO_INCREMENT,
    data_vendor_id    INT            NOT NULL,
    symbol_id         INT            NOT NULL,
    timestamp         DATETIME       NOT NULL,  -- bar start, UTC
    `interval`        VARCHAR(8)     NOT NULL,  -- 1Min, 5Min, 15Min, 1H, 1Day
    open_price        DECIMAL(19, 6) NOT NULL,
    high_price        DECIMAL(19, 6) NOT NULL,
    low_price         DECIMAL(19, 6) NOT NULL,
    close_price       DECIMAL(19, 6) NOT NULL,
    volume            DECIMAL(24, 8) NOT NULL,  -- fractional for crypto
    trade_count       INT            NULL,
    vwap              DECIMAL(19, 6) NULL,
    created_date      DATETIME       NOT NULL,
    last_updated_date DATETIME       NOT NULL,
    PRIMARY KEY (id),
    -- one bar per vendor, symbol, interval and timestamp: makes reloads idempotent
    UNIQUE KEY uq_bar (data_vendor_id, symbol_id, `interval`, timestamp),
    KEY idx_symbol_interval_time (symbol_id, `interval`, timestamp),
    CONSTRAINT fk_bar_vendor FOREIGN KEY (data_vendor_id) REFERENCES data_vendor (id),
    CONSTRAINT fk_bar_symbol FOREIGN KEY (symbol_id) REFERENCES symbol (id)
);
