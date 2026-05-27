-- Migration 008: queue read receipts for offline delivery
-- When the original message sender is offline, the receipt is stored here
-- and flushed to them on their next login.
CREATE TABLE IF NOT EXISTS offline_read_receipts (
    id          SERIAL       PRIMARY KEY,
    from_user   VARCHAR(64)  NOT NULL REFERENCES users(username) ON DELETE CASCADE,
    to_user     VARCHAR(64)  NOT NULL REFERENCES users(username) ON DELETE CASCADE,
    message_ts  TEXT         NOT NULL,
    queued_at   TIMESTAMPTZ  DEFAULT NOW(),
    delivered   BOOLEAN      NOT NULL DEFAULT FALSE
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_orr_dedup ON offline_read_receipts(from_user, to_user, message_ts);
CREATE INDEX IF NOT EXISTS idx_orr_to_user ON offline_read_receipts(to_user) WHERE delivered = FALSE;
