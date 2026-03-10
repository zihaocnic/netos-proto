FROM ubuntu:22.04 AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential cmake \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build -j

FROM ubuntu:22.04

RUN apt-get update \
    && apt-get install -y --no-install-recommends redis-server \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /opt/netos/bin
COPY --from=build /src/build/src/netos/netos-node /opt/netos/bin/netos-node
COPY infra/entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

ENV NETOS_REDIS_SOCKET=/var/run/redis/redis.sock

EXPOSE 9000/udp

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
