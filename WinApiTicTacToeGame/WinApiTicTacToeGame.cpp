#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <windowsx.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <io.h>
#include <cstdlib>
#include <ctime>
#include <cmath>

using namespace std;

// Определение строковых констант для класса окна и его имени
const TCHAR szWinClass[] = _T("App");
const TCHAR szWinName[] = _T("Window");

// Объявления для межпро цессорного взаимодействия board
HANDLE hMapFile; // идентификатор разделяемого сегмента памяти
LPCTSTR szName = _T("Local\\SharedMemoryExample"); // Имя для разделяемого сегмента памяти
// для commandsArray
HANDLE hMapFileCommands; // идентификатор разделяемого сегмента памяти
LPCTSTR szNameC = _T("Local\\SharedMemoryCommands"); // Имя для разделяемого сегмента памяти

// Объявление глобальных переменных
HWND hwnd;               // Дескриптор окна
HBRUSH hBackgroundBrush; // Кисть для фона
HPEN hGridPen;           // Ручка для отрисовки сетки
int gridSize = 3;        // Размер сетки по умолчанию
int cellSize1;            // Размер ячейки на игровом поле
int cellSize2;            // Размер ячейки на игровом поле
int* board;              // Массив для хранения состояния игрового поля
UINT_PTR g_nTimerID;     // Индификатор таймера
int* commandsArray;      // Массив для хранения команд
int check = 0;
HANDLE backThread = NULL, hFileMapping, hFileMapping1;

// Объявление переменных для хранения цветов фона
COLORREF startColor = RGB(rand() % 256, rand() % 256, rand() % 256);
COLORREF endColor = RGB(rand() % 256, rand() % 256, rand() % 256);
COLORREF currentColor = startColor;
const int animationSpeed = 50; // Скорость анимации (в миллисекундах)
int animationStep = 0; // Текущий шаг анимации

// Переменная для имени файла конфигурации в формате wchar_t
const wchar_t* sconfig_c = L"config.cfg";
// Переменная для имени файла конфигурации в формате string
string sconfig_str = "config.cfg";
// Переменная для хранения строки аргументов командной строки
string str = "";
// Структура для представления цвета в формате RGB
struct rgb {
    int r, g, b;
};
// Структура для хранения конфигурационных параметров программы
struct Config {
    int N = 3;
    int width = 320;
    int height = 240;
    rgb grid_line_color = { 255, 0, 0 };
    rgb bg_color = { 0, 0, 255 };
}config;

HANDLE hMutex;

LOGPEN logpen; // Структура для хранения параметров текущего Pen

// Прототипы функций
void DrawGrid(HDC hdc);
void DrawCross(HDC hdc, int row, int col);
void DrawCircle(HDC hdc, int row, int col);
void GetClickedCell(int x, int y, int& row, int& col);
void RunNotepad();
void SaveConfig1(Config& config);
void LoadConfig1(Config& config);
void SaveConfig2(Config& config);
void LoadConfig2(Config& config);
void SaveConfig3(Config& config);
void LoadConfig3(Config& config);
void SaveConfig4(Config& config);
void LoadConfig4(Config& config);
bool CheckCrossWin();
bool CheckCircleWin();
bool CheckDraw();
DWORD WINAPI background(LPVOID);

// Основная функция
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// Основная функция программы
int main(int argc, char** argv)
{
    setlocale(LC_ALL, "");
    BOOL bMessageOk;
    MSG message;
    WNDCLASS wincl = { 0 };

    int nCmdShow = SW_SHOW;
    HINSTANCE hThisInstance = GetModuleHandle(NULL);

    // Создание разделяемого сегмента памяти для board
    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,   // использовать файл обмена
        NULL,                   // защита по умолчанию
        PAGE_READWRITE,         // чтение/запись доступа
        0,                      // размер выделенного блока, в байтах
        sizeof(int) * gridSize * gridSize, // размер массива в разделяемой памяти
        szName);                // имя файла обмена

    // Создание мьютекса
    hMutex = CreateMutex(NULL, FALSE, _T("MutexExample"));
    if (hMutex == NULL)
    {
        _tprintf(_T("CreateMutex error: %d\n"), GetLastError());
        return 1;
    }

    if (hMapFile == NULL)
    {
        _tprintf(_T("Could not create file mapping object (%d).\n"), GetLastError());
        return 1;
    }

    // Присоединение разделяемого сегмента к адресному пространству текущего процесса
    board = (int*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int) * gridSize * gridSize);
    if (board == NULL)
    {
        _tprintf(_T("Could not map view of file (%d).\n"), GetLastError());
        CloseHandle(hMapFile);
        return 1;
    }

    // Выделение памяти под массив для хранения состояния игрового поля
    board = (int*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int) * gridSize * gridSize);

    // Создание разделяемого сегмента памяти для commandsArray
    hMapFileCommands = CreateFileMapping(
        INVALID_HANDLE_VALUE,   // использовать файл обмена
        NULL,                   // защита по умолчанию
        PAGE_READWRITE,         // чтение/запись доступа
        0,                      // размер выделенного блока, в байтах
        6, // размер массива в разделяемой памяти
        szNameC);                // имя файла обмена

    // Присоединение разделяемого сегмента к адресному пространству текущего процесса
    commandsArray = (int*)MapViewOfFile(hMapFileCommands, FILE_MAP_ALL_ACCESS, 0, 0, 6);
    if (commandsArray == NULL)
    {
        _tprintf(_T("Could not map view of file (%d).\n"), GetLastError());
        CloseHandle(hMapFile);
        return 1;
    }

    // Выделение памяти под массив для хранения состояния игрового поля
    commandsArray = (int*)MapViewOfFile(hMapFileCommands, FILE_MAP_ALL_ACCESS, 0, 0, 6);

    // Проверка наличия аргументов командной строки
    if (argc > 2) {
        gridSize = atoi(argv[1]);
        str = argv[2];
        cout << "Метод - " << str << endl;
        if (str == "m1") LoadConfig1(config);
        else if (str == "m2") LoadConfig2(config);
        else if (str == "m3") LoadConfig3(config);
        else if (str == "m4") LoadConfig4(config);
        config.N = atoi(argv[1]);
    }
    if (argc == 2)
    {
        gridSize = atoi(argv[1]);
    }

    // Выбор команды
    int result = MessageBox(NULL, L"Крестики - да | Нолики - нет", L"Выбор команды", MB_YESNO | MB_ICONQUESTION);
    if (result == IDYES)
    {
        if (commandsArray[0] != 0 && commandsArray[1] != 0)
        {
            MessageBox(NULL, L"Все команды заняты", L"Ошибка", MB_OK | MB_ICONINFORMATION);

            // Завершение работы приложения при закрытии окна
            PostQuitMessage(0);
            // Освобождение выделенной памяти
            CloseHandle(backThread);
            UnmapViewOfFile(board);
            UnmapViewOfFile(commandsArray);
        }
        else
        {
            if (commandsArray[0] != 0)
            {
                MessageBox(NULL, L"Крестики заняты", L"Ошибка", MB_OK | MB_ICONINFORMATION);

                // Завершение работы приложения при закрытии окна
                PostQuitMessage(0);
                // Освобождение выделенной памяти
                CloseHandle(backThread);
                UnmapViewOfFile(board);
                UnmapViewOfFile(commandsArray);
            }
            else
            {
                // Попытка захвата мьютекса
                DWORD dwWaitResult = WaitForSingleObject(hMutex, INFINITE);
                // Меняем значение для команды крестиков
                commandsArray[0] = 1;
                commandsArray[3] = GetCurrentThreadId();
                // Освобождение мьютекса
                ReleaseMutex(hMutex);
            }
        }
    }
    else if (result == IDNO)
    {
        if (commandsArray[0] != 0 && commandsArray[1] != 0)
        {
            MessageBox(NULL, L"Все команды заняты", L"Ошибка", MB_OK | MB_ICONINFORMATION);

            // Завершение работы приложения при закрытии окна
            PostQuitMessage(0);
            // Освобождение выделенной памяти
            CloseHandle(backThread);
            UnmapViewOfFile(board);
            UnmapViewOfFile(commandsArray);
        }
        else
        {
            if (commandsArray[1] != 0)
            {
                MessageBox(NULL, L"Нолики заняты", L"Ошибка", MB_OK | MB_ICONINFORMATION);

                // Завершение работы приложения при закрытии окна
                PostQuitMessage(0);
                // Освобождение выделенной памяти
                CloseHandle(backThread);
                UnmapViewOfFile(board);
                UnmapViewOfFile(commandsArray);
            }
            else
            {
                // Попытка захвата мьютекса
                DWORD dwWaitResult = WaitForSingleObject(hMutex, INFINITE);
                // Меняем значение для команды крестиков
                commandsArray[1] = 1;
                commandsArray[4] = GetCurrentThreadId();
                // Освобождение мьютекса
                ReleaseMutex(hMutex);
            }
        }
    }

    // Создание кисти для фона и ручки для сетки
    hBackgroundBrush = CreateSolidBrush(RGB(config.bg_color.r, config.bg_color.g, config.bg_color.b));
    hGridPen = CreatePen(PS_SOLID, 2, RGB(config.grid_line_color.r, config.grid_line_color.g, config.grid_line_color.b));

    // Заполнение структуры WNDCLASS и регистрация класса окна
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szWinClass;
    wincl.lpfnWndProc = WindowProcedure;
    wincl.hbrBackground = hBackgroundBrush;

    if (!RegisterClass(&wincl))
        return 0;

    // Создание окна
    hwnd = CreateWindow(
        szWinClass,
        szWinName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        config.width,
        config.width,
        HWND_DESKTOP,
        NULL,
        hThisInstance,
        NULL);

    // Отображение окна
    ShowWindow(hwnd, nCmdShow);

    // Создание таймера для периодического обновления интерфейса
    g_nTimerID = SetTimer(hwnd, 1, 100, NULL);

    // Обработка сообщений
    while ((bMessageOk = GetMessage(&message, NULL, 0, 0)) != 0)
    {
        if (bMessageOk == -1)
        {
            // Вывод сообщения об ошибке, если GetMessage завершилось с ошибкой
            puts("GetMessage failed! You can call GetLastError() to see what happened");
            break;
        }
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    // Освобождение выделенной памяти и объектов GDI
    free(board);
    DeleteObject(hBackgroundBrush);
    DeleteObject(hGridPen);
    CloseHandle(hMapFile);

    // Освобождение разделяемого сегмента при завершении работы
    UnmapViewOfFile(board);
    CloseHandle(hMapFile);

    // Завершение работы с мьютиксом
    CloseHandle(hMutex);

    return 0;
}

