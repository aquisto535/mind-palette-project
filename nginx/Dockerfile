FROM nginx:alpine

COPY nginx.conf /etc/nginx/nginx.conf

# SSL 인증서는 볼륨으로 마운트 — 이미지에 포함 안 함
# docker-compose.yml: volumes: ./nginx/ssl:/etc/nginx/ssl:ro

EXPOSE 80 443
