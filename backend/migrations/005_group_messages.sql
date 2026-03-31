CREATE TABLE IF NOT EXISTS group_messages (
    id                SERIAL PRIMARY KEY,
    group_name        VARCHAR(128) NOT NULL,
    sender            VARCHAR(64)  NOT NULL,
    body              TEXT         NOT NULL,
    encrypted_keys    TEXT         NOT NULL,
    created_at        TIMESTAMP    DEFAULT NOW()
);
