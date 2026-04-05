# ─────────────────────────────────────────────────────────────────────────────
# Stage 1: Builder — vcpkg + CMake 빌드 (Ubuntu 22.04, x64-linux triplet)
# ─────────────────────────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    cmake \
    ninja-build \
    git \
    curl \
    zip \
    unzip \
    pkg-config \
    build-essential \
    libssl-dev \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# vcpkg 설치 (builtin-baseline과 동일한 커밋 기준으로 bootstrap)
RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git /vcpkg && \
    /vcpkg/bootstrap-vcpkg.sh -disableMetrics

ENV VCPKG_ROOT=/vcpkg

WORKDIR /app
COPY . .

# Configure & Build
# - VCPKG_TARGET_TRIPLET=x64-linux : 동적 링크 (Windows x64-windows와 동일 역할)
# - PREPROCESS_WORKERS 환경변수는 런타임에 주입 (ADR-028)
RUN cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
    -DCMAKE_BUILD_TYPE=Release \
    -G Ninja && \
    cmake --build build --config Release --target preprocess_server

# ─────────────────────────────────────────────────────────────────────────────
# Stage 2: Runtime — 바이너리 + vcpkg 동적 라이브러리만 포함
# ─────────────────────────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libgomp1 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/bin/preprocess_server .
COPY --from=builder /vcpkg/installed/x64-linux/lib/*.so* /usr/local/lib/

RUN ldconfig

# shared_volume 디렉토리 (Docker 볼륨으로 마운트됨)
RUN mkdir -p /shared_volume/uploads /shared_volume/processed /shared_volume/results

EXPOSE 8081

# ADR-028: PREPROCESS_WORKERS=2 (c5.large vCPU 2 기준), 환경변수로 런타임 튜닝 가능
ENV PREPROCESS_WORKERS=2

CMD ["./preprocess_server"]
