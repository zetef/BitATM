-- Migration 007: store sender's own copy of AES key for multi-device sync
ALTER TABLE messages ADD COLUMN IF NOT EXISTS sender_encrypted_key TEXT;
