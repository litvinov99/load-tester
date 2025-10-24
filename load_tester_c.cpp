/// @file load_tester_c.cpp
/// @brief Реализация C-интерфейса для нагрузочного тестирования

#include "load_tester_c.h"
#include "load_tester.hpp"
#include <cstring>

#ifdef __cplusplus
extern "C" {
#endif

// === Реализация функций ===

LoadTesterPtr create_tester() {
    return new LoadTester();
}

LoadTesterPtr create_tester_with_url(const char* url) {
    return new LoadTester(std::string(url));
}

void destroy_tester(LoadTesterPtr tester) {
    delete static_cast<LoadTester*>(tester);
}

TestDataConfigPtr create_test_data_config() {
    return new TestDataConfig();
}

void destroy_test_data_config(TestDataConfigPtr config) {
    delete static_cast<TestDataConfig*>(config);
}

void set_target_url(LoadTesterPtr tester, const char* url) {
    LoadTester* t = static_cast<LoadTester*>(tester);
    t->setTargetUrl(std::string(url));
}

void set_test_data_config(LoadTesterPtr tester, TestDataConfigPtr config) {
    LoadTester* t = static_cast<LoadTester*>(tester);
    TestDataConfig* c = static_cast<TestDataConfig*>(config);
    t->setTestDataConfig(*c);
}

void add_field_to_config(TestDataConfigPtr config, const char* field_name, 
                        const char* value, int is_random, int min_val, int max_val) {
    TestDataConfig* c = static_cast<TestDataConfig*>(config);
    FieldConfig field_config;
    field_config.value = value ? std::string(value) : "";
    field_config.is_random = (is_random != 0);
    field_config.min_val = min_val;
    field_config.max_val = max_val;
    (*c)[std::string(field_name)] = field_config;
}

void add_response_check(LoadTesterPtr tester, const char* field_path, 
                       const char* expected_value, int check_exists) {
    LoadTester* t = static_cast<LoadTester*>(tester);
    t->addResponseCheck(std::string(field_path), 
                       expected_value ? std::string(expected_value) : "",
                       check_exists != 0);
}

void clear_response_checks(LoadTesterPtr tester) {
    LoadTester* t = static_cast<LoadTester*>(tester);
    t->clearResponseChecks();
}

void run_test(LoadTesterPtr tester, int num_threads, int duration_seconds, int requests_per_second) {
    LoadTester* t = static_cast<LoadTester*>(tester);
    t->runTest(num_threads, duration_seconds, requests_per_second);
}

#ifdef __cplusplus
}
#endif