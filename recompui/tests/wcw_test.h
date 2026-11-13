//
// RecompFrontend
//

// [wcw] Minimal unit test harness for the wcw tests: TEST_CASE registers a test, CHECK / CHECK_NEAR record failures
// without stopping the test, and runAll() runs everything and returns the process exit code.

#pragma once

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace WcwTest {
    struct Case {
        const char *name;
        std::function<void()> body;
    };

    inline std::vector<Case> &cases() {
        static std::vector<Case> list;
        return list;
    }

    inline int &failures() {
        static int count = 0;
        return count;
    }

    struct Registrar {
        Registrar(const char *name, std::function<void()> body) {
            cases().push_back({ name, std::move(body) });
        }
    };

    inline void fail(const char *file, int line, const std::string &message) {
        fprintf(stderr, "  FAILED %s:%d: %s\n", file, line, message.c_str());
        failures()++;
    }

    inline int runAll(const char *suite) {
        int failedCases = 0;
        for (const Case &c : cases()) {
            const int before = failures();
            c.body();
            if (failures() != before) {
                fprintf(stderr, "[%s] FAIL %s\n", suite, c.name);
                failedCases++;
            }
        }

        fprintf(stderr, "[%s] %zu tests, %d failed\n", suite, cases().size(), failedCases);
        return (failedCases == 0) ? 0 : 1;
    }
};

#define WCW_TEST_CONCAT2(a, b) a##b
#define WCW_TEST_CONCAT(a, b) WCW_TEST_CONCAT2(a, b)
#define TEST_CASE(name) \
    static void WCW_TEST_CONCAT(wcwTest_, __LINE__)(); \
    static WcwTest::Registrar WCW_TEST_CONCAT(wcwTestRegistrar_, __LINE__)(name, WCW_TEST_CONCAT(wcwTest_, __LINE__)); \
    static void WCW_TEST_CONCAT(wcwTest_, __LINE__)()

#define CHECK(expr) \
    do { if (!(expr)) { WcwTest::fail(__FILE__, __LINE__, #expr); } } while (0)

#define CHECK_EQ(a, b) \
    do { const auto wcwA = (a); const auto wcwB = (b); if (!(wcwA == wcwB)) { \
        WcwTest::fail(__FILE__, __LINE__, std::string(#a " == " #b " (") + std::to_string(wcwA) + " vs " + std::to_string(wcwB) + ")"); } } while (0)

#define CHECK_NEAR(a, b, eps) \
    do { const double wcwA = double(a); const double wcwB = double(b); if (!(std::fabs(wcwA - wcwB) <= double(eps))) { \
        WcwTest::fail(__FILE__, __LINE__, std::string(#a " ~= " #b " (") + std::to_string(wcwA) + " vs " + std::to_string(wcwB) + ")"); } } while (0)
