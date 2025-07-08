
import http from 'k6/http';
import { sleep } from 'k6';

export const options = {
  vus: 100,
  duration: '30s',
};

export default function () {
  const payload = JSON.stringify({ key: "loadtest", hits: 1 });
  const params = { headers: { 'Content-Type': 'application/json' } };
  http.post('http://localhost:50051/ratelimit.RateLimiter/CheckQuota', payload, params);
  sleep(0.01);
}