// Функция отрисовки сетки
void DrawGrid(HDC hdc)
{
    RECT rect;
    GetClientRect(hwnd, &rect);

    // Вычисление размера ячейки на основе размера окна и количества ячеек в сетке
    cellSize1 = rect.right / gridSize;
    cellSize2 = rect.bottom / gridSize;

    // Настройка ручки для отрисовки сетки
    SelectObject(hdc, hGridPen);

    // Отрисовка вертикальных линий сетки
    for (int i = 1; i < gridSize; ++i)
    {
        int coord1 = i * cellSize1;
        int coord2 = i * cellSize2;

        MoveToEx(hdc, coord1, 0, NULL);
        LineTo(hdc, coord1, rect.bottom);

        MoveToEx(hdc, 0, coord2, NULL);
        LineTo(hdc, rect.right, coord2);
    }

    // Отрисовка крестиков и ноликов на игровом поле
    for (int row = 0; row < gridSize; ++row)
    {
        for (int col = 0; col < gridSize; ++col)
        {
            int index = row * gridSize + col;
            if (board[index] == 1)
            {
                DrawCross(hdc, row, col);
            }
            else if (board[index] == 2)
            {
                DrawCircle(hdc, row, col);
            }
        }
    }
}

// Функция отрисовки крестика
void DrawCross(HDC hdc, int row, int col)
{
    int x = col * cellSize1;
    int y = row * cellSize2;

    // Отрисовка первой линии крестика
    MoveToEx(hdc, x, y, NULL);
    LineTo(hdc, x + cellSize1, y + cellSize2);

    // Отрисовка второй линии крестика
    MoveToEx(hdc, x, y + cellSize2, NULL);
    LineTo(hdc, x + cellSize1, y);
}

// Функция отрисовки нолика
void DrawCircle(HDC hdc, int row, int col)
{
    int x = col * cellSize1;
    int y = row * cellSize2;

    // Смена кисти для отрисовки нолика
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, CreateSolidBrush(RGB(255, 0, 0)));

    // Отрисовка эллипса (нолика)
    Ellipse(hdc, x, y, x + cellSize1, y + cellSize2);

    // Восстановление предыдущей кисти
    SelectObject(hdc, hOldBrush);

    // Удаление созданной кисти
    DeleteObject(hOldBrush);
}

// Функция определения ячейки, по которой было произведено нажатие
void GetClickedCell(int x, int y, int& row, int& col)
{
    // Вычисление номера строки и столбца, по которым было произведено нажатие мыши
    row = y / cellSize2;
    col = x / cellSize1;
}

// Запуск приложения Notepad
void RunNotepad()
{
    ShellExecute(NULL, _T("open"), _T("notepad.exe"), NULL, NULL, SW_SHOWNORMAL);
}

