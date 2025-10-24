/// @file load_tester.hpp
/// @brief Заголовочный файл для утилиты нагрузочного тестирования

#ifndef LOAD_TESTER_HPP
#define LOAD_TESTER_HPP

#include <string>
#include <atomic>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <random>

// Предварительное объявление для CURL
typedef void CURL;

/**
 * @struct FieldConfig
 * @brief Структура поля тестового запроса
 */
struct FieldConfig {
    std::string value;        //< Фиксированное значение или шаблон
    bool is_random = false;   //< Флаг случайного значения
    int min_val = 0;          //< Минимальное значение для случайных чисел
    int max_val = 9999;       //< Максимальное значение для случайных чисел
};

/**
 * @struct ResponseCheckConfig
 * @brief Структура поля ответа на входной запрос
 */
struct ResponseCheckConfig {
    std::string field_path;         //< Путь к полю (например: "success", "data.status")
    std::string expected_value;     //< Ожидаемое значение
    bool check_exists = true;       //< Проверять существование поля
};

using TestDataConfig = std::unordered_map<std::string, FieldConfig>;

/**
 * @class LoadTester
 * @brief Класс для проведения нагрузочного тестирования HTTP-сервисов
 */
class LoadTester {
public:
    /// @brief Конструктор по умолчанию
    LoadTester();

    /// @brief Конструктор принимающий target_url
    LoadTester(const std::string& url);

    /// @brief Устанавливает целевой URL для тестирования
    void setTargetUrl(const std::string& url);

    /// @brief Устанавливает конфигурацию тестовых данных
    void setTestDataConfig(const TestDataConfig& config);

    /// @brief Устанавливает конкретное поле
    void setField(const std::string& field_name, const std::string& value, bool is_random = false);

    /// @brief Добавляет проверку ответа от сервера
    void addResponseCheck(const std::string& field_path, const std::string& expected_value = "", bool check_exists = true);

    /// @brief Устанавливает список проверок ответа от сервера
    void setResponseChecks(const std::vector<ResponseCheckConfig>& checks);

    /// @brief Очищает все проверки ответа от сервера
    void clearResponseChecks();

    /// @brief Отправляет один HTTP-запрос на целевой сервер
    bool sendRequest(int thread_id, int request_id);

    /// @brief Запускает нагрузочный тест с указанными параметрами
    void runTest(int num_threads, int duration_seconds, int requests_per_second = 0);

    /// @brief Выводит итоговую статистику тестирования
    void printResults();

private:
    std::atomic<long> requests_sent;            //< Общее количество отправленных запросов
    std::atomic<long> requests_failed;          //< Количество неудачных запросов
    std::atomic<long> success_responses;        //< Количество успешных ответов
    std::atomic<long> error_responses;          //< Количество ответов с ошибкой
    std::atomic<long> total_response_time;      //< Суммарное время ответов

    std::string target_url;                     //< Целевой URL для тестирования

    TestDataConfig data_config;                 //< Конфигурация тестовых данных
    std::random_device rd;                      //< Генератор случайных чисел
    std::mt19937 gen;                           //< Генератор Mersenne Twister

    std::vector<ResponseCheckConfig> response_checks; //< Конфигурация проверок ответа

    /// @brief Выводит информацию о настройках теста
    void printTestHeader(int num_threads, int duration_seconds, int requests_per_second) const;

    /// @brief Callback-функция для записи ответа от сервера
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* response);

    /// @brief Генерирует значение для поля
    std::string generateFieldValue(const FieldConfig& config);

    /// @brief Генерирует тестовые данные для запроса
    nlohmann::json generateTestData();

    /// @brief Проверяет успешность ответа от сервера
    bool checkResponseSuccess(const std::string& response_json);
};

#endif // LOAD_TESTER_HPP