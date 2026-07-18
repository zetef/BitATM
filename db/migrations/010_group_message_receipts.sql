-- 010: per-member group receipt tracking (receipts v2)
CREATE TABLE group_message_receipts (
    message_id   INT  NOT NULL REFERENCES group_messages(id) ON DELETE CASCADE,
    username     TEXT NOT NULL,
    delivered_at TIMESTAMPTZ,
    seen_at      TIMESTAMPTZ,
    PRIMARY KEY (message_id, username)
);

-- offline receipt queue now carries both event kinds
ALTER TABLE offline_read_receipts ADD COLUMN kind TEXT NOT NULL DEFAULT 'seen';