// При помощи отображения файлов на память (CreateFileMapping/MapViewOfFile)
void SaveConfig1(Config& config) {
    // Создаем или открываем файл для записи
    HANDLE hFile = CreateFile(L"config.cfg", GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        // Создаем отображение файла в память
        HANDLE hMapFile = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, sizeof(Config), NULL);
        if (hMapFile != NULL) {
            // Отображаем файл на память
            LPVOID pData = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Config));
            if (pData != NULL) {
                // Копируем конфигурацию в отображенный участок памяти
                memcpy(pData, &config, sizeof(Config));
                // Отключаем отображенный участок памяти
                UnmapViewOfFile(pData);
            }
            // Закрываем отображение файла
            CloseHandle(hMapFile);
        }
        // Закрываем файл
        CloseHandle(hFile);
    }
}
void LoadConfig1(Config& config) {
    // Открываем файл для чтения
    HANDLE hFile = CreateFile(L"config.cfg", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        // Создаем отображение файла в память
        HANDLE hMapFile = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, sizeof(Config), NULL);
        if (hMapFile != NULL) {
            // Отображаем файл на память
            LPVOID pData = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, sizeof(Config));
            if (pData != NULL) {
                // Копируем конфигурацию из отображенного участка памяти
                memcpy(&config, pData, sizeof(Config));
                // Отключаем отображенный участок памяти
                UnmapViewOfFile(pData);
            }
            // Закрываем отображение файла
            CloseHandle(hMapFile);
        }
        // Закрываем файл
        CloseHandle(hFile);
    }
}

// При помощи файловых переменных (fopen / fread / fwrite / fclose).
void SaveConfig2(Config& config) {
    FILE* file;
    // Открываем файл для записи в бинарном режиме
    if (fopen_s(&file, "config.cfg", "wb") == 0) {
        // Записываем конфигурацию в файл
        fwrite(&config, sizeof(Config), 1, file);
        // Закрываем файл
        fclose(file);
    }
    else {
        // Выводим сообщение об ошибке, если не удалось открыть файл для записи
        std::cerr << "Failed to open file for writing." << std::endl;
    }
}
void LoadConfig2(Config& config) {
    FILE* file;
    // Открываем файл для чтения в бинарном режиме
    if (fopen_s(&file, "config.cfg", "rb") == 0) {
        // Считываем конфигурацию из файла
        fread(&config, sizeof(Config), 1, file);
        // Закрываем файл
        fclose(file);
    }
    else {
        // Выводим сообщение об ошибке, если не удалось открыть файл для чтения
        std::cerr << "Failed to open file for reading." << std::endl;
    }
}

// При помощи потоков ввода-вывода (fstream)
void SaveConfig3(Config& config) {
    // Открываем файл для записи в бинарном режиме
    ofstream fout("config.cfg", std::ios::binary | std::ios::out);
    // Записываем конфигурацию в файл
    fout.write(reinterpret_cast<char*>(&config), sizeof(Config));
    // Закрываем файл
    fout.close();
}
void LoadConfig3(Config& config) {
    // Открываем файл для чтения в бинарном режиме
    ifstream fin("config.cfg", std::ios::binary | std::ios::in);
    // Считываем конфигурацию из файла
    fin.read(reinterpret_cast<char*>(&config), sizeof(Config));
    // Закрываем файл
    fin.close();
}

// При помощи файловых функций WinAPI/NativeAPI.
void SaveConfig4(Config& config) {
    // Открываем файл для записи
    HANDLE hFile = CreateFile(L"config.cfg", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        // Записываем конфигурацию в файл
        DWORD bytesWritten;
        WriteFile(hFile, &config, sizeof(Config), &bytesWritten, NULL);
        // Закрываем файл
        CloseHandle(hFile);
    }
    else {
        std::cerr << "Failed to open file for writing." << std::endl;
    }
}
void LoadConfig4(Config& config) {
    // Открываем файл для чтения
    HANDLE hFile = CreateFile(L"config.cfg", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        // Считываем конфигурацию из файла
        DWORD bytesRead;
        ReadFile(hFile, &config, sizeof(Config), &bytesRead, NULL);
        // Закрываем файл
        CloseHandle(hFile);
    }
    else {
        std::cerr << "Failed to open file for reading." << std::endl;
    }
}

