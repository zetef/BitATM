CREATE TABLE IF NOT EXISTS offline_queue (
    id                SERIAL PRIMARY KEY,
    message_id        INTEGER REFERENCES messages(id),
    recipient         VARCHAR(64) NOT NULL,
    queued_at         TIMESTAMP DEFAULT NOW(),
    delivered         BOOLEAN   DEFAULT FALSE,
    delivery_attempts INTEGER   DEFAULT 0
);