interface CacheEntry<T> {
  value: T;
  expiresAt: number;
}

interface CacheOptions {
  maxSize: number;
  ttlSeconds: number;
}

/**
 * LRU + TTL 인메모리 캐시.
 * Map은 삽입 순서를 보장하므로 LRU 구현에 활용한다.
 */
export class CacheService<T> {
  private readonly store: Map<string, CacheEntry<T>>;
  private readonly maxSize: number;
  private readonly ttlMs: number;

  constructor({ maxSize, ttlSeconds }: CacheOptions) {
    this.store = new Map();
    this.maxSize = maxSize;
    this.ttlMs = ttlSeconds * 1000;
  }

  get(key: string): T | undefined {
    const entry = this.store.get(key);
    if (!entry) return undefined;

    if (Date.now() >= entry.expiresAt) {
      this.store.delete(key);
      return undefined;
    }

    // LRU: 접근 시 최신 위치로 이동
    this.store.delete(key);
    this.store.set(key, entry);
    return entry.value;
  }

  set(key: string, value: T): void {
    // 이미 존재하면 제거 후 재삽입(순서 갱신)
    if (this.store.has(key)) {
      this.store.delete(key);
    } else if (this.store.size >= this.maxSize) {
      // 가장 오래된 항목(Map 첫 번째 키) 제거
      const oldestKey = this.store.keys().next().value;
      if (oldestKey !== undefined) {
        this.store.delete(oldestKey);
      }
    }

    this.store.set(key, { value, expiresAt: Date.now() + this.ttlMs });
  }

  get size(): number {
    return this.store.size;
  }

  clear(): void {
    this.store.clear();
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// 분석 결과 캐시 싱글턴 (analysisService.ts에서 import)
// ─────────────────────────────────────────────────────────────────────────────
const TTL = parseInt(process.env.CACHE_TTL_SECONDS ?? '3600', 10);
const MAX = parseInt(process.env.CACHE_MAX_SIZE ?? '100', 10);

export const analysisCache = new CacheService<unknown>({ maxSize: MAX, ttlSeconds: TTL });
