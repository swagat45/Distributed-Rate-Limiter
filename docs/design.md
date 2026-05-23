# Distributed Rate Limiter Design

## Overview

This service enforces per-client quotas with a token-bucket algorithm. Clients call gRPC APIs, while Redis stores both the client configuration and the live bucket state so multiple server instances can share quota decisions.

## Service Contract

The gRPC surface contains three RPCs:

- `CheckQuota(client_id, hits)` to consume tokens and return allow/deny state
- `UpdateQuota(...)` to administratively add tokens or set a token count
- `UpsertClientConfig(...)` to store per-client capacity, refill rate, and TTL

The proto contract is defined in `proto/ratelimit.proto`.

## Data Model

Each client uses two Redis keys:

- `rl:cfg:<client_id>` stores `capacity`, `refill_rate_per_sec`, and `ttl_seconds`
- `rl:bkt:<client_id>` stores `tokens` and `ts` for the live bucket

If no config exists for a client, the server falls back to the process-level defaults from environment variables.

Both keys are assigned expirations so inactive clients naturally age out of Redis.

## Token-Bucket Behavior

- Capacity defines the maximum burst size
- Refill rate defines how many tokens are restored per second
- Each `CheckQuota` request refills based on elapsed time, then attempts to consume `hits`
- `reset_after_ms` reports either time until enough tokens exist for the request or time until the bucket is full again

The quota consumption path is executed with a Redis Lua script so bucket updates remain atomic.

## Observability

The service exports four Prometheus metrics:

- `ratelimiter_requests_total`
- `ratelimiter_allowed_requests_total`
- `ratelimiter_rejected_requests_total`
- `ratelimiter_latency_ms`

Grafana assets remain in `grafana/dashboards/`.

## Validation Strategy

The project uses unit tests for:

- core token-bucket math
- refill and burst edge cases
- quota update and config reconciliation
- gRPC service validation paths
- metrics changes for allowed and rejected requests

Local load testing uses the k6 gRPC script in `scripts/bench_k6.js` to run roughly 500-1000 quota checks and observe latency and rejection behavior.
