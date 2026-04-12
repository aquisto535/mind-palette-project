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

# vcpkg manifest 복사 및 종속성 빌드 최적화
COPY vcpkg.json vcpkg-configuration.json ./
# vcpkg install을 미리 실행하여 종속성을 캐싱할 수 있으나, 
# 현재 CMake가 이를 수행하므로 CMake 단계 전까지만 유지

COPY . .

# Configure (vcpkg가 이 단계에서 Crow 등 의존성 설치)
# - VCPKG_TARGET_TRIPLET=x64-linux : 동적 링크 (Windows x64-windows와 동일 역할)
# - PREPROCESS_WORKERS 환경변수는 런타임에 주입 (ADR-028)
RUN cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
    -DCMAKE_BUILD_TYPE=Release \
    -G Ninja

# Crow 1.2.1 패치: HTTP 422 Unprocessable Entity 지원 추가
# Crow의 statusCodes 맵에 422가 없어 500으로 변환되는 버그 수정 (ADR-033 Tier 1)
RUN python3 -c "\
path = 'build/vcpkg_installed/x64-linux/include/crow/http_connection.h'; \
content = open(path).read(); \
needle = '              {status::PRECONDITION_REQUIRED'; \
patch = '              {422, \"HTTP/1.1 422 Unprocessable Entity\\\\r\\\\n\"},\n'; \
assert needle in content, 'Crow patch target not found — check Crow version'; \
open(path, 'w').write(content.replace(needle, patch + needle, 1)); \
print('Crow patched: HTTP 422 Unprocessable Entity added to statusCodes') \
"

# Build
RUN cmake --build build --config Release --target preprocess_server

# ─────────────────────────────────────────────────────────────────────────────
# Stage 2: Runtime — 바이너리 + vcpkg 동적 라이브러리만 포함
# ─────────────────────────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libgomp1 \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/bin/preprocess_server .
COPY --from=builder /app/build/vcpkg_installed/x64-linux/lib/*.so* /usr/local/lib/

RUN ldconfig && \
    mkdir -p /shared_volume/uploads /shared_volume/processed /shared_volume/results

EXPOSE 8081

# ADR-028: PREPROCESS_WORKERS=2 (c5.large vCPU 2 기준), 환경변수로 런타임 튜닝 가능
ENV PREPROCESS_WORKERS=2

CMD ["./preprocess_server"]
