import logger from '../src/utils/logger';

console.log('--- Testing with string requestId ---');
logger.info('Test message with string requestId', { requestId: 'test-uuid-1234' });

console.log('\n--- Testing with object requestId ---');
logger.info('Test message with object requestId', { requestId: { id: 'test-uuid-5678', type: 'internal' } });

console.log('\n--- Testing with no requestId ---');
logger.info('Test message with no requestId', { other: 'meta' });
