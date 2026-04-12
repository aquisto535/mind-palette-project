import { CacheService } from '../src/services/cacheService';

describe('CacheService', () => {
  let cache: CacheService<string>;

  beforeEach(() => {
    cache = new CacheService({ maxSize: 3, ttlSeconds: 60 });
  });

  describe('캐시 미스', () => {
    it('존재하지 않는 키는 undefined를 반환한다', () => {
      expect(cache.get('nonexistent')).toBeUndefined();
    });
  });

  describe('캐시 히트', () => {
    it('저장한 값을 즉시 반환한다', () => {
      cache.set('key1', 'value1');
      expect(cache.get('key1')).toBe('value1');
    });

    it('동일 키로 덮어쓰면 새 값을 반환한다', () => {
      cache.set('key1', 'old');
      cache.set('key1', 'new');
      expect(cache.get('key1')).toBe('new');
    });
  });

  describe('TTL 만료', () => {
    it('TTL이 지난 항목은 undefined를 반환한다', () => {
      const shortCache = new CacheService<string>({ maxSize: 10, ttlSeconds: 0 });
      shortCache.set('key', 'value');
      // TTL=0 → 저장 즉시 만료
      expect(shortCache.get('key')).toBeUndefined();
    });

    it('TTL이 남은 항목은 정상 반환한다', () => {
      const longCache = new CacheService<string>({ maxSize: 10, ttlSeconds: 3600 });
      longCache.set('key', 'alive');
      expect(longCache.get('key')).toBe('alive');
    });
  });

  describe('LRU 정책 (maxSize 초과)', () => {
    it('maxSize를 초과하면 가장 오래된 항목을 제거한다', () => {
      cache.set('a', '1');
      cache.set('b', '2');
      cache.set('c', '3');
      cache.set('d', '4'); // 'a'가 제거됨
      expect(cache.get('a')).toBeUndefined();
      expect(cache.get('b')).toBe('2');
      expect(cache.get('c')).toBe('3');
      expect(cache.get('d')).toBe('4');
    });

    it('get으로 접근한 항목은 LRU에서 최신으로 이동한다', () => {
      cache.set('a', '1');
      cache.set('b', '2');
      cache.set('c', '3');
      cache.get('a');         // 'a'를 최신으로 갱신
      cache.set('d', '4');   // 가장 오래된 'b'가 제거됨
      expect(cache.get('b')).toBeUndefined();
      expect(cache.get('a')).toBe('1');
    });
  });

  describe('size', () => {
    it('초기 사이즈는 0이다', () => {
      expect(cache.size).toBe(0);
    });

    it('항목 추가 시 사이즈가 증가한다', () => {
      cache.set('k1', 'v');
      cache.set('k2', 'v');
      expect(cache.size).toBe(2);
    });

    it('maxSize를 초과해도 사이즈는 maxSize를 유지한다', () => {
      cache.set('a', '1');
      cache.set('b', '2');
      cache.set('c', '3');
      cache.set('d', '4');
      expect(cache.size).toBe(3);
    });
  });

  describe('clear', () => {
    it('clear 후 모든 항목이 제거된다', () => {
      cache.set('a', '1');
      cache.set('b', '2');
      cache.clear();
      expect(cache.get('a')).toBeUndefined();
      expect(cache.size).toBe(0);
    });
  });
});
