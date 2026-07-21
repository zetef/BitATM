CREATE TABLE IF NOT EXISTS pending_notifications (
    id          SERIAL PRIMARY KEY,
    recipient   VARCHAR(64) NOT NULL,
    packet_type INTEGER NOT NULL,
    from_user   VARCHAR(64) NOT NULL DEFAULT '',
    body        VARCHAR(255) NOT NULL DEFAULT '',
    created_at  TIMESTAMP DEFAULT NOW()
);
CREATE INDEX idx_pending_notifications_recipient ON pending_notifications(recipient);
