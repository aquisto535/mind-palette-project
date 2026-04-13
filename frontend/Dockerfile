# ─────────────────────────────────────────────────────────────────────────────
# Stage 1: Builder — Vite 빌드
# ─────────────────────────────────────────────────────────────────────────────
FROM node:20-alpine AS builder

WORKDIR /app

COPY package*.json ./
RUN npm ci

COPY . .

# Vite 빌드 타임 환경 변수 (build arg → ENV → Vite 인라인)
# .dockerignore가 .env.local을 제외하므로 이 ARG가 유일한 주입 경로입니다.
# 기본값은 반드시 프로덕션 안전값(false)으로 유지하세요.
ARG VITE_USE_MOCK=false
ENV VITE_USE_MOCK=$VITE_USE_MOCK

ARG VITE_API_URL=/api
ENV VITE_API_URL=$VITE_API_URL

RUN npm run build

# ─────────────────────────────────────────────────────────────────────────────
# Stage 2: Runtime — nginx:alpine 정적 서빙
# ─────────────────────────────────────────────────────────────────────────────
FROM nginx:alpine AS runtime

COPY --from=builder /app/dist /usr/share/nginx/html

# 기본 default.conf 제거 후 SPA 라우팅용 설정 주입
RUN rm /etc/nginx/conf.d/default.conf
COPY nginx-spa.conf /etc/nginx/conf.d/default.conf

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]
