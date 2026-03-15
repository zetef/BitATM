FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y libssl3 libpq5 && \
    rm -rf /var/lib/apt/lists/*

COPY bitatm-backend /usr/local/bin/bitatm-backend
RUN chmod +x /usr/local/bin/bitatm-backend

EXPOSE 8080
CMD ["bitatm-backend"]