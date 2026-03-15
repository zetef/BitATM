CREATE TABLE IF NOT EXISTS users (
    id            SERIAL PRIMARY KEY,
    username      VARCHAR(64)  UNIQUE NOT NULL,
    password_hash VARCHAR(256) NOT NULL,
    public_key    TEXT,
    last_seen     TIMESTAMP DEFAULT NOW(),
    created_at    TIMESTAMP DEFAULT NOW()
);