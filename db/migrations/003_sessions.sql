CREATE TABLE IF NOT EXISTS sessions (
    id            SERIAL PRIMARY KEY,
    user_id       INTEGER REFERENCES users(id),
    session_token VARCHAR(256) UNIQUE NOT NULL,
    created_at    TIMESTAMP DEFAULT NOW(),
    expires_at    TIMESTAMP NOT NULL,
    is_active     BOOLEAN   DEFAULT TRUE
);