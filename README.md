# Distributed Rate Limiter

A C++17 distributed rate limiter built around a token-bucket algorithm, gRPC APIs, Redis-backed shared state, Prometheus metrics, Docker-based verification, and Swagger-backed API documentation.

## What This Project Demonstrates

- Token-bucket request throttling with refill timing and burst handling
- Three gRPC APIs for quota checks, quota updates, and per-client config management
- Redis-backed per-client config and bucket state with atomic quota consumption
- Prometheus metrics for total requests, allowed requests, rejected requests, and latency
- Docker-first build and test verification
- Swagger UI documentation for the gRPC contract through a checked-in OpenAPI file

## Architecture

```text
gRPC client
    |
    v
RateLimiter service (C++)
    |
    +-- Token bucket math
    +-- Prometheus metrics
    |
    v
Redis
    +-- rl:cfg:<client_id>   per-client capacity/refill/ttl
    +-- rl:bkt:<client_id>   current tokens + last refill timestamp

Swagger UI
    |
    +-- renders docs/openapi.yaml for humans
```

The service is stateless apart from Redis, so multiple server instances can share the same quota state.

## gRPC API

The service definition lives in [proto/ratelimit.proto](/d:/Swagat%20Suman%20Mishra/CORE%20PROJECTS/Distributed-Rate-Limiter/proto/ratelimit.proto:1).

```proto
service RateLimiter {
  rpc CheckQuota(CheckQuotaRequest) returns (CheckQuotaReply);
  rpc UpdateQuota(UpdateQuotaRequest) returns (UpdateQuotaReply);
  rpc UpsertClientConfig(UpsertClientConfigRequest) returns (UpsertClientConfigReply);
}
```

- `CheckQuota` consumes tokens for a client and returns `allowed`, `remaining`, and `reset_after_ms`
- `UpdateQuota` is an admin API that either adds tokens or sets the current token count
- `UpsertClientConfig` stores per-client `capacity`, `refill_rate_per_sec`, and `ttl_seconds`, with optional quota reset

## Docker Verification

Docker is the primary verification path for this repo. The image build now performs a real compile and test pass during the build stage.

### 1. Build From Scratch

```bash
docker compose build --no-cache
```

Expected result:

- the build stage installs dependencies
- CMake configures the project
- all binaries compile
- `ctest --output-on-failure` passes before the runtime image is produced

### 2. Start The Stack

```bash
docker compose up -d
```

Services and ports:

- `50051` gRPC rate limiter
- `9102` Prometheus metrics endpoint
- `9090` Prometheus UI
- `3000` Grafana
- `8080` Swagger UI

### 3. Inspect Logs

```bash
docker compose logs ratelimiter
```

Expected result:

- the server starts successfully
- it binds to `0.0.0.0:50051`
- it exposes metrics on `0.0.0.0:9102`

### 4. Run A Real gRPC Smoke Test

The runtime image now includes the example client binary, so you can execute it inside the running service container:

```bash
docker compose exec ratelimiter /usr/local/bin/ratelimiter_client localhost:50051 smoke-client
```

Expected result:

- `UpsertClientConfig` succeeds
- repeated `CheckQuota` calls show allowed and eventually rejected behavior
- `UpdateQuota` succeeds
- a final quota check succeeds after the update

### 5. Verify Metrics

Open `http://localhost:9102/metrics` and confirm these metrics appear:

- `ratelimiter_requests_total`
- `ratelimiter_allowed_requests_total`
- `ratelimiter_rejected_requests_total`
- `ratelimiter_latency_ms`

### 6. Shut Down

```bash
docker compose down
```

## Swagger UI

Swagger documentation is served from the checked-in OpenAPI file at [docs/openapi.yaml](/d:/Swagat%20Suman%20Mishra/CORE%20PROJECTS/Distributed-Rate-Limiter/docs/openapi.yaml:1).

Start the stack and open:

```text
http://localhost:8080
```

Important note:

- Swagger UI in this repo is documentation for the gRPC contract
- it does not mean the server exposes a live REST/JSON gateway
- the OpenAPI file provides HTTP-style representations only to make the API browsable and easier to discuss

## Manual Native Build

If you later install a native toolchain, you can still build locally without Docker:

```bash
mkdir build
cd build
cmake -DWITH_RAFT=OFF ..
cmake --build .
ctest --output-on-failure
```

## Load Testing

The local k6 script in [scripts/bench_k6.js](/d:/Swagat%20Suman%20Mishra/CORE%20PROJECTS/Distributed-Rate-Limiter/scripts/bench_k6.js:1) uses k6's gRPC client and runs `1000` total `CheckQuota` calls after seeding a client config.

Example:

```bash
k6 run scripts/bench_k6.js
```

If `k6` is not installed on your machine, keep it as an optional follow-up step after Docker verification.

## Tests

The test suite covers:

- token consumption and rejection
- refill timing
- burst handling and clamping
- invalid token and config inputs
- quota update behavior
- config reconciliation
- service validation and status codes
- metric increments for allowed and rejected requests

## Repository Layout

- `proto/` gRPC contract
- `src/` server, client, Redis store, metrics, token-bucket logic
- `tests/` unit tests
- `scripts/` local load testing
- `docker/` container and Prometheus config
- `docs/` design notes and OpenAPI spec
- `grafana/` dashboard assets
