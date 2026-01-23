#include "crow.h"
#include "server.h"
#include "utils/Logger.h"

int main() {
    Logger::init();
    LOG_INFO("Preprocess Server starting...");
    
    crow::SimpleApp app;
    setup_routes(app);
    app.port(8081).multithreaded().run();
}
