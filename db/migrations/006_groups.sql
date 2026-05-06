-- Migration 006: group chat schema
-- Drops the placeholder table from earlier migrations and creates the correct normalized schema.

DROP TABLE IF EXISTS group_messages CASCADE;

CREATE TABLE groups (
    id         SERIAL       PRIMARY KEY,
    name       VARCHAR(128) NOT NULL,
    creator    VARCHAR(64)  NOT NULL REFERENCES users(username),
    created_at TIMESTAMPTZ  DEFAULT NOW()
);

CREATE TABLE group_members (
    group_id  INT         NOT NULL REFERENCES groups(id) ON DELETE CASCADE,
    username  VARCHAR(64) NOT NULL REFERENCES users(username),
    role      VARCHAR(16) NOT NULL DEFAULT 'member',
    joined_at TIMESTAMPTZ DEFAULT NOW(),
    PRIMARY KEY (group_id, username),
    CONSTRAINT role_valid CHECK (role IN ('creator', 'admin', 'member'))
);

CREATE TABLE group_keys (
    group_id          INT         NOT NULL REFERENCES groups(id) ON DELETE CASCADE,
    username          VARCHAR(64) NOT NULL REFERENCES users(username),
    encrypted_aes_key TEXT        NOT NULL,
    PRIMARY KEY (group_id, username)
);

CREATE TABLE group_messages (
    id             SERIAL       PRIMARY KEY,
    group_id       INT          NOT NULL REFERENCES groups(id) ON DELETE CASCADE,
    sender         VARCHAR(64)  NOT NULL REFERENCES users(username),
    encrypted_body TEXT         NOT NULL,
    timestamp      TIMESTAMPTZ  DEFAULT NOW()
);
