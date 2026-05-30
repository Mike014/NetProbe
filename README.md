# NetProbe

Studio comparativo sulla latenza TCP in C++ moderno.
Ispirato a c-tcp-latency di Lorenzo Martini.

## Build

```bash
cmake -B build -G "Ninja"
cmake --build build
```

## Struttura

- `src/` — sorgenti: server, client, header condiviso
- `analysis/` — calcolo percentili e statistiche
- `docs/` — documentazione e risultati