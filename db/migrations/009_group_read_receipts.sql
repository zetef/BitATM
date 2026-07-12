-- 009: group read receipts
-- Adds group context to queued read receipts so a receipt for a group
-- message can be replayed with its group id intact. NULL = 1:1 receipt.
ALTER TABLE offline_read_receipts
    ADD COLUMN group_id INTEGER REFERENCES groups(id) ON DELETE CASCADE;