// Функция для проверки победы крестиков
bool CheckCrossWin() {
    // Проверяем все строки, столбцы и диагонали на наличие крестиков подряд

    // Проверка строк
    for (int i = 0; i < gridSize; ++i) {
        bool rowWin = true; // Флаг, обозначающий победу в текущей строке
        for (int j = 0; j < gridSize; ++j) {
            if (board[i * gridSize + j] != 1) { // Если хотя бы одна ячейка не содержит крестик, то победы в строке нет
                rowWin = false;
                break;
            }
        }
        if (rowWin) return true; // Если все ячейки строки содержат крестики, то это победа
    }

    // Проверка столбцов
    for (int i = 0; i < gridSize; ++i) {
        bool colWin = true; // Флаг, обозначающий победу в текущем столбце
        for (int j = 0; j < gridSize; ++j) {
            if (board[j * gridSize + i] != 1) { // Если хотя бы одна ячейка не содержит крестик, то победы в столбце нет
                colWin = false;
                break;
            }
        }
        if (colWin) return true; // Если все ячейки столбца содержат крестики, то это победа
    }

    // Проверка главной диагонали
    bool mainDiagonalWin = true; // Флаг, обозначающий победу в главной диагонали
    for (int i = 0; i < gridSize; ++i) {
        if (board[i * gridSize + i] != 1) { // Если хотя бы одна ячейка на главной диагонали не содержит крестик, то победы в ней нет
            mainDiagonalWin = false;
            break;
        }
    }
    if (mainDiagonalWin) return true; // Если все ячейки на главной диагонали содержат крестики, то это победа

    // Проверка побочной диагонали
    bool secondaryDiagonalWin = true; // Флаг, обозначающий победу в побочной диагонали
    for (int i = 0; i < gridSize; ++i) {
        if (board[i * gridSize + (gridSize - 1 - i)] != 1) { // Если хотя бы одна ячейка на побочной диагонали не содержит крестик, то победы в ней нет
            secondaryDiagonalWin = false;
            break;
        }
    }
    if (secondaryDiagonalWin) return true; // Если все ячейки на побочной диагонали содержат крестики, то это победа

    return false; // Если ни одно из условий не выполнено, то победы крестиков нет
}

// Функция для проверки победы ноликов
bool CheckCircleWin() {
    // Проверяем все строки, столбцы и диагонали на наличие ноликов подряд

    // Проверка строк
    for (int i = 0; i < gridSize; ++i) {
        bool rowWin = true; // Флаг, обозначающий победу в текущей строке
        for (int j = 0; j < gridSize; ++j) {
            if (board[i * gridSize + j] != 2) { // Если хотя бы одна ячейка не содержит нолик, то победы в строке нет
                rowWin = false;
                break;
            }
        }
        if (rowWin) return true; // Если все ячейки строки содержат нолики, то это победа
    }

    // Проверка столбцов
    for (int i = 0; i < gridSize; ++i) {
        bool colWin = true; // Флаг, обозначающий победу в текущем столбце
        for (int j = 0; j < gridSize; ++j) {
            if (board[j * gridSize + i] != 2) { // Если хотя бы одна ячейка не содержит нолик, то победы в столбце нет
                colWin = false;
                break;
            }
        }
        if (colWin) return true; // Если все ячейки столбца содержат нолики, то это победа
    }

    // Проверка главной диагонали
    bool mainDiagonalWin = true; // Флаг, обозначающий победу в главной диагонали
    for (int i = 0; i < gridSize; ++i) {
        if (board[i * gridSize + i] != 2) { // Если хотя бы одна ячейка на главной диагонали не содержит нолик, то победы в ней нет
            mainDiagonalWin = false;
            break;
        }
    }
    if (mainDiagonalWin) return true; // Если все ячейки на главной диагонали содержат нолики, то это победа

    // Проверка побочной диагонали
    bool secondaryDiagonalWin = true; // Флаг, обозначающий победу в побочной диагонали
    for (int i = 0; i < gridSize; ++i) {
        if (board[i * gridSize + (gridSize - 1 - i)] != 2) { // Если хотя бы одна ячейка на побочной диагонали не содержит нолик, то победы в ней нет
            secondaryDiagonalWin = false;
            break;
        }
    }
    if (secondaryDiagonalWin) return true; // Если все ячейки на побочной диагонали содержат нолики, то это победа

    return false; // Если ни одно из условий не выполнено, то победы ноликов нет
}

// Функция для проверки ничьи
bool CheckDraw() {
    // Проверяем, остались ли еще пустые ячейки

    for (int i = 0; i < gridSize * gridSize; ++i) {
        if (board[i] == 0)
            return false; // Найдена пустая ячейка, игра не окончена
    }
    // Если все ячейки заполнены, и нет победителя, то это ничья
    return true;
}

// Функция анимации фона которая будет выполняться в отдельном потоке
DWORD WINAPI background(LPVOID)
{
    while (true)
    {
        // Запись цвета фона в конфигурационный файл
        config.bg_color.r = rand() % 256;
        config.bg_color.g = rand() % 256;
        config.bg_color.b = rand() % 256;

        hBackgroundBrush = CreateSolidBrush(RGB(config.bg_color.r, config.bg_color.g, config.bg_color.b));
        // Установка новой кисти для фона
        SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hBackgroundBrush);
        // Запрос перерисовки окна с новым фоном
        InvalidateRect(hwnd, NULL, TRUE);
        Sleep(500);
    }
    return 0;
}

