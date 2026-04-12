# ─────────────────────────────────────────────────────────────────────────────
# Stage 1: Builder — TypeScript 컴파일
# ─────────────────────────────────────────────────────────────────────────────
FROM node:20-alpine AS builder

WORKDIR /app

COPY package*.json ./
RUN npm ci

COPY tsconfig*.json ./
COPY src ./src

RUN npm run build

# ─────────────────────────────────────────────────────────────────────────────
# Stage 2: Runtime — 프로덕션 의존성만 포함
# ─────────────────────────────────────────────────────────────────────────────
FROM node:20-alpine AS runtime
RUN apk add --no-cache curl

WORKDIR /app

COPY package*.json ./
RUN npm ci --omit=dev

COPY --from=builder /app/dist ./dist

# shared_volume 디렉토리 (Docker 볼륨으로 마운트됨)
# fileStorage.ts: SHARED_ROOT = /app/../../../shared_volume = /shared_volume
RUN mkdir -p /shared_volume/uploads /shared_volume/processed /shared_volume/results

EXPOSE 3000

ENV NODE_ENV=production

CMD ["node", "dist/server.js"]
