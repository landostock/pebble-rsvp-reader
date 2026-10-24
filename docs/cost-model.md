# Cost Model

Cost status: deployed as a deliberately small personal Fly.io instance.

## Required Runtime Shape

- 1 Fly Machine.
- `shared-cpu-1x`.
- 512 MB RAM. The largest current EPUB processing run peaked at about 208 MB RSS, so 256 MB does not leave a safe margin.
- One region.
- `auto_stop_machines = "stop"`.
- `auto_start_machines = true`.
- `min_machines_running = 0`.
- Application state stored only on one encrypted 1 GB Fly Volume.
- Seven-day volume snapshots plus periodic downloadable backups.

## Forbidden Infrastructure

- Postgres.
- Redis or Upstash.
- Permanently running Fly Machines.
- Multiple machines.
- Dedicated IPv4.
- Background workers that stay running.
- Server-side cron or polling.

## Cost Guards

- Maximum individual EPUB: 50 MB.
- Default source-library limit: 2 GB.
- Warning threshold: 1.5 GB.
- Hard storage guard: configurable.
- Web UI must show approximate stored bytes.
- Volume auto-extension remains disabled.

## Expected Cost Shape

- Machine compute is billed only while the auto-started Machine is running.
- The fixed 1 GB volume has a small monthly storage charge.
- Shared IPv4 and public IPv6 avoid a dedicated IPv4 charge.
- There is no database, Redis, worker, custom domain, or always-on Machine.

## Reading Path

Watch reading latency must be solved through Android local caching. Keeping Fly.io awake for watch responsiveness is an architectural defect.