// Основная функция обработки сообщений окна
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    switch (message)
    {
    case WM_DESTROY:
        // Сохраняем конфигурацию
        if (str == "m1") SaveConfig1(config);
        else if (str == "m2") SaveConfig2(config);
        else if (str == "m3") SaveConfig3(config);
        else if (str == "m4") SaveConfig4(config);

        // Завершение работы приложения при закрытии окна
        PostQuitMessage(0);
        // Освобождение выделенной памяти
        CloseHandle(backThread);
        UnmapViewOfFile(board);
        UnmapViewOfFile(commandsArray);
        return 0;
    case WM_PAINT:
    {
        // Обработка сообщения о необходимости перерисовки окна
        PAINTSTRUCT ps;
        hdc = BeginPaint(hwnd, &ps);

        // Заливка окна цветом фона
        FillRect(hdc, &ps.rcPaint, hBackgroundBrush);

        // Отрисовка сетки и символов на игровом поле
        DrawGrid(hdc);

        // Захват мьютекса перед чтением данных
        WaitForSingleObject(hMutex, INFINITE);

        // Проверка изменений в разделяемой памяти
        for (int row = 0; row < gridSize; ++row)
        {
            for (int col = 0; col < gridSize; ++col)
            {
                int index = row * gridSize + col;
                if (board[index] == 1)
                {
                    DrawCross(hdc, row, col);
                }
                else if (board[index] == 2)
                {
                    DrawCircle(hdc, row, col);
                }
            }
        }

        // Освобождение мьютекса после чтения данных
        ReleaseMutex(hMutex);

        // Завершение перерисовки
        EndPaint(hwnd, &ps);
    }
    return 0;
    case WM_SIZE:
    {
        // Определение размеров окна после изменения его размера
        RECT rcClient;
        GetWindowRect(hwnd, &rcClient);
        config.width = rcClient.right - rcClient.left;
        config.height = rcClient.bottom - rcClient.top;

        // Запрос перерисовки окна
        InvalidateRect(hwnd, NULL, TRUE);
    }
    return 0;
    case WM_TIMER:
    {
        // Обработка таймера для периодического обновления интерфейса
        InvalidateRect(hwnd, NULL, TRUE);
    }
    return 0;
    case WM_LBUTTONDOWN:
    {
        if (commandsArray[5] == 0)
        {
            int pID = GetCurrentThreadId();
            if (pID == commandsArray[3])
            {
                if (commandsArray[2] % 2 == 0)
                {
                    int xPos = LOWORD(lParam);
                    int yPos = HIWORD(lParam);

                    int row, col;
                    GetClickedCell(xPos, yPos, row, col);

                    int index = row * gridSize + col;

                    // Попытка захвата мьютекса
                    DWORD dwWaitResult = WaitForSingleObject(hMutex, INFINITE);

                    if (dwWaitResult == WAIT_OBJECT_0)
                    {
                        // Проверка, что ячейка не занята
                        if (board[index] == 0)
                        {
                            board[index] = 1; // Используем 1 для крестика
                            commandsArray[2]++;

                            // Запрос перерисовки окна после изменения игрового поля
                            InvalidateRect(hwnd, NULL, TRUE);
                        }

                        // Освобождение мьютекса
                        ReleaseMutex(hMutex);
                    }
                    if (CheckCrossWin())
                    {
                        commandsArray[5] = 1;
                        MessageBox(NULL, L"Крестики победили", L"Результат", MB_OK | MB_ICONINFORMATION);
                        break;
                    }
                    if (CheckDraw())
                    {
                        commandsArray[5] = 1;
                        MessageBox(NULL, L"Ничья", L"Результат", MB_OK | MB_ICONINFORMATION);
                    }

                }
                else
                    MessageBox(NULL, L"Ход ноликов", L"Ошибка", MB_OK | MB_ICONINFORMATION);
            }
            if (pID == commandsArray[4])
            {
                if (commandsArray[2] % 2 != 0)
                {
                    int xPos = LOWORD(lParam);
                    int yPos = HIWORD(lParam);

                    int row, col;
                    GetClickedCell(xPos, yPos, row, col);

                    int index = row * gridSize + col;

                    // Попытка захвата мьютекса
                    DWORD dwWaitResult = WaitForSingleObject(hMutex, INFINITE);

                    if (dwWaitResult == WAIT_OBJECT_0)
                    {
                        // Проверка, что ячейка не занята
                        if (board[index] == 0)
                        {
                            board[index] = 2; // Используем 2 для круга
                            commandsArray[2]++;

                            // Запрос перерисовки окна после изменения игрового поля
                            InvalidateRect(hwnd, NULL, TRUE);
                        }

                        // Освобождение мьютекса
                        ReleaseMutex(hMutex);
                    }
                    if (CheckCircleWin())
                    {
                        commandsArray[5] = 1;
                        MessageBox(NULL, L"Нолики победили", L"Результат", MB_OK | MB_ICONINFORMATION);
                        break;
                    }
                    if (CheckDraw())
                    {
                        commandsArray[5] = 1;
                        MessageBox(NULL, L"Ничья", L"Результат", MB_OK | MB_ICONINFORMATION);
                    }
                }
                else
                    MessageBox(NULL, L"Ход крестиков", L"Ошибка", MB_OK | MB_ICONINFORMATION);
            }
        }
    }
    return 0;

    case WM_RBUTTONDOWN:
    {
        if (commandsArray[5] == 0)
        {
            int pID = GetCurrentThreadId();
            if (pID == commandsArray[3])
            {
                if (commandsArray[2] % 2 == 0)
                {
                    int xPos = LOWORD(lParam);
                    int yPos = HIWORD(lParam);

                    int row, col;
                    GetClickedCell(xPos, yPos, row, col);

                    int index = row * gridSize + col;

                    // Попытка захвата мьютекса
                    DWORD dwWaitResult = WaitForSingleObject(hMutex, INFINITE);

                    if (dwWaitResult == WAIT_OBJECT_0)
                    {
                        // Проверка, что ячейка не занята
                        if (board[index] == 0)
                        {
                            board[index] = 1; // Используем 1 для крестика
                            commandsArray[2]++;

                            // Запрос перерисовки окна после изменения игрового поля
                            InvalidateRect(hwnd, NULL, TRUE);
                        }

                        // Освобождение мьютекса
                        ReleaseMutex(hMutex);
                    }
                    if (CheckCrossWin())
                    {
                        commandsArray[5] = 1;
                        MessageBox(NULL, L"Крестики победили", L"Результат", MB_OK | MB_ICONINFORMATION);
                        break;
                    }
                    if (CheckDraw())
                    {
                        commandsArray[5] = 1;
                        MessageBox(NULL, L"Ничья", L"Результат", MB_OK | MB_ICONINFORMATION);
                    }

                }
                else
                    MessageBox(NULL, L"Ход ноликов", L"Ошибка", MB_OK | MB_ICONINFORMATION);
            }
            if (pID == commandsArray[4])
            {
                if (commandsArray[2] % 2 != 0)
                {
                    int xPos = LOWORD(lParam);
                    int yPos = HIWORD(lParam);

                    int row, col;
                    GetClickedCell(xPos, yPos, row, col);

                    int index = row * gridSize + col;

                    // Попытка захвата мьютекса
                    DWORD dwWaitResult = WaitForSingleObject(hMutex, INFINITE);

                    if (dwWaitResult == WAIT_OBJECT_0)
                    {
                        // Проверка, что ячейка не занята
                        if (board[index] == 0)
                        {
                            board[index] = 2; // Используем 2 для круга
                            commandsArray[2]++;

                            // Запрос перерисовки окна после изменения игрового поля
                            InvalidateRect(hwnd, NULL, TRUE);
                        }

                        // Освобождение мьютекса
                        ReleaseMutex(hMutex);
                    }
                    if (CheckCircleWin())
                    {
                        commandsArray[5] = 1;
                        MessageBox(NULL, L"Нолики победили", L"Результат", MB_OK | MB_ICONINFORMATION);
                        break;
                    }
                    if (CheckDraw())
                    {
                        commandsArray[5] = 1;
                        MessageBox(NULL, L"Ничья", L"Результат", MB_OK | MB_ICONINFORMATION);
                    }
                }
                else
                    MessageBox(NULL, L"Ход крестиков", L"Ошибка", MB_OK | MB_ICONINFORMATION);
            }
        }
    }
    return 0;
    case WM_KEYDOWN:
    {
        // Обработка сообщений о нажатии клавиш
        if ((wParam == 'C' || wParam == 'c') && (GetKeyState(VK_SHIFT) & 0x8000))
        {
            // Запуск Notepad при нажатии "Shift + C"
            RunNotepad();
        }
        else if (wParam == VK_RETURN)
        {
            // Запись цвета фона в конфигурационный файл
            config.bg_color.r = rand() % 256;
            config.bg_color.g = rand() % 256;
            config.bg_color.b = rand() % 256;

            // Изменение цвета фона на случайный при нажатии "Enter"
            hBackgroundBrush = CreateSolidBrush(RGB(config.bg_color.r, config.bg_color.g, config.bg_color.b));
            // Установка новой кисти для фона
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hBackgroundBrush);
            // Запрос перерисовки окна с новым фоном
            InvalidateRect(hwnd, NULL, TRUE);
        }
        else if (wParam == VK_ESCAPE || ((wParam == 'Q' || wParam == 'q') && (GetKeyState(VK_CONTROL) & 0x8000)))
        {
            // Завершение работы приложения при нажатии "Ctrl + Q" или "Esc"
            PostQuitMessage(0);
        }
        else if (wParam == VK_SPACE)
        {
            if (check == 0)
            {
                backThread = CreateThread(NULL, 64 * 1024, background, NULL, 0, NULL);
                check = 1;
            }
            else if (check == 1)
            {

                SuspendThread(backThread); // Закрываем дескриптор потока

                check = 2;
            }
            else if (check == 2)
            {

                ResumeThread(backThread); // Закрываем дескриптор потока

                check = 1;
            }
        }
        else if (wParam == '1')
        {
            if (check > 0)
            {
                SetThreadPriority(backThread, THREAD_PRIORITY_IDLE);
                int currentPriority = GetThreadPriority(backThread);
                cout << "Приоритет потока: " << currentPriority << " Самый низкий приоритет." << endl << endl;
            }
            else
            {
                cout << "Установка приоритета невозможна." << endl;
            }
        }
        else if (wParam == '2')
        {
            if (check > 0)
            {
                SetThreadPriority(backThread, THREAD_PRIORITY_LOWEST);
                int currentPriority = GetThreadPriority(backThread);
                cout << "Приоритет потока: " << currentPriority << " Низкий приоритет." << endl << endl;
            }
            else
            {
                cout << "Установка приоритета невозможна." << endl;
            }
        }
        else if (wParam == '3')
        {
            if (check > 0)
            {
                SetThreadPriority(backThread, THREAD_PRIORITY_BELOW_NORMAL);
                int currentPriority = GetThreadPriority(backThread);
                cout << "Приоритет потока: " << currentPriority << " Ниже среднего приоритета." << endl << endl;
            }
            else
            {
                cout << "Установка приоритета невозможна." << endl;
            }
        }
        else if (wParam == '4')
        {
            if (check > 0)
            {
                SetThreadPriority(backThread, THREAD_PRIORITY_NORMAL);
                int currentPriority = GetThreadPriority(backThread);
                cout << "Приоритет потока: " << currentPriority << " Стандартный приоритет." << endl << endl;
            }
            else
            {
                cout << "Установка приоритета невозможна." << endl;
            }
        }
        else if (wParam == '5')
        {
            if (check > 0)
            {
                SetThreadPriority(backThread, THREAD_PRIORITY_ABOVE_NORMAL);
                int currentPriority = GetThreadPriority(backThread);
                cout << "Приоритет потока: " << currentPriority << " Выше среднего приоритета." << endl << endl;
            }
            else
            {
                cout << "Установка приоритета невозможна." << endl;
            }
        }
        else if (wParam == '6')
        {
            if (check > 0)
            {
                SetThreadPriority(backThread, THREAD_PRIORITY_HIGHEST);
                int currentPriority = GetThreadPriority(backThread);
                cout << "Приоритет потока: " << currentPriority << " Самый высокий приоритет." << endl << endl;
            }
            else
            {
                cout << "Установка приоритета невозможна." << endl;
            }
        }
        else if (wParam == '7')
        {
            if (check > 0)
            {
                SetThreadPriority(backThread, THREAD_PRIORITY_TIME_CRITICAL);
                int currentPriority = GetThreadPriority(backThread);
                cout << "Приоритет потока: " << currentPriority << " Критический по времени приоритет." << endl << endl;
            }
            else
            {
                cout << "Установка приоритета невозможна." << endl;
            }
        }
        break;
    }
    return 0;
    case WM_MOUSEWHEEL:
    {
        // Обработка сообщений от колеса мыши для изменения цвета линий сетки
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        int steps = delta / WHEEL_DELTA * 10;  // Увеличение чувствительности

        // Получение текущего контекста устройства
        HDC hdc = GetDC(hwnd);

        // Получение текущего цвета линий сетки
        COLORREF currentColor;
        if (GetObject(hGridPen, sizeof(logpen), &logpen))
        {
            currentColor = logpen.lopnColor;
        }
        else
        {
            // Если не удалось получить текущий цвет, используем красный
            currentColor = RGB(255, 0, 0);
        }

        // Изменение цвета линий сетки в зависимости от количества шагов
        config.grid_line_color.r = (GetRValue(currentColor) + steps) % 256;
        config.grid_line_color.g = (GetGValue(currentColor) + steps) % 256;
        config.grid_line_color.b = (GetBValue(currentColor) + steps) % 256;

        // Создание новой ручки с измененным цветом
        hGridPen = CreatePen(PS_SOLID, 2, RGB(config.grid_line_color.r, config.grid_line_color.g, config.grid_line_color.b));

        // Запрос перерисовки окна с новым цветом линий сетки
        InvalidateRect(hwnd, NULL, TRUE);

        // Освобождение контекста устройства
        ReleaseDC(hwnd, hdc);
    }
    return 0;
    }

    // Вызов стандартной функции обработки сообщений для необработанных случаев
    return DefWindowProc(hwnd, message, wParam, lParam);
}