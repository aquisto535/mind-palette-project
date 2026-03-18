/**
 * [TDD Red] API 명세 일치 검증 테스트
 *
 * OpenAPI 3.0 스펙 파일(openapi.yaml)이 존재하고,
 * 실제 엔드포인트의 응답 구조와 일치하는지 검증한다.
 */
import fs from 'node:fs';
import path from 'node:path';
import yaml from 'js-yaml';
import request from 'supertest';
import app from '../src/server';

const SPEC_PATH = path.resolve(__dirname, '../../docs/api/openapi.yaml');

/** $ref 문자열을 resolve하여 실제 스키마 객체를 반환 */
function resolveRef(spec: any, ref: string): any {
  // "#/components/schemas/Foo" 형태만 처리
  const parts = ref.replace(/^#\//, '').split('/');
  return parts.reduce((obj: any, key: string) => obj?.[key], spec);
}

/** 응답 스키마를 가져오되, $ref면 resolve */
function getResponseSchema(spec: any, path: string, method: string, statusCode: string): any {
  const responseContent =
    spec.paths[path]?.[method]?.responses?.[statusCode]?.content?.['application/json'];
  if (!responseContent) return undefined;
  if (responseContent.schema?.$ref) {
    return resolveRef(spec, responseContent.schema.$ref);
  }
  return responseContent.schema;
}

describe('OpenAPI Spec 파일 존재 및 구조 검증', () => {
  let spec: any;

  beforeAll(() => {
    const raw = fs.readFileSync(SPEC_PATH, 'utf-8');
    spec = yaml.load(raw);
  });

  it('openapi.yaml 파일이 존재해야 한다', () => {
    expect(fs.existsSync(SPEC_PATH)).toBe(true);
  });

  it('OpenAPI 3.0 버전 형식이어야 한다', () => {
    expect(spec.openapi).toMatch(/^3\./);
  });

  it('info 필드(title, version)가 있어야 한다', () => {
    expect(spec.info).toBeDefined();
    expect(typeof spec.info.title).toBe('string');
    expect(typeof spec.info.version).toBe('string');
  });

  it('paths에 GET /, GET /health, POST /analyze가 정의되어야 한다', () => {
    expect(spec.paths['/']).toBeDefined();
    expect(spec.paths['/health']).toBeDefined();
    expect(spec.paths['/analyze']).toBeDefined();

    expect(spec.paths['/']['get']).toBeDefined();
    expect(spec.paths['/health']['get']).toBeDefined();
    expect(spec.paths['/analyze']['post']).toBeDefined();
  });
});

describe('GET / — 명세 vs 실제 응답 일치 검증', () => {
  it('200 응답을 반환해야 한다', async () => {
    const res = await request(app).get('/');
    expect(res.status).toBe(200);
  });
});

describe('GET /health — 명세 vs 실제 응답 일치 검증', () => {
  let spec: any;

  beforeAll(() => {
    const raw = fs.readFileSync(SPEC_PATH, 'utf-8');
    spec = yaml.load(raw);
  });

  it('200 응답을 반환해야 한다', async () => {
    const res = await request(app).get('/health');
    expect(res.status).toBe(200);
  });

  it('응답에 status, timestamp, uptime, memory, disk 필드가 있어야 한다', async () => {
    const res = await request(app).get('/health');
    expect(res.body).toHaveProperty('status');
    expect(res.body).toHaveProperty('timestamp');
    expect(res.body).toHaveProperty('uptime');
    expect(res.body).toHaveProperty('memory');
    expect(res.body).toHaveProperty('disk');
  });

  it('명세의 /health GET 응답 스키마 필드가 실제 응답과 일치해야 한다', async () => {
    const res = await request(app).get('/health');
    const schema = getResponseSchema(spec, '/health', 'get', '200');
    const schemaProps: string[] = Object.keys(schema.properties);
    schemaProps.forEach((field) => {
      expect(res.body).toHaveProperty(field);
    });
  });
});

describe('POST /analyze — 명세 vs 실제 응답 일치 검증', () => {
  let spec: any;

  beforeAll(() => {
    const raw = fs.readFileSync(SPEC_PATH, 'utf-8');
    spec = yaml.load(raw);
  });

  it('파일 없이 요청 시 400을 반환해야 한다', async () => {
    const res = await request(app).post('/analyze');
    expect(res.status).toBe(400);
    expect(res.body).toHaveProperty('error');
  });

  it('명세의 400 에러 응답에 error 필드가 정의되어 있어야 한다', () => {
    const errorSchema = getResponseSchema(spec, '/analyze', 'post', '400');
    expect(errorSchema.properties).toHaveProperty('error');
  });

  it('명세의 POST /analyze 성공 응답 스키마에 score, percentile, date, interpretation, details 필드가 있어야 한다', () => {
    const successSchema = getResponseSchema(spec, '/analyze', 'post', '200');
    const props = Object.keys(successSchema.properties);
    expect(props).toContain('score');
    expect(props).toContain('percentile');
    expect(props).toContain('date');
    expect(props).toContain('interpretation');
    expect(props).toContain('details');
  });

  it('명세의 POST /analyze 요청 body가 multipart/form-data 이어야 한다', () => {
    const requestBody = spec.paths['/analyze'].post.requestBody;
    expect(requestBody.content).toHaveProperty('multipart/form-data');
    expect(requestBody.content['multipart/form-data'].schema.properties).toHaveProperty('image');
  });
});
