FROM debian:bookworm-slim

RUN apt-get update && \
    apt-get install -y libssl3 libpq5 && \
    rm -rf /var/lib/apt/lists/*

COPY build/backend/backend /usr/local/bin/e2ee-backend

EXPOSE 8080
CMD ["bitatm-backend"]