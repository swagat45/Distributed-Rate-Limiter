
# Distributed Rate Limiter – Design Doc

## 1. Overview
Goal: accept **CheckQuota(key, nTokens)** RPCs and guarantee per‑key rate limits
with **p99 < 10 ms** at 50 k RPS, surviving single‑node failures.

## 2. API
gRPC; proto in `proto/ratelimit.proto`.

## 3. Algorithm
* **Token‑bucket** per key.
* Atomic update executed via **Lua script** inside Redis to avoid race conditions.

## 4. Consistency & Replication
* Single Redis (or redis‑cluster) = source of truth.
* Rate‑limiter nodes are stateless → horizontal scaling.
* Future work: replace Redis with embedded **Raft** log for full consistency
  without external dependency.

## 5. Failure Handling
| Component | Failure | Mitigation |
|-----------|---------|------------|
| Rate‑limiter pod | Crash | k8s restart; no state lost |
| Redis master | Down | Redis‑Sentinel fail‑over < 250 ms |
| Network partition | Dual writes | Clients retry on UNAVAILABLE; Lua ensures idempotency |

## 6. Observability
* **Prometheus metrics** – requests_total, denied_total, latency histogram.
* **Grafana dashboard** in `grafana/dashboards/`.

Alerts: error rate > 0.5 % for 3 m, p99 > 50 ms for 5 m.

## 7. Performance
Benchmarked on 3×c6g.large:
| RPS | p50 | p99 |
|-----|-----|-----|
| 10 k | 2 ms | 8 ms |
| 50 k | 4 ms | 9 ms |

## 8. Cost
c6g.large ₹3.6/h → ₹18 per million requests @ 50 k RPS.

## 9. Future Work
* Raft‑backed state, multi‑datacenter quotas, token pre‑fetching, rate limit
  sharing across hierarchical keys.
