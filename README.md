# BitATM

End-to-end encrypted chat application built with C++20, Qt 6/QML, and a Poco WebSocket backend.

## Features

- End-to-end encryption (AES-256-GCM + RSA-2048-OAEP key exchange)
- Cross-platform: Linux desktop, Windows desktop, Android (no plans for Apple OSes, sorry not sorry :) )
- Real-time messaging over WebSocket (WSS)
- Offline message queue — messages delivered when recipient reconnects
- PostgreSQL persistence

## Tech Stack

| Layer      | Technology                                                               |
| ---------- | ------------------------------------------------------------------------ |
| Frontend   | Qt 6 / QML, OpenSSL                                                      |
| Backend    | C++20, Poco (WebSocket + HTTP + Data)                                    |
| Database   | PostgreSQL 16 via Poco::Data                                             |
| Deployment | Docker + backend on homeserver ([wss://api.zetef.xyz](wss://api.zetef.xyz)) |
| Build      | CMake 3.25+, vcpkg, Ninja                                                |
| CI         | GitHub Actions (ubuntu-latest + windows-latest)                          |

## Getting Started

### Docker Compose (backend-only / recommended)

```bash
mkdir -p /path/to/custom/bitatm-stack
cd /path/to/custom/bitatm-stack
git clone https://github.com/zetef/BitATM
cp BitATM/docker-compose.yml .

## modify the docker-compose.yml to your needs
## also modify inside BitATM/backend/main.cpp
## to point to localhost:EXTERNAL_PORT from 
## your modified docker-compose.yml

## SETUP DATABASE PASSWORD

## please input the password another way
echo -ne "POSTGRES_PASSWORD=change_me" > .env
## seal it up
chmod 400 .env

docker compose up -d

## SIDE NOTE
## technically it should just work to modify the
## source code on the main branch as well but we 
## would like our main branch to include the
## fully deployed code
## maybe as a TODO we would include a custom param
## in the docker-compose.yml file to change the 
## backend API URL to something custom
```

### Build from source (both back/frontend)

#### Prerequisites

- CMake 3.25+
- Qt 6.7+ (Desktop kit)
- vcpkg (set `VCPKG_ROOT` in your environment to your vcpkg installation folder, also the following will be installed from [vcpkg.json](vcpkg.json))
  - Poco (Util, Data, Net, PostgreSQL)
  - OpenSSL
  - libpq
- Ninja
- Doxygen (optional for documentation)

#### Build

```bash
git clone https://github.com/zetef/BitATM
cd BitATM
cmake --preset=default
cmake --build build
```

#### Run Tests

```bash
ctest --test-dir build --output-on-failure
```

#### Generate Docs (optional)

```bash
cmake --build build --target docs
# Output: docs/generated/html/index.html
```

We also host our own Github Pages documentation at [docs.bitatm.zetef.xyz](https://docs.bitatm.zetef.xyz)

## Project Structure

```
BitATM/
├── common/          # Shared: Packet protocol, AppException hierarchy
├── backend/         # Poco WebSocket server, handlers, DB layer
├── frontend/        # Qt 6 QML client, crypto engine
│   └── qml/         # QML UI files
├── db/
│   └── migrations/  # Numbered SQL migration files
├── .github/
│   └── workflows/   # CI: Linux + Windows builds
└── docker-compose.yml # Example docker compose deployment
```

## License

See [LICENSE](LICENSE)
