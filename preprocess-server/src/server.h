#pragma once
#include "crow.h"

inline void setup_routes(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/")([](){
        return "Preprocess Server is running!";
    });

    CROW_ROUTE(app, "/health")([](){
        return crow::response(200, "OK");
    });

    CROW_ROUTE(app, "/preprocess").methods(crow::HTTPMethod::POST)([](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        if (!body.has("imagePath")) {
            return crow::response(400, "Missing imagePath");
        }
        
        // TODO: 실제 전처리 로직 구현 (Week 2-4)
        // 현재는 통신 계약 테스트를 위한 더미 응답 반환
        // 입력: /shared/uploads/test.jpg -> 출력: /shared/processed/test_clean.jpg
        
        crow::json::wvalue res;
        res["processedPath"] = "/shared/processed/test_clean.jpg";
        
        return crow::response(200, res);
    });
}
