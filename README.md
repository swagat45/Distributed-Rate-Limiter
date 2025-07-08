
# deshaw‑rate‑limiter (C++17)

Production‑style distributed rate limiter ready for interview demos.

## Features
* gRPC API, Redis atomic backend (Lua)
* Prometheus metrics & Grafana dashboard
* Docker‑compose stack: server + Redis + Prometheus + Grafana
* GitHub Actions CI
* Design doc & k6 load test

## Quick start
```bash
git clone <repo>
cd deshaw_rate_limiter
docker-compose up --build
```
Open Grafana `http://localhost:3000` (admin/admin), import dashboard
from `grafana/dashboards/`.

## Build locally
```bash
mkdir build && cd build
cmake .. && make -j
./server   # expects Redis on localhost
```
