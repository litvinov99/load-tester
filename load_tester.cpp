/// @file load_tester.сзз
/// @brief Файл с реализацией для утилиты нагрузочного тестирования

#include "load_tester.hpp"

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

LoadTester::LoadTester() : 
    requests_sent(0), 
    requests_failed(0), 
    success_responses(0), 
    error_responses(0), 
    total_response_time(0),
    target_url(),
    response_checks() {
}

LoadTester::LoadTester(const std::string& url) : 
    requests_sent(0), 
    requests_failed(0), 
    success_responses(0), 
    error_responses(0), 
    total_response_time(0),
    target_url(url),
    gen(rd()),
    response_checks() {
}

size_t LoadTester::writeCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t total_size = size * nmemb;
    response->append((char*)contents, total_size);
    return total_size;
}

void LoadTester::setTestDataConfig(const TestDataConfig& config) {
    data_config = config;
}

void LoadTester::setField(const std::string& field_name, const std::string& value, bool is_random) {
    data_config[field_name] = {value, is_random};
}

std::string LoadTester::generateFieldValue(const FieldConfig& config) {
    if (config.is_random) {
        std::uniform_int_distribution<> dis(config.min_val, config.max_val);
        return std::to_string(dis(gen));
    }
    return config.value;
}

json LoadTester::generateTestData() {
    json result;
    
    for (const auto& [field_name, config] : data_config) {
        result[field_name] = generateFieldValue(config);
    }
    
    return result;
}

void LoadTester::addResponseCheck(const std::string& field_path, const std::string& expected_value, bool check_exists) {
    response_checks.push_back({field_path, expected_value, check_exists});
}

void LoadTester::setResponseChecks(const std::vector<ResponseCheckConfig>& checks) {
    response_checks = checks;
}

void LoadTester::clearResponseChecks() {
    if (!response_checks.empty()) {
        response_checks.clear();
    }
}

bool LoadTester::checkResponseSuccess(const std::string& response_json) {
    if (response_checks.empty()) {
        // Без проверок считаем любой валидный JSON успешным
        try {
            json response_data = json::parse(response_json);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error parsing response: " << e.what() << std::endl;
            return false;
        }
    }
    
    try {
        json response_data = json::parse(response_json);
        
        for (const auto& check : response_checks) {
            // Простой парсинг пути (можно улучшить для вложенных полей)
            if (!response_data.contains(check.field_path)) {
                if (check.check_exists) {
                    return false; // Поле должно существовать, но его нет
                }
                continue;
            }
            
            // Если указано ожидаемое значение - проверяем его
            if (!check.expected_value.empty()) {
                std::string actual_value = response_data[check.field_path].dump();
                if (actual_value != check.expected_value) {
                    return false; // Значение не совпадает
                }
            }
        }
        
        return true; // Все проверки пройдены
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing response: " << e.what() << std::endl;
        return false;
    }
}

void LoadTester::setTargetUrl(const std::string& url) {
    target_url = url;
}

bool LoadTester::sendRequest(int thread_id, int request_id) {
    CURL* curl = curl_easy_init();
    std::string response_string;
    bool request_success = false;
    
    if (curl) {
        auto start_time = std::chrono::steady_clock::now();
        
        try {
            json request_data = generateTestData();
            std::string post_data = request_data.dump();
            
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, "Accept: application/json");
            
            curl_easy_setopt(curl, CURLOPT_URL, target_url.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            
            CURLcode res = curl_easy_perform(curl);
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            total_response_time += duration.count();
            
            if (res == CURLE_OK) {
                long response_code;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
                
                if (response_code == 200) {
                    requests_sent++;
                    request_success = true;
                    
                    if (checkResponseSuccess(response_string)) {
                        success_responses++;
                        if (request_id % 100 == 0) {
                            std::cout << " -> Thread " << thread_id << " - Request " << request_id 
                                      << " SUCCESS - Response time: " << duration.count() << "ms" << std::endl;
                        }
                    } else {
                        error_responses++;
                        std::cerr << "Thread " << thread_id << " - Request " << request_id 
                                  << " FAILED: success field is false" << std::endl;
                    }
                } else {
                    requests_failed++;
                    std::cerr << "Thread " << thread_id << " - HTTP Error: " << response_code 
                              << " - Response: " << response_string << std::endl;
                }
            } else {
                requests_failed++;
                std::cerr << "Thread " << thread_id << " - CURL Error: " << curl_easy_strerror(res) << std::endl;
            }
            
            curl_slist_free_all(headers);
            
        } catch (const std::exception& e) {
            requests_failed++;
            std::cerr << "Thread " << thread_id << " - Exception: " << e.what() << std::endl;
        }
        
        curl_easy_cleanup(curl);
    } else {
        requests_failed++;
        std::cerr << "Failed to initialize CURL" << std::endl;
    }
    
    return request_success;
}

