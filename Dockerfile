FROM node:22-bookworm-slim

ENV NODE_ENV=production
WORKDIR /app

RUN apt-get update \
    && apt-get install --no-install-recommends --yes gosu \
    && rm -rf /var/lib/apt/lists/*

COPY package.json ./
COPY server/package.json server/package.json
COPY shared/package.json shared/package.json
COPY web/package.json web/package.json
COPY server/src server/src
COPY shared/src shared/src
COPY web web
COPY scripts/container-entrypoint.sh /usr/local/bin/pebble-rsvp-entrypoint

RUN chmod 0755 /usr/local/bin/pebble-rsvp-entrypoint \
    && chown -R node:node /app

EXPOSE 8080
ENTRYPOINT ["pebble-rsvp-entrypoint"]
CMD ["node", "server/src/server.js"]
