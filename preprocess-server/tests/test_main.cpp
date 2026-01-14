#include <gtest/gtest.h>
#include "crow.h"
#include "server.h"

TEST(ServerTest, RootRoute) {
    crow::SimpleApp app;
    setup_routes(app);
    app.validate();

    crow::request req;
    req.url = "/";
    
    crow::response res;
    app.handle_full(req, res);
    
    EXPECT_EQ(res.code, 200);
    EXPECT_EQ(res.body, "Preprocess Server is running!");
}

TEST(ServerTest, HealthCheck) {
    crow::SimpleApp app;
    setup_routes(app);
    app.validate();

    crow::request req;
    req.url = "/health";
    
    crow::response res;
    app.handle_full(req, res);
    
    EXPECT_EQ(res.code, 200);
    EXPECT_EQ(res.body, "OK");
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
