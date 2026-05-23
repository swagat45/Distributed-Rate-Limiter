FROM ubuntu:22.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    googletest \
    libcurl4-openssl-dev \
    libgrpc++-dev \
    libgtest-dev \
    libhiredis-dev \
    libprotobuf-dev \
    libssl-dev \
    pkg-config \
    protobuf-compiler-grpc \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build_prom
RUN git clone --recurse-submodules https://github.com/jupp0r/prometheus-cpp.git . && \
    git submodule update --init --recursive && \
    mkdir -p build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DENABLE_TESTING=OFF && \
    make -j"$(nproc)" && \
    make install && \
    ldconfig

WORKDIR /app
COPY . .
RUN mkdir -p build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DWITH_RAFT=OFF && \
    cmake --build . -j"$(nproc)" && \
    ctest --output-on-failure

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libcurl4 \
    libgrpc++1 \
    libhiredis0.14 \
    libprotobuf23 \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /usr/local/lib/libprometheus*.so* /usr/local/lib/
COPY --from=build /usr/local/include/prometheus /usr/local/include/prometheus
COPY --from=build /app/build/server /usr/local/bin/ratelimiter_server
COPY --from=build /app/build/client /usr/local/bin/ratelimiter_client

RUN ldconfig

ENTRYPOINT ["/usr/local/bin/ratelimiter_server"]