void LoadTester::printTestHeader(int num_threads, int duration_seconds, int requests_per_second) const {
    std::cout << "=== C++ Load Tester ===" << std::endl;
    std::cout << "Target: " << target_url << std::endl;
    std::cout << "Threads: " << num_threads << std::endl;
    std::cout << "Duration: " << duration_seconds << " seconds" << std::endl;
    std::cout << "Target RPS: " << (requests_per_second > 0 ? std::to_string(requests_per_second) : "MAX") << std::endl;
    std::cout << "========================\n" << std::endl;
}

void LoadTester::runTest(int num_threads, int duration_seconds, int requests_per_second) {
    // Выводим шапку с настройками теста
    printTestHeader(num_threads, duration_seconds, requests_per_second);

    std::vector<std::thread> threads;
    auto start_time = std::chrono::steady_clock::now();
    std::atomic<int> global_request_id{0};
    
    std::cout << "Starting load test with " << num_threads << " threads for " 
              << duration_seconds << " seconds" << std::endl;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, start_time, duration_seconds, requests_per_second, &global_request_id, num_threads]() {
            while (std::chrono::steady_clock::now() - start_time < 
                  std::chrono::seconds(duration_seconds)) {
                
                int request_id = global_request_id++;
                this->sendRequest(i, request_id);
                
                if (requests_per_second > 0 && num_threads > 0) {
                    int rps_per_thread = requests_per_second / num_threads;
                    if (rps_per_thread > 0) {
                        int delay_ms = 1000 / rps_per_thread;
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                    }
                }
            }
        });
    }
    
    auto progress_thread = std::thread([this, start_time, duration_seconds]() {
        while (std::chrono::steady_clock::now() - start_time < 
              std::chrono::seconds(duration_seconds)) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start_time);
            int progress = (elapsed.count() * 100) / duration_seconds;
            
            std::cout << "\rProgress: " << progress << "% | "
                      << "Requests: " << requests_sent << " | "
                      << "Success: " << success_responses << " | "
                      << "Errors: " << error_responses;
            std::cout.flush();
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    progress_thread.join();
    
    printResults();
}

void LoadTester::printResults() {
    std::cout << "\n\n=== Load Test Results ===" << std::endl;
    std::cout << "Total requests sent: " << requests_sent << std::endl;
    std::cout << "Successful responses: " << success_responses << std::endl;
    std::cout << "Error responses: " << error_responses << std::endl;
    std::cout << "Failed requests: " << requests_failed << std::endl;
    
    if (requests_sent > 0) {
        double success_rate = (success_responses * 100.0) / requests_sent;
        double error_rate = (error_responses * 100.0) / requests_sent;
        double avg_response_time = static_cast<double>(total_response_time) / requests_sent;
        double rps = requests_sent / (static_cast<double>(total_response_time) / 1000.0);
        
        std::cout << "Success rate: " << success_rate << "%" << std::endl;
        std::cout << "Error rate: " << error_rate << "%" << std::endl;
        std::cout << "Average response time: " << avg_response_time << " ms" << std::endl;
        std::cout << "Requests per second: " << rps << std::endl;
    }
}