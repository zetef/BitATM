CREATE TABLE IF NOT EXISTS messages (
    id            SERIAL PRIMARY KEY,
    sender        VARCHAR(64) NOT NULL,
    recipient     VARCHAR(64) NOT NULL,
    body          TEXT        NOT NULL,
    encrypted_key TEXT        NOT NULL,
    status        VARCHAR(16) DEFAULT 'sent',
    created_at    TIMESTAMP   DEFAULT NOW()
);