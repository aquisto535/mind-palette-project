const axios = require('axios');
const fs = require('fs');
const path = require('path');
const FormData = require('form-data');

const GATEWAY_URL = 'http://localhost:3000';
const TEST_DIR = path.join(__dirname, '../shared_volume/uploads');
const TEST_IMAGE_PATH = path.join(TEST_DIR, 'test_input.jpg');

async function runTest() {
    console.log('🚀 Starting Integration Logging & Health Test...');

    // 0. AI Server Health Check 확인
    try {
        console.log('🏥 Checking AI Server Health...');
        const healthRes = await axios.get(`${GATEWAY_URL.replace('3000', '8002')}/health`);
        console.log('✅ AI Server Status:', JSON.stringify(healthRes.data, null, 2));
    } catch (e) {
        console.warn('⚠️ AI Server Health Check failed, but proceeding with analysis test...');
    }

    if (!fs.existsSync(TEST_DIR)) {
        console.log(`📁 Creating directory: ${TEST_DIR}`);
        fs.mkdirSync(TEST_DIR, { recursive: true });
    }

    if (!fs.existsSync(TEST_IMAGE_PATH)) {
        console.log('📝 Creating test image...');
        fs.writeFileSync(TEST_IMAGE_PATH, 'dummy content');
    }

    try {
        const formData = new FormData();
        formData.append('image', fs.createReadStream(TEST_IMAGE_PATH));

        console.log('📡 Sending request to API Gateway...');
        const response = await axios.post(`${GATEWAY_URL}/analyze`, formData, {
            headers: formData.getHeaders()
        });

        const requestId = response.headers['x-request-id'];
        console.log(`✅ Request Successful!`);
        console.log(`🆔 Request ID: ${requestId}`);

        console.log('\n--- Verification ---');
        console.log(`1. Check Gateway Log: grep "${requestId}" api-gateway/logs/combined.log`);
        console.log(`2. Check C++ Log: grep "${requestId}" preprocess-server/logs/preprocess.log`);

    } catch (error) {
        console.error('❌ Request Failed');
        if (error.response) {
            console.log('Status:', error.response.status);
            console.log('ID:', error.response.headers['x-request-id']);
        } else {
            console.log('Message:', error.message);
        }
    }
}

runTest();
