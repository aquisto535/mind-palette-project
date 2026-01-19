const request = require('supertest');
const app = require('../server');

describe('Health Check API', () => {
  describe('GET /health', () => {
    it('should return 200 OK with health status', async () => {
      const response = await request(app).get('/health');

      expect(response.status).toBe(200);
      expect(response.body).toHaveProperty('status', 'healthy');
      expect(response.body).toHaveProperty('timestamp');
      expect(response.body).toHaveProperty('uptime');
    });

    it('should return server uptime information', async () => {
      const response = await request(app).get('/health');

      expect(response.body.uptime).toBeDefined();
      expect(typeof response.body.uptime).toBe('string');
      expect(response.body.uptime).toMatch(/\d+/); // 숫자 포함
    });

    it('should return memory usage information', async () => {
      const response = await request(app).get('/health');

      expect(response.body).toHaveProperty('memory');
      expect(response.body.memory).toHaveProperty('used');
      expect(response.body.memory).toHaveProperty('total');
      expect(response.body.memory.used).toMatch(/MB$/);
      expect(response.body.memory.total).toMatch(/MB$/);
    });

    it('should return disk space information', async () => {
      const response = await request(app).get('/health');

      expect(response.body).toHaveProperty('disk');
      expect(response.body.disk).toHaveProperty('available');
      expect(response.body.disk.available).toMatch(/GB$/);
    });

    it('should have valid timestamp in ISO format', async () => {
      const response = await request(app).get('/health');

      const timestamp = new Date(response.body.timestamp);
      expect(timestamp).toBeInstanceOf(Date);
      expect(timestamp.getTime()).not.toBeNaN();
    });
  });
});
