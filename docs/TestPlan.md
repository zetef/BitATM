# BitATM Test Plan - Integration Test Results

Manual integration tests IT-01..IT-10 run against the live homelab
(wss://api.zetef.xyz, backend + PostgreSQL on hades). Unit tests (UT-BE-01..06,
UT-FE-01..08) run via CTest; see `.claude/rules/testing.md` for definitions.

## Run: 2026-07-12 (Stefan, DB truncated before run)

Setup: two desktop client instances on Linux (second instance isolated via
XDG_CONFIG_HOME/XDG_DATA_HOME), accounts alice and bob, server-side
verification via psql on hades.

| ID | Name | Result | Notes |
| ---- | ------ | -------- | ------- |
| IT-01 | Registration | Pass | users rows created, salted password hashes, RSA pubkeys stored (base64 PEM) |
| IT-02 | Login | Pass | Active session rows with 24h expiry token; old sessions deactivated |
| IT-03 | Failed login | Pass | "LOGIN: invalid credentials" error packet, no session row created |
| IT-04 | Message (both online) | Pass | Live delivery, messages row stored |
| IT-05 | E2EE verification | Pass | body is AES-GCM ciphertext; plaintext grep across DB: 0 hits; sender_encrypted_key present for sender-side sync |
| IT-06 | Offline queue | Pass | offline_queue row created, delivered on login, delivered=t after 1 attempt |
| IT-07 | Key exchange | Pass | Server returned bob's pubkey; alice encrypted per-message AES key for bob |
| IT-08 | Cross-platform | Pending | David/Windows client not available during run |
| IT-09 | Reconnect | Partial | Clients detect drop and auto-reconnect the socket, all flows work after manual re-login. No automatic session resume: users are returned to the login screen. Follow-up: frontend session resume (David) |
| IT-10 | Android | Pending | No APK device available during run |

## Defects found and fixed during this run

1. Idle WebSocket disconnect after 60s. Poco's default HTTP session timeout
   was inherited by the upgraded WebSocket and no side sent keepalives, so any
   60s-idle connection was dropped and its session deactivated. Fix: server
   answers WS ping frames (ClientSession) with a 600s receive timeout
   (Server.cpp, WS_RECEIVE_TIMEOUT_SEC); client pings every 30s
   (NetworkManager, WS_CLIENT_PING_INTERVAL_SEC). Verified: idle clients
   survive indefinitely, silent dead peers still reaped.

2. Duplicate messages on history sync + read receipts never matching for
   server-replayed messages. Live packets carry client ISO timestamps
   ("...T...Z") while sync/offline replays carry PostgreSQL text
   ("YYYY-MM-DD HH:MM:SS.f"); dedup and seen-status matching compare strings.
   Fix: NetworkManager::canonicalTimestamp() normalizes every inbound
   timestamp (messages, group messages, read receipts, delivery ACKs) to
   Qt::ISODateWithMs UTC. Verified: clean re-sync with no duplicates, seen
   status propagates for offline-delivered messages.
