import grpc from 'k6/net/grpc';

const quotaClient = new grpc.Client();
quotaClient.load(['proto'], 'ratelimit.proto');

export const options = {
  vus: 20,
  iterations: 1000,
};

export function setup() {
  const adminClient = new grpc.Client();
  adminClient.load(['proto'], 'ratelimit.proto');
  adminClient.connect('localhost:50051', { plaintext: true });
  adminClient.invoke('ratelimiter.RateLimiter/UpsertClientConfig', {
    client_id: 'loadtest-client',
    capacity: 25,
    refill_rate_per_sec: 5,
    ttl_seconds: 300,
    reset_quota_state: true,
  });
  adminClient.close();
}

export default function () {
  quotaClient.connect('localhost:50051', { plaintext: true });
  quotaClient.invoke('ratelimiter.RateLimiter/CheckQuota', {
    client_id: 'loadtest-client',
    hits: 1,
  });
  quotaClient.close();
}
