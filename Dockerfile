FROM gcc:latest AS builder

RUN apt-get update && apt-get install -y cmake ninja-build

WORKDIR /app
COPY . .

RUN cmake -B build -G "Ninja" && cmake --build build

FROM gcc:latest

RUN apt-get update && apt-get install -y netcat-openbsd

COPY --from=builder /app/build/bin/server /usr/local/bin/netprobe-server
COPY --from=builder /app/build/bin/client /usr/local/bin/netprobe-client