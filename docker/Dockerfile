
FROM ubuntu:22.04 AS build

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y build-essential cmake git libgrpc++-dev protobuf-compiler-grpc libprotobuf-dev libhiredis-dev libprometheus-cpp-dev

WORKDIR /app
COPY . .
RUN mkdir build && cd build && cmake .. && make -j

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y libgrpc++1 libprotobuf30 libhiredis0.14 libprometheus-cpp1
COPY --from=build /app/build/server /usr/local/bin/ratelimiter_server
ENTRYPOINT ["ratelimiter_server"]
