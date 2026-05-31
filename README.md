# NetProbe

**TCP round-trip latency benchmark with cycle-accurate measurement.**

Inspired by [c-tcp-latency](https://github.com/lorenzomartini/c-tcp-latency) by Lorenzo Martini,
NetProbe measures the **round-trip time (RTT)** between a client and server using the x86
**RDTSC** instruction for sub-microsecond precision. The client sends a fixed-size payload,
the server echoes it back, and every iteration is timed in CPU cycles.

Output is printed in **real time** with ANSI color coding: green for normal latencies,
**red for spikes**, making outliers immediately visible at a glance.

## Features

- **Cycle-level precision** — RDTSC/RDTSCP measures RTT in CPU cycles, no OS syscall overhead
- **Real-time color-coded output** — green for expected latency, red for anomalies
- **Configurable payload** — adjust message size with `-b` to test different workloads
- **Docker-native** — single `docker compose up` spins up server and client in isolated containers
- **Cross-platform** — builds on Linux GCC, Windows MSYS2, and MSVC
- **TCP_NODELAY** — disables Nagle's algorithm for clean one-packet-per-message semantics

## Architecture

```
┌─────────────────────────────────────────────────┐
│                   Client                        │
│                                                 │
│   for i in 1..N_ROUNDS {                        │
│       tsc_start = rdtsc()                       │
│       send_message(sockfd, payload)     ───────┐│
│       receive_message(sockfd, reply)   ←───────│┤
│       tsc_end = rdtscp()                        │ 
│       rtt_cycles = tsc_end - tsc_start          │ │
│       print_color(rtt_cycles)                   │ │
│   }                                             │ │
└─────────────────────────────────────────────────┘ │
                                                    
┌─────────────────────────────────────────────────┐ 
│                   Server                        │ │
│                                                 | |
│   listen(port)                                  | |
│   accept() → client_fd                          | |
│   for i in 1..N_ROUNDS {                        │ │
│       receive_message(client_fd, payload)  ←─────┘|
│       send_message(client_fd, payload)          |
│   }                                             │
└─────────────────────────────────────────────────┘
```

The server blocks on `accept` for a single connection, then enters an echo loop.
The client measures every round trip with `rdtsc()` (before send) and `rdtscp()`
(after receive), computes the delta, and prints the result.

## Project structure

```
netprobe/
├── CMakeLists.txt            # Root CMake (C++20)
├── Dockerfile                # Multi-stage build
├── docker-compose.yml        # Server + client orchestration
├── src/
│   ├── CMakeLists.txt        # Executable targets
│   ├── connection.hpp        # Socket wrappers, config, RDTSC helpers
│   ├── server.cpp            # TCP echo server
│   └── client.cpp            # TCP benchmark client
```

## Build on Windows with MSYS2

### Prerequisites

- [MSYS2](https://www.msys2.org/) — **UCRT64** environment
- GCC 13+ (required for `std::format`)
- CMake 3.20+
- Ninja (optional, speeds up builds)

### Install toolchain

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

### Build

```bash
mkdir build
cmake -G Ninja -B build
cmake --build build
```

Binaries land in `build/bin/`:

```bash
build/bin/server.exe -p 8080 -b 1024
build/bin/client.exe -a 127.0.0.1 -p 8080 -b 1024
```

### Local test (two terminals)

**Terminal 1 — server:**

```bash
build/bin/server.exe -p 8080 -b 64
```

**Terminal 2 — client:**

```bash
build/bin/client.exe -a 127.0.0.1 -p 8080 -b 64
```

## Run with Docker (single command)

```bash
docker compose up --build
```

Docker Compose builds both images, starts the server, and then the client.
The client polls `nc -z server 8080` until the server is ready, then starts
the benchmark.

To stop:

```bash
docker compose down
```

## Command-line options

| Flag | Default      | Description                        |
|------|--------------|------------------------------------|
| `-p` | `8080`       | TCP port                           |
| `-b` | `1`          | Payload size in bytes              |
| `-a` | `127.0.0.1`  | Server address (client only)       |

## Example output

```
Address: server, Port: 8080, N_bytes: 1024
Connected to server:8080, starting send-receive loop
  Iter      1  RTT      12340 cycles  ✓
  Iter      2  RTT      12301 cycles  ✓
  Iter      3  RTT      12288 cycles  ✓
  ...
  Iter  47891  RTT     832140 cycles  ✗  ←  scheduler jitter / CPU migration
  Iter  47892  RTT      12355 cycles  ✓
  ...
  Iter 923445  RTT    1204567 cycles  ✗  ←  network congestion / queue full
  Iter 923446  RTT      12310 cycles  ✓
  ...
```

Normal iterations print in **green**; outlier spikes (e.g., >2× median) print in
**red** and are tagged with a marker. This makes it easy to spot intermittent
events like context switches, hypervisor interference, or cross-container
network contention.

## Technical notes

### RDTSC and RDTSCP

NetProbe uses the x86 `rdtsc` / `rdtscp` instructions to timestamp each round
trip. Advantages over `clock_gettime` / `std::chrono`:

- **~20 cycles overhead** vs hundreds for a syscall-based timer
- **Monotonic** on modern x86 (invariant TSC guaranteed)
- **rdtscp** adds a dispatch serialisation that prevents earlier instructions
  from skewing the read

On older CPUs or virtualised environments without invariant TSC, results may
not be comparable across cores. For Docker-on-hypervisor deployments, verify
that `invariant_tsc` is exposed to the guest.

### TCP_NODELAY

Both client and server enable `TCP_NODELAY` (via `setsockopt`), disabling
Nagle's algorithm. This ensures that every `send()` call is transmitted
immediately as a single TCP segment rather than being delayed waiting for
more data. Without this flag, small payloads could coalesce and inflate
RTT measurements.

### Why non-blocking server?

The server's data socket runs in non-blocking mode (`FIONBIO`). The
`receive_message` / `send_message` loops busy-wait on
`EAGAIN`/`EWOULDBLOCK` rather than blocking in the kernel. This is a
deliberate microbenchmark trade-off: it eliminates context-switch
jitter at the cost of burning CPU cycles on idle spins.

### Platform-specific considerations

- `std::format` requires a C++20 standard library (GCC 13+, MSVC 2022 17+).
  Older toolchains can replace `<format>` / `std::format` with
  `<iostream>` / manual formatting.
- On MSYS2, Winsock calls propagate errors through `errno`, so the
  generic `panic()` helper works transparently. Native MSVC builds need
  `WSAGetLastError()` instead.
