#pragma once
#include "crow.h"

inline void setup_routes(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/")([](){
        return "Preprocess Server is running!";
    });

    CROW_ROUTE(app, "/health")([](){
        return crow::response(200, "OK");
    });
}
