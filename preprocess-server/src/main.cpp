#include "crow.h"
#include "core/server.h"
#include "utils/Logger.h"

int main() {
    Logger::init();
    LOG_INFO("Preprocess Server starting...");

    crow::SimpleApp app;
    setup_routes(app); // 라우트 등록 (콜백 함수들을 URL에 매핑)
    app.port(8081).multithreaded().run(); //CPU 코어 수(Logical Cores)에 맞춰 스레드 생성
}
