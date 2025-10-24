#!/usr/bin/env python3
"""
Графический интерфейс для утилиты нагрузочного тестирования
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import ctypes
import threading
import queue
import sys
import os

# Загрузка динамической библиотеки
try:
    lib = ctypes.CDLL('./libload_tester.so')
except Exception as e:
    print(f"Ошибка загрузки библиотеки: {e}")
    sys.exit(1)

# Определение типов функций
lib.create_tester.restype = ctypes.c_void_p
lib.create_tester_with_url.restype = ctypes.c_void_p
lib.create_test_data_config.restype = ctypes.c_void_p

lib.destroy_tester.argtypes = [ctypes.c_void_p]
lib.destroy_test_data_config.argtypes = [ctypes.c_void_p]
lib.set_target_url.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
lib.set_test_data_config.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
lib.add_field_to_config.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
lib.add_response_check.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
lib.run_test.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]

class LoadTesterGUI:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Load Tester - Нагрузочное тестирование")
        self.root.geometry("1000x800")
        
        self.tester = None
        self.config = None
        self.test_thread = None
        self.log_queue = queue.Queue()
        
        # Пустые стандартные поля - пользователь сам настроит через JSON
        self.standard_fields = []
        
        self.create_widgets()
        self.setup_defaults()
        
        # Запуск обработки логов
        self.process_log_queue()
    
    def create_widgets(self):
        """Создание элементов интерфейса"""
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Настройка сетки
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(1, weight=1)
        
        # === Секция настроек теста ===
        settings_frame = ttk.LabelFrame(main_frame, text="Настройки теста", padding="5")
        settings_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=(0, 10))
        settings_frame.columnconfigure(1, weight=1)
        
        # URL
        ttk.Label(settings_frame, text="Целевой URL:").grid(row=0, column=0, sticky=tk.W, pady=2)
        self.url_entry = ttk.Entry(settings_frame, width=60)
        self.url_entry.grid(row=0, column=1, sticky=(tk.W, tk.E), pady=2, padx=(5, 0))
        self.url_entry.insert(0, "")
        
        # Количество потоков
        ttk.Label(settings_frame, text="Количество потоков:").grid(row=1, column=0, sticky=tk.W, pady=2)
        self.threads_entry = ttk.Entry(settings_frame, width=10)
        self.threads_entry.grid(row=1, column=1, sticky=tk.W, pady=2, padx=(5, 0))
        self.threads_entry.insert(0, "10")
        
        # Длительность теста
        ttk.Label(settings_frame, text="Длительность (сек):").grid(row=2, column=0, sticky=tk.W, pady=2)
        self.duration_entry = ttk.Entry(settings_frame, width=10)
        self.duration_entry.grid(row=2, column=1, sticky=tk.W, pady=2, padx=(5, 0))
        self.duration_entry.insert(0, "30")
        
        # RPS
        ttk.Label(settings_frame, text="Запросов в секунду:").grid(row=3, column=0, sticky=tk.W, pady=2)
        self.rps_entry = ttk.Entry(settings_frame, width=10)
        self.rps_entry.grid(row=3, column=1, sticky=tk.W, pady=2, padx=(5, 0))
        self.rps_entry.insert(0, "50")
        
        # === Секция JSON полей (рядом друг с другом) ===
        json_frame = ttk.LabelFrame(main_frame, text="JSON данные", padding="5")
        json_frame.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=(0, 10))
        json_frame.columnconfigure(0, weight=1)
        json_frame.columnconfigure(1, weight=1)
        
        # Левая колонка - Входной запрос
        request_frame = ttk.Frame(json_frame)
        request_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), padx=(0, 5))
        
        ttk.Label(request_frame, text="Входной запрос (JSON):").pack(anchor=tk.W)
        self.request_json_text = scrolledtext.ScrolledText(request_frame, height=12, width=45)
        self.request_json_text.pack(fill=tk.BOTH, expand=True)
        
        # Пример входного JSON
        example_request = """{
    "username": "test_user",
    "password": "test_pass",
    "action": "login"
}"""
        self.request_json_text.insert(tk.END, example_request)
        
        # Правая колонка - Ответный запрос
        response_frame = ttk.Frame(json_frame)
        response_frame.grid(row=0, column=1, sticky=(tk.W, tk.E, tk.N, tk.S), padx=(5, 0))
        
        ttk.Label(response_frame, text="Ожидаемый ответ (JSON):").pack(anchor=tk.W)
        self.response_json_text = scrolledtext.ScrolledText(response_frame, height=12, width=45)
        self.response_json_text.pack(fill=tk.BOTH, expand=True)
        
        # Пример ответного JSON
        example_response = """{
    "success": true,
    "status": "ok",
    "user_id": 12345
}"""
        self.response_json_text.insert(tk.END, example_response)
        
        # === Кнопки управления ===
        button_frame = ttk.Frame(main_frame)
        button_frame.grid(row=2, column=0, columnspan=2, pady=(0, 10))
        
        self.setup_btn = ttk.Button(button_frame, text="Обновить настройки", command=self.setup_test)
        self.setup_btn.pack(side=tk.LEFT, padx=(0, 10))
        
        self.start_btn = ttk.Button(button_frame, text="Начать тестирование", command=self.start_test)
        self.start_btn.pack(side=tk.LEFT, padx=(0, 10))
        
        self.reset_btn = ttk.Button(button_frame, text="Сбросить данные", command=self.reset_data)
        self.reset_btn.pack(side=tk.LEFT, padx=(0, 10))
        
        self.clear_btn = ttk.Button(button_frame, text="Очистить лог", command=self.clear_log)
        self.clear_btn.pack(side=tk.LEFT)
        
        # === Лог выполнения ===
        log_frame = ttk.LabelFrame(main_frame, text="Лог выполнения", padding="5")
        log_frame.grid(row=3, column=0, columnspan=2, sticky=(tk.W, tk.E, tk.N, tk.S))
        log_frame.columnconfigure(0, weight=1)
        log_frame.rowconfigure(0, weight=1)
        main_frame.rowconfigure(3, weight=1)
        
        self.log_text = scrolledtext.ScrolledText(log_frame, height=20, width=100)
        self.log_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Статус бар
        self.status_var = tk.StringVar(value="Готов к работе")
        status_bar = ttk.Label(main_frame, textvariable=self.status_var, relief=tk.SUNKEN)
        status_bar.grid(row=4, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=(5, 0))
    
    def setup_defaults(self):
        """Установка значений по умолчанию"""
        self.log("Load Tester инициализирован")
        self.log("Введите URL целевого сервера и настройте JSON данные")
    
    def setup_test(self):
        """Настройка теста"""
        try:
            # Проверяем обязательные поля
            url = self.url_entry.get().strip()
            if not url:
                messagebox.showwarning("Предупреждение", "Введите целевой URL!")
                return
            
            # Уничтожаем старые объекты
            if self.tester:
                lib.destroy_tester(self.tester)
            if self.config:
                lib.destroy_test_data_config(self.config)
            
            # Создаем новые объекты
            self.tester = lib.create_tester_with_url(url.encode('utf-8'))
            self.config = lib.create_test_data_config()
            
            # Добавляем поля из JSON запроса
            request_json = self.request_json_text.get("1.0", tk.END).strip()
            if request_json:
                try:
                    import json as py_json
                    request_data = py_json.loads(request_json)
                    for key, value in request_data.items():
                        # Определяем, является ли поле случайным
                        is_random = False
                        min_val = 0
                        max_val = 0
                        
                        # Если значение пустое или содержит ключевые слова - делаем случайным
                        if value == "" or str(value).lower() in ["random", "rand", "rnd"]:
                            is_random = True
                            min_val = 1000
                            max_val = 9999
                        
                        lib.add_field_to_config(
                            self.config,
                            key.encode('utf-8'),
                            str(value).encode('utf-8') if not is_random else b"",
                            1 if is_random else 0,
                            min_val,
                            max_val
                        )
                    self.log(f"Добавлено {len(request_data)} полей из входного запроса")
                except Exception as e:
                    self.log(f"Ошибка парсинга JSON запроса: {e}")
                    messagebox.showerror("Ошибка", f"Неверный формат JSON во входном запросе: {e}")
                    return
            
            # Настраиваем проверки ответа из JSON ответа
            response_json = self.response_json_text.get("1.0", tk.END).strip()
            if response_json:
                try:
                    import json as py_json
                    response_data = py_json.loads(response_json)
                    
                    # Очищаем старые проверки
                    # lib.clear_response_checks(self.tester)
                    
                    # Добавляем проверки для каждого поля ответа
                    for key, value in response_data.items():
                        lib.add_response_check(
                            self.tester,
                            key.encode('utf-8'),
                            str(value).encode('utf-8'),
                            1
                        )
                    self.log(f"Добавлено {len(response_data)} проверок ответа")
                except Exception as e:
                    self.log(f"Ошибка парсинга JSON ответа: {e}")
                    messagebox.showerror("Ошибка", f"Неверный формат JSON в ожидаемом ответе: {e}")
                    return
            else:
                # Проверка по умолчанию если ответ не указан
                lib.add_response_check(self.tester, b"success", b"true", 1)
                self.log("Добавлена проверка по умолчанию: success == true")
            
            # Применяем конфигурацию
            lib.set_test_data_config(self.tester, self.config)
            
            self.log("Тест успешно настроен")
            self.status_var.set("Тест настроен - готов к запуску")
            
        except Exception as e:
            self.log(f"Ошибка настройки теста: {e}")
            messagebox.showerror("Ошибка", f"Не удалось настроить тест: {e}")

    def start_test(self):
        """Запуск теста в отдельном потоке"""
        if not self.tester:
            messagebox.showwarning("Предупреждение", "Сначала настройте тест!")
            return
        
        try:
            threads = int(self.threads_entry.get())
            duration = int(self.duration_entry.get())
            rps = int(self.rps_entry.get())
            
            if threads <= 0 or duration <= 0 or rps < 0:
                messagebox.showwarning("Предупреждение", "Проверьте корректность введенных значений!")
                return
            
            # Блокируем кнопки во время теста
            self.start_btn.config(state='disabled')
            self.setup_btn.config(state='disabled')
            self.status_var.set("Тестирование запущено...")
            
            # Запускаем тест в отдельном потоке
            self.test_thread = threading.Thread(
                target=self.run_test_thread,
                args=(threads, duration, rps),
                daemon=True
            )
            self.test_thread.start()
            
            self.log(f"Запуск теста: {threads} потоков, {duration} сек, {rps} RPS")
            
        except ValueError:
            messagebox.showerror("Ошибка", "Введите корректные числовые значения!")
        except Exception as e:
            self.log(f"Ошибка запуска теста: {e}")
            messagebox.showerror("Ошибка", f"Не удалось запустить тест: {e}")
    
    def run_test_thread(self, threads, duration, rps):
        """Запуск теста в отдельном потоке"""
        try:
            lib.run_test(self.tester, threads, duration, rps)
            self.log_queue.put("Тестирование завершено")
        except Exception as e:
            self.log_queue.put(f"Ошибка во время теста: {e}")
        finally:
            self.log_queue.put("DONE")  # Сигнал завершения
    
    def reset_data(self):
        """Сброс данных к значениям по умолчанию"""
        self.url_entry.delete(0, tk.END)
        
        self.threads_entry.delete(0, tk.END)
        self.threads_entry.insert(0, "10")
        
        self.duration_entry.delete(0, tk.END)
        self.duration_entry.insert(0, "30")
        
        self.rps_entry.delete(0, tk.END)
        self.rps_entry.insert(0, "50")
        
        # Очищаем JSON поля
        self.request_json_text.delete("1.0", tk.END)
        self.request_json_text.insert(tk.END, '{\n    "username": "test_user",\n    "action": "test"\n}')
        
        self.response_json_text.delete("1.0", tk.END)
        self.response_json_text.insert(tk.END, '{\n    "success": true,\n    "status": "ok"\n}')
        
        self.log("Данные сброшены")
        self.status_var.set("Данные сброшены")
    
    def clear_log(self):
        """Очистка лога"""
        self.log_text.delete("1.0", tk.END)
        self.log("Лог очищен")
    
    def log(self, message):
        """Добавление сообщения в лог"""
        self.log_text.insert(tk.END, f"{message}\n")
        self.log_text.see(tk.END)
        self.root.update_idletasks()
    
    def process_log_queue(self):
        """Обработка сообщений из очереди логов"""
        try:
            while True:
                message = self.log_queue.get_nowait()
                if message == "DONE":
                    # Разблокируем кнопки после завершения теста
                    self.start_btn.config(state='normal')
                    self.setup_btn.config(state='normal')
                    self.status_var.set("Тестирование завершено")
                else:
                    self.log(message)
        except queue.Empty:
            pass
        
        # Планируем следующую проверку через 100мс
        self.root.after(100, self.process_log_queue)
    
    def run(self):
        """Запуск главного цикла"""
        self.root.mainloop()
    
    def __del__(self):
        """Деструктор - освобождение ресурсов"""
        if self.tester:
            lib.destroy_tester(self.tester)
        if self.config:
            lib.destroy_test_data_config(self.config)

if __name__ == "__main__":
    # Проверяем наличие библиотеки
    if not os.path.exists('./libload_tester.so'):
        print("Ошибка: файл libload_tester.so не найден!")
        print("Скомпилируйте C++ код сначала:")
        print("g++ -std=c++17 -fPIC -O2 -c load_tester.cpp -o load_tester.o")
        print("g++ -std=c++17 -fPIC -O2 -c load_tester_c.cpp -o load_tester_c.o")
        print("g++ -shared -o libload_tester.so load_tester.o load_tester_c.o -lcurl -ljsoncpp -lpthread")
        input("Нажмите Enter для выхода...")  # <-- Ждет нажатия Enter
        sys.exit(1)
    
    try:
        app = LoadTesterGUI()
        app.run()
    except Exception as e:
        print(f"Критическая ошибка: {e}")
        import traceback
        print(traceback.format_exc())
        input("Нажмите Enter для выхода...")  # <-- Ждет нажатия Enter