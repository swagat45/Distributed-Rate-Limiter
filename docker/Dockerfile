
FROM ubuntu:22.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libgrpc++-dev \
    protobuf-compiler-grpc \
    libprotobuf-dev \
    libhiredis-dev \
    libcurl4-openssl-dev \
    libssl-dev

# Build Prometheus C++ from source
WORKDIR /build_prom
RUN git clone --recurse-submodules https://github.com/jupp0r/prometheus-cpp.git . && \
    git submodule update --init --recursive && \
    mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON && \
    make -j$(nproc) && \
    make install && \
    ldconfig

# Build the rate limiter application
WORKDIR /app
COPY . .
RUN mkdir build && cd build && cmake .. && make -j$(nproc)

# Runtime stage
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libgrpc++1 \
    libprotobuf30 \
    libhiredis0.14 \
    libcurl4 \
    libssl3

# Copy built prometheus-cpp libraries from build stage
COPY --from=build /usr/local/lib/libprometheus*.so* /usr/local/lib/
COPY --from=build /usr/local/include/prometheus /usr/local/include/prometheus

# Copy the compiled server binary
COPY --from=build /app/build/server /usr/local/bin/ratelimiter_server

# Update library cache
RUN ldconfig

ENTRYPOINT ["ratelimiter_server"]
