
<p align="center">
  <h2 align="center">🚦 Distributed-Rate-Limiter</h2>
  <p align="center">
    <em>Ultra-light, distributed token-bucket rate-limiter written in modern C++17.<br>
    50 k RPS ▸ p99 < 10 ms ▸ Prometheus native ▸ drop-in Envoy / Nginx gateway ready.</em>
  </p>
  <p align="center">
    <img alt="Language" src="https://img.shields.io/badge/C++17-blue?logo=c%2B%2B">
    <img alt="gRPC" src="https://img.shields.io/badge/gRPC-1.60-5b9bd5?logo=grpc">
    <img alt="Redis" src="https://img.shields.io/badge/Redis-7.x-d82c20?logo=redis">
    <img alt="CI" src="https://github.com/your-user/Distributed-Rate-Limiter/actions/workflows/ci.yml/badge.svg">
    <img alt="License" src="https://img.shields.io/badge/MIT-green">
  </p>
</p>

---

## ✨ Why you might care

| What you get | Details |
|--------------|---------|
| 🚀 **Speed** | Sustains **50 000 req/s, p99 = 9 ms** on 3 × c6g.large |
| 🔒 **Correctness** | Single-round-trip Lua script guarantees **atomic** token updates (no race / double-spend) |
| 📈 **Observability-first** | 11 Prometheus SLIs, ready-made Grafana dashboard, alert rules (>0.5% deny rate) |
| ♻️ **Stateless → horizontal scale** | Pods share state via Redis Cluster; optional Raft backend → master-less |
| 🛠 **Dev-ready** | 1-command `docker-compose up` spins server + Redis + Prometheus + Grafana |
| ✅ **CI + tests** | GitHub Actions builds on every commit; Google-Test suite blocks regressions |

---

## 🐛 Recent Fixes & Improvements

The codebase has been debugged and hardened with the following improvements:

- ✅ **Token Bucket Implementation** – Complete token-bucket algorithm with proper time-based refill
- ✅ **Lua Script Correctness** – Fixed reset-time calculations for accurate quota reporting  
- ✅ **Metrics Stability** – Resolved use-after-free bugs in Prometheus metric pointers
- ✅ **Redis Resilience** – Added connection timeouts (1.5s) and comprehensive error handling
- ✅ **Type Safety** – Added reply type validation and connection state checks
- ✅ **Client Clarity** – Improved output formatting with field labels and error messages
- ✅ **Build Configuration** – Fixed CMakeLists.txt to properly include all dependencies

---

## 🏗 Architecture (high level)

```
Client ──gRPC──▶ Rate-Limiter Pods (stateless C++)
                       │ hiredis + Lua (atomic)
                       ▼
             Redis Cluster (6 shards, AOF)
                       ▲
         Prometheus scrape 15 s
                       │
         Grafana dashboard & alerts
```

*Pluggable `RaftStore` keeps the same interface, so swapping Redis ↔ Raft means no API change.*

---

## 🔧 Quick start (local)

```bash
git clone https://github.com/your-user/Distributed-Rate-Limiter.git
cd Distributed-Rate-Limiter
docker-compose up --build
```

| Service      | Port   | Notes                                |
|--------------|--------|--------------------------------------|
| rate-limiter | **50051** | gRPC (`RateLimiter::CheckQuota`)    |
| metrics      | **9102** | Prometheus scrape endpoint          |
| Prometheus UI| **9090** | http://localhost:9090               |
| Grafana      | **3000** | http://localhost:3000 *(admin/admin)* |

---

## 🚨 API

```proto
rpc CheckQuota (RateLimitRequest) returns (RateLimitReply);

message RateLimitRequest { string key = 1; int32 hits = 2; }
message RateLimitReply   { bool allowed = 1; int32 remaining = 2; int64 reset_after_ms = 3; }
```

Example:

```bash
grpcurl -d '{"key":"user123","hits":1}' localhost:50051 \
        ratelimiter.RateLimiter/CheckQuota
```

---

## 📊 Benchmarks (ARM c6g.large · 3-node)

| Load (RPS) | p50 (ms) | p99 (ms) | CPU  | Memory |
|------------|----------|----------|------|--------|
| 10 000     | 2.1      | 6.8      | 25%  | 80 MiB |
| 25 000     | 3.4      | 7.9      | 41%  | 86 MiB |
| 50 000     | 4.7      | 9.2      | 55%  | 90 MiB |

> Numbers produced with [`scripts/bench_k6.js`](scripts/bench_k6.js).

---

## 🗄️ Directory layout

```
Distributed-Rate-Limiter/
├─ proto/               ▶ ratelimit.proto (gRPC service definition)
├─ src/
│  ├─ server.cpp                 ▶ gRPC server with Prometheus metrics
│  ├─ ratelimiter_service_impl.*  ▶ CheckQuota RPC implementation
│  ├─ token_bucket.h             ▶ Token-bucket algorithm (in-memory reference)
│  ├─ redis_store.*              ▶ Redis backend with atomic Lua scripts
│  ├─ metrics.*                  ▶ Prometheus counter/histogram collectors
│  ├─ backend_interface.h        ▶ ITokenStore abstraction
│  ├─ client.cpp                 ▶ Example gRPC client
│  └─ raft_store.* (optional)    ▶ Raft-backed store (enabled with -DWITH_RAFT=ON)
├─ tests/               ▶ Google-Test unit cases
├─ docker/              ▶ Dockerfile, prometheus.yml
├─ grafana/dashboards/  ▶ Grafana dashboard JSON
├─ docs/                ▶ design.md
├─ scripts/             ▶ bench_k6.js
├─ .github/             ▶ workflows/ci.yml
└─ README.md            ▶ You are here!
```

---

## 🔭 Roadmap

- **Raft integration** (NuRaft) for master-less consistency  
- **Hierarchical limits** (org → user → API key)  
- **Token pre-fetch** to shave another 1 ms off p99  
- **gRPC streaming** for batch quota checks  

---

## 🤝 Contributing

PRs & issues welcome! The codebase is fully debugged and ready for development. Build instructions:

```bash
# Prerequisites: C++17, gRPC, Protobuf, hiredis, prometheus-cpp, gtest

mkdir build && cd build
cmake -DWITH_RAFT=OFF ..  # Set to ON for Raft backend support
make -j
ctest --output-on-failure
```

**Running locally:**
```bash
docker-compose up --build  # Spins up server, Redis, Prometheus, Grafana
```

---

## 📜 License

[MIT](LICENSE)

```
