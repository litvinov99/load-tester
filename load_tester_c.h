/// @file load_tester_c.h
/// @brief C-интерфейс для утилиты нагрузочного тестирования

#ifndef LOAD_TESTER_C_H
#define LOAD_TESTER_C_H

#ifdef __cplusplus
extern "C" {
#endif

// Тип указателя на LoadTester (скрывает C++ реализацию)
typedef void* LoadTesterPtr;

// Тип указателя на FieldConfig
typedef void* FieldConfigPtr;

// Тип указателя на TestDataConfig  
typedef void* TestDataConfigPtr;

// === Создание и уничтожение объектов ===

/// @brief Создает новый экземпляр LoadTester
/// @return Указатель на LoadTester
LoadTesterPtr create_tester();

/// @brief Создает новый экземпляр LoadTester с URL
/// @param url Целевой URL
/// @return Указатель на LoadTester
LoadTesterPtr create_tester_with_url(const char* url);

/// @brief Уничтожает экземпляр LoadTester
/// @param tester Указатель на LoadTester
void destroy_tester(LoadTesterPtr tester);

/// @brief Создает конфигурацию тестовых данных
/// @return Указатель на TestDataConfig
TestDataConfigPtr create_test_data_config();

/// @brief Уничтожает конфигурацию тестовых данных
/// @param config Указатель на TestDataConfig
void destroy_test_data_config(TestDataConfigPtr config);

// === Настройка тестера ===

/// @brief Устанавливает целевой URL
/// @param tester Указатель на LoadTester
/// @param url Целевой URL
void set_target_url(LoadTesterPtr tester, const char* url);

/// @brief Устанавливает конфигурацию тестовых данных
/// @param tester Указатель на LoadTester
/// @param config Указатель на TestDataConfig
void set_test_data_config(LoadTesterPtr tester, TestDataConfigPtr config);

/// @brief Добавляет поле в конфигурацию тестовых данных
/// @param config Указатель на TestDataConfig
/// @param field_name Имя поля
/// @param value Значение поля
/// @param is_random Флаг случайного значения (0=false, 1=true)
/// @param min_val Минимальное значение для случайных чисел
/// @param max_val Максимальное значение для случайных чисел
void add_field_to_config(TestDataConfigPtr config, const char* field_name, 
                        const char* value, int is_random, int min_val, int max_val);

/// @brief Добавляет проверку ответа
/// @param tester Указатель на LoadTester
/// @param field_path Путь к полю в JSON
/// @param expected_value Ожидаемое значение
/// @param check_exists Проверять существование (0=false, 1=true)
void add_response_check(LoadTesterPtr tester, const char* field_path, 
                       const char* expected_value, int check_exists);

/// @brief Очищает все проверки ответа
/// @param tester Указатель на LoadTester
void clear_response_checks(LoadTesterPtr tester);

// === Запуск теста ===

/// @brief Запускает нагрузочный тест
/// @param tester Указатель на LoadTester
/// @param num_threads Количество потоков
/// @param duration_seconds Длительность теста в секундах
/// @param requests_per_second Запросов в секунду (0 = максимальная скорость)
void run_test(LoadTesterPtr tester, int num_threads, int duration_seconds, 
              int requests_per_second);

#ifdef __cplusplus
}
#endif

#endif // LOAD_TESTER_C_H