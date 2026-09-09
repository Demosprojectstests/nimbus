FROM ubuntu:24.04 AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git libasio-dev libssl-dev libsqlite3-dev ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY api/CMakeLists.txt /src/CMakeLists.txt
COPY api/include /src/include
COPY api/src /src/src
COPY api/tests /src/tests

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j \
    && ctest --test-dir build --output-on-failure

FROM ubuntu:24.04
RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 libsqlite3-0 ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --system --uid 10001 --create-home appuser \
    && mkdir -p /data && chown appuser:appuser /data

COPY --from=build /src/build/nimbus_api /usr/local/bin/nimbus_api
USER 10001
EXPOSE 8080
ENV BIND_HOST=0.0.0.0 PORT=8080 DB_PATH=/data/nimbus.db
ENTRYPOINT ["/usr/local/bin/nimbus_api"]
