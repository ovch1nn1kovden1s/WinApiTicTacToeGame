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

// ����������� ��������� �������� ��� ������ ���� � ��� �����
const TCHAR szWinClass[] = _T("App");
const TCHAR szWinName[] = _T("Window");

// ���������� ��� ������ ���������� �������������� board
HANDLE hMapFile; // ������������� ������������ �������� ������
LPCTSTR szName = _T("Local\\SharedMemoryExample"); // ��� ��� ������������ �������� ������
// ��� commandsArray
HANDLE hMapFileCommands; // ������������� ������������ �������� ������
LPCTSTR szNameC = _T("Local\\SharedMemoryCommands"); // ��� ��� ������������ �������� ������

// ���������� ���������� ����������
HWND hwnd;               // ���������� ����
HBRUSH hBackgroundBrush; // ����� ��� ����
HPEN hGridPen;           // ����� ��� ��������� �����
int gridSize = 3;        // ������ ����� �� ���������
int cellSize1;            // ������ ������ �� ������� ����
int cellSize2;            // ������ ������ �� ������� ����
int* board;              // ������ ��� �������� ��������� �������� ����
UINT_PTR g_nTimerID;     // ����������� �������
int* commandsArray;      // ������ ��� �������� ������
int check = 0;
HANDLE backThread = NULL, hFileMapping, hFileMapping1;

// ���������� ���������� ��� �������� ������ ����
COLORREF startColor = RGB(rand() % 256, rand() % 256, rand() % 256);
COLORREF endColor = RGB(rand() % 256, rand() % 256, rand() % 256);
COLORREF currentColor = startColor;
const int animationSpeed = 50; // �������� �������� (� �������������)
int animationStep = 0; // ������� ��� ��������

// ���������� ��� ����� ����� ������������ � ������� wchar_t
const wchar_t* sconfig_c = L"config.cfg";
// ���������� ��� ����� ����� ������������ � ������� string
string sconfig_str = "config.cfg";
// ���������� ��� �������� ������ ���������� ��������� ������
string str = "";
// ��������� ��� ������������� ����� � ������� RGB
struct rgb {
    int r, g, b;
};
// ��������� ��� �������� ���������������� ���������� ���������
struct Config {
    int N = 3;
    int width = 320;
    int height = 240;
    rgb grid_line_color = { 255, 0, 0 };
    rgb bg_color = { 0, 0, 255 };
}config;

HANDLE hMutex;

LOGPEN logpen; // ��������� ��� �������� ���������� �������� Pen

// ��������� �������
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

// �������� �������
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// �������� ������� ���������
int main(int argc, char** argv)
{
    setlocale(LC_ALL, "");
    BOOL bMessageOk;
    MSG message;
    WNDCLASS wincl = { 0 };

    int nCmdShow = SW_SHOW;
    HINSTANCE hThisInstance = GetModuleHandle(NULL);

    // �������� ������������ �������� ������ ��� board
    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,   // ������������ ���� ������
        NULL,                   // ������ �� ���������
        PAGE_READWRITE,         // ������/������ �������
        0,                      // ������ ����������� �����, � ������
        sizeof(int) * gridSize * gridSize, // ������ ������� � ����������� ������
        szName);                // ��� ����� ������

    // �������� ��������
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

    // ������������� ������������ �������� � ��������� ������������ �������� ��������
    board = (int*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int) * gridSize * gridSize);
    if (board == NULL)
    {
        _tprintf(_T("Could not map view of file (%d).\n"), GetLastError());
        CloseHandle(hMapFile);
        return 1;
    }

    // ��������� ������ ��� ������ ��� �������� ��������� �������� ����
    board = (int*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int) * gridSize * gridSize);

    // �������� ������������ �������� ������ ��� commandsArray
    hMapFileCommands = CreateFileMapping(
        INVALID_HANDLE_VALUE,   // ������������ ���� ������
        NULL,                   // ������ �� ���������
        PAGE_READWRITE,         // ������/������ �������
        0,                      // ������ ����������� �����, � ������
        6, // ������ ������� � ����������� ������
        szNameC);                // ��� ����� ������

    // ������������� ������������ �������� � ��������� ������������ �������� ��������
    commandsArray = (int*)MapViewOfFile(hMapFileCommands, FILE_MAP_ALL_ACCESS, 0, 0, 6);
    if (commandsArray == NULL)
    {
        _tprintf(_T("Could not map view of file (%d).\n"), GetLastError());
        CloseHandle(hMapFile);
        return 1;
    }

    // ��������� ������ ��� ������ ��� �������� ��������� �������� ����
    commandsArray = (int*)MapViewOfFile(hMapFileCommands, FILE_MAP_ALL_ACCESS, 0, 0, 6);

    // �������� ������� ���������� ��������� ������
    if (argc > 2) {
        gridSize = atoi(argv[1]);
        str = argv[2];
        cout << "����� - " << str << endl;
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

    // ����� �������
    int result = MessageBox(NULL, L"�������� - �� | ������ - ���", L"����� �������", MB_YESNO | MB_ICONQUESTION);
    if (result == IDYES)
    {
        if (commandsArray[0] != 0 && commandsArray[1] != 0)
        {
            MessageBox(NULL, L"��� ������� ������", L"������", MB_OK | MB_ICONINFORMATION);

            // ���������� ������ ���������� ��� �������� ����
            PostQuitMessage(0);
            // ������������ ���������� ������
            CloseHandle(backThread);
            UnmapViewOfFile(board);
            UnmapViewOfFile(commandsArray);
        }
        else
        {
            if (commandsArray[0] != 0)
            {
                MessageBox(NULL, L"�������� ������", L"������", MB_OK | MB_ICONINFORMATION);

                // ���������� ������ ���������� ��� �������� ����
                PostQuitMessage(0);
                // ������������ ���������� ������
                CloseHandle(backThread);
                UnmapViewOfFile(board);
                UnmapViewOfFile(commandsArray);
            }
            else
            {
                // ������� ������� ��������
                DWORD dwWaitResult = WaitForSingleObject(hMutex, INFINITE);
                // ������ �������� ��� ������� ���������
                commandsArray[0] = 1;
                commandsArray[3] = GetCurrentThreadId();
                // ������������ ��������
                ReleaseMutex(hMutex);
            }
        }
    }
    else if (result == IDNO)
    {
        if (commandsArray[0] != 0 && commandsArray[1] != 0)
        {
            MessageBox(NULL, L"��� ������� ������", L"������", MB_OK | MB_ICONINFORMATION);

            // ���������� ������ ���������� ��� �������� ����
            PostQuitMessage(0);
            // ������������ ���������� ������
            CloseHandle(backThread);
            UnmapViewOfFile(board);
            UnmapViewOfFile(commandsArray);
        }
        else
        {
            if (commandsArray[1] != 0)
            {
                MessageBox(NULL, L"������ ������", L"������", MB_OK | MB_ICONINFORMATION);

                // ���������� ������ ���������� ��� �������� ����
                PostQuitMessage(0);
                // ������������ ���������� ������
                CloseHandle(backThread);
                UnmapViewOfFile(board);
                UnmapViewOfFile(commandsArray);
            }
            else
            {
                // ������� ������� ��������
                DWORD dwWaitResult = WaitForSingleObject(hMutex, INFINITE);
                // ������ �������� ��� ������� ���������
                commandsArray[1] = 1;
                commandsArray[4] = GetCurrentThreadId();
                // ������������ ��������
                ReleaseMutex(hMutex);
            }
        }
    }

    // �������� ����� ��� ���� � ����� ��� �����
    hBackgroundBrush = CreateSolidBrush(RGB(config.bg_color.r, config.bg_color.g, config.bg_color.b));
    hGridPen = CreatePen(PS_SOLID, 2, RGB(config.grid_line_color.r, config.grid_line_color.g, config.grid_line_color.b));

    // ���������� ��������� WNDCLASS � ����������� ������ ����
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szWinClass;
    wincl.lpfnWndProc = WindowProcedure;
    wincl.hbrBackground = hBackgroundBrush;

    if (!RegisterClass(&wincl))
        return 0;

    // �������� ����
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

    // ����������� ����
    ShowWindow(hwnd, nCmdShow);

    // �������� ������� ��� �������������� ���������� ����������
    g_nTimerID = SetTimer(hwnd, 1, 100, NULL);

    // ��������� ���������
    while ((bMessageOk = GetMessage(&message, NULL, 0, 0)) != 0)
    {
        if (bMessageOk == -1)
        {
            // ����� ��������� �� ������, ���� GetMessage ����������� � �������
            puts("GetMessage failed! You can call GetLastError() to see what happened");
            break;
        }
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    // ������������ ���������� ������ � �������� GDI
    free(board);
    DeleteObject(hBackgroundBrush);
    DeleteObject(hGridPen);
    CloseHandle(hMapFile);

    // ������������ ������������ �������� ��� ���������� ������
    UnmapViewOfFile(board);
    CloseHandle(hMapFile);

    // ���������� ������ � ���������
    CloseHandle(hMutex);

    return 0;
}

// ������� ��������� �����
void DrawGrid(HDC hdc)
{
    RECT rect;
    GetClientRect(hwnd, &rect);

    // ���������� ������� ������ �� ������ ������� ���� � ���������� ����� � �����
    cellSize1 = rect.right / gridSize;
    cellSize2 = rect.bottom / gridSize;

    // ��������� ����� ��� ��������� �����
    SelectObject(hdc, hGridPen);

    // ��������� ������������ ����� �����
    for (int i = 1; i < gridSize; ++i)
    {
        int coord1 = i * cellSize1;
        int coord2 = i * cellSize2;

        MoveToEx(hdc, coord1, 0, NULL);
        LineTo(hdc, coord1, rect.bottom);

        MoveToEx(hdc, 0, coord2, NULL);
        LineTo(hdc, rect.right, coord2);
    }

    // ��������� ��������� � ������� �� ������� ����
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

// ������� ��������� ��������
void DrawCross(HDC hdc, int row, int col)
{
    int x = col * cellSize1;
    int y = row * cellSize2;

    // ��������� ������ ����� ��������
    MoveToEx(hdc, x, y, NULL);
    LineTo(hdc, x + cellSize1, y + cellSize2);

    // ��������� ������ ����� ��������
    MoveToEx(hdc, x, y + cellSize2, NULL);
    LineTo(hdc, x + cellSize1, y);
}

// ������� ��������� ������
void DrawCircle(HDC hdc, int row, int col)
{
    int x = col * cellSize1;
    int y = row * cellSize2;

    // ����� ����� ��� ��������� ������
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, CreateSolidBrush(RGB(255, 0, 0)));

    // ��������� ������� (������)
    Ellipse(hdc, x, y, x + cellSize1, y + cellSize2);

    // �������������� ���������� �����
    SelectObject(hdc, hOldBrush);

    // �������� ��������� �����
    DeleteObject(hOldBrush);
}

// ������� ����������� ������, �� ������� ���� ����������� �������
void GetClickedCell(int x, int y, int& row, int& col)
{
    // ���������� ������ ������ � �������, �� ������� ���� ����������� ������� ����
    row = y / cellSize2;
    col = x / cellSize1;
}

// ������ ���������� Notepad
void RunNotepad()
{
    ShellExecute(NULL, _T("open"), _T("notepad.exe"), NULL, NULL, SW_SHOWNORMAL);
}

// ��� ������ ����������� ������ �� ������ (CreateFileMapping/MapViewOfFile)
void SaveConfig1(Config& config) {
    // ������� ��� ��������� ���� ��� ������
    HANDLE hFile = CreateFile(L"config.cfg", GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        // ������� ����������� ����� � ������
        HANDLE hMapFile = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, sizeof(Config), NULL);
        if (hMapFile != NULL) {
            // ���������� ���� �� ������
            LPVOID pData = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Config));
            if (pData != NULL) {
                // �������� ������������ � ������������ ������� ������
                memcpy(pData, &config, sizeof(Config));
                // ��������� ������������ ������� ������
                UnmapViewOfFile(pData);
            }
            // ��������� ����������� �����
            CloseHandle(hMapFile);
        }
        // ��������� ����
        CloseHandle(hFile);
    }
}
void LoadConfig1(Config& config) {
    // ��������� ���� ��� ������
    HANDLE hFile = CreateFile(L"config.cfg", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        // ������� ����������� ����� � ������
        HANDLE hMapFile = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, sizeof(Config), NULL);
        if (hMapFile != NULL) {
            // ���������� ���� �� ������
            LPVOID pData = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, sizeof(Config));
            if (pData != NULL) {
                // �������� ������������ �� ������������� ������� ������
                memcpy(&config, pData, sizeof(Config));
                // ��������� ������������ ������� ������
                UnmapViewOfFile(pData);
            }
            // ��������� ����������� �����
            CloseHandle(hMapFile);
        }
        // ��������� ����
        CloseHandle(hFile);
    }
}

// ��� ������ �������� ���������� (fopen / fread / fwrite / fclose).
void SaveConfig2(Config& config) {
    FILE* file;
    // ��������� ���� ��� ������ � �������� ������
    if (fopen_s(&file, "config.cfg", "wb") == 0) {
        // ���������� ������������ � ����
        fwrite(&config, sizeof(Config), 1, file);
        // ��������� ����
        fclose(file);
    }
    else {
        // ������� ��������� �� ������, ���� �� ������� ������� ���� ��� ������
        std::cerr << "Failed to open file for writing." << std::endl;
    }
}
void LoadConfig2(Config& config) {
    FILE* file;
    // ��������� ���� ��� ������ � �������� ������
    if (fopen_s(&file, "config.cfg", "rb") == 0) {
        // ��������� ������������ �� �����
        fread(&config, sizeof(Config), 1, file);
        // ��������� ����
        fclose(file);
    }
    else {
        // ������� ��������� �� ������, ���� �� ������� ������� ���� ��� ������
        std::cerr << "Failed to open file for reading." << std::endl;
    }
}

// ��� ������ ������� �����-������ (fstream)
void SaveConfig3(Config& config) {
    // ��������� ���� ��� ������ � �������� ������
    ofstream fout("config.cfg", std::ios::binary | std::ios::out);
    // ���������� ������������ � ����
    fout.write(reinterpret_cast<char*>(&config), sizeof(Config));
    // ��������� ����
    fout.close();
}
void LoadConfig3(Config& config) {
    // ��������� ���� ��� ������ � �������� ������
    ifstream fin("config.cfg", std::ios::binary | std::ios::in);
    // ��������� ������������ �� �����
    fin.read(reinterpret_cast<char*>(&config), sizeof(Config));
    // ��������� ����
    fin.close();
}

// ��� ������ �������� ������� WinAPI/NativeAPI.
void SaveConfig4(Config& config) {
    // ��������� ���� ��� ������
    HANDLE hFile = CreateFile(L"config.cfg", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        // ���������� ������������ � ����
        DWORD bytesWritten;
        WriteFile(hFile, &config, sizeof(Config), &bytesWritten, NULL);
        // ��������� ����
        CloseHandle(hFile);
    }
    else {
        std::cerr << "Failed to open file for writing." << std::endl;
    }
}
void LoadConfig4(Config& config) {
    // ��������� ���� ��� ������
    HANDLE hFile = CreateFile(L"config.cfg", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        // ��������� ������������ �� �����
        DWORD bytesRead;
        ReadFile(hFile, &config, sizeof(Config), &bytesRead, NULL);
        // ��������� ����
        CloseHandle(hFile);
    }
    else {
        std::cerr << "Failed to open file for reading." << std::endl;
    }
}

// ������� ��� �������� ������ ���������
bool CheckCrossWin() {
    // ��������� ��� ������, ������� � ��������� �� ������� ��������� ������

    // �������� �����
    for (int i = 0; i < gridSize; ++i) {
        bool rowWin = true; // ����, ������������ ������ � ������� ������
        for (int j = 0; j < gridSize; ++j) {
            if (board[i * gridSize + j] != 1) { // ���� ���� �� ���� ������ �� �������� �������, �� ������ � ������ ���
                rowWin = false;
                break;
            }
        }
        if (rowWin) return true; // ���� ��� ������ ������ �������� ��������, �� ��� ������
    }

    // �������� ��������
    for (int i = 0; i < gridSize; ++i) {
        bool colWin = true; // ����, ������������ ������ � ������� �������
        for (int j = 0; j < gridSize; ++j) {
            if (board[j * gridSize + i] != 1) { // ���� ���� �� ���� ������ �� �������� �������, �� ������ � ������� ���
                colWin = false;
                break;
            }
        }
        if (colWin) return true; // ���� ��� ������ ������� �������� ��������, �� ��� ������
    }

    // �������� ������� ���������
    bool mainDiagonalWin = true; // ����, ������������ ������ � ������� ���������
    for (int i = 0; i < gridSize; ++i) {
        if (board[i * gridSize + i] != 1) { // ���� ���� �� ���� ������ �� ������� ��������� �� �������� �������, �� ������ � ��� ���
            mainDiagonalWin = false;
            break;
        }
    }
    if (mainDiagonalWin) return true; // ���� ��� ������ �� ������� ��������� �������� ��������, �� ��� ������

    // �������� �������� ���������
    bool secondaryDiagonalWin = true; // ����, ������������ ������ � �������� ���������
    for (int i = 0; i < gridSize; ++i) {
        if (board[i * gridSize + (gridSize - 1 - i)] != 1) { // ���� ���� �� ���� ������ �� �������� ��������� �� �������� �������, �� ������ � ��� ���
            secondaryDiagonalWin = false;
            break;
        }
    }
    if (secondaryDiagonalWin) return true; // ���� ��� ������ �� �������� ��������� �������� ��������, �� ��� ������

    return false; // ���� �� ���� �� ������� �� ���������, �� ������ ��������� ���
}

// ������� ��� �������� ������ �������
bool CheckCircleWin() {
    // ��������� ��� ������, ������� � ��������� �� ������� ������� ������

    // �������� �����
    for (int i = 0; i < gridSize; ++i) {
        bool rowWin = true; // ����, ������������ ������ � ������� ������
        for (int j = 0; j < gridSize; ++j) {
            if (board[i * gridSize + j] != 2) { // ���� ���� �� ���� ������ �� �������� �����, �� ������ � ������ ���
                rowWin = false;
                break;
            }
        }
        if (rowWin) return true; // ���� ��� ������ ������ �������� ������, �� ��� ������
    }

    // �������� ��������
    for (int i = 0; i < gridSize; ++i) {
        bool colWin = true; // ����, ������������ ������ � ������� �������
        for (int j = 0; j < gridSize; ++j) {
            if (board[j * gridSize + i] != 2) { // ���� ���� �� ���� ������ �� �������� �����, �� ������ � ������� ���
                colWin = false;
                break;
            }
        }
        if (colWin) return true; // ���� ��� ������ ������� �������� ������, �� ��� ������
    }

    // �������� ������� ���������
    bool mainDiagonalWin = true; // ����, ������������ ������ � ������� ���������
    for (int i = 0; i < gridSize; ++i) {
        if (board[i * gridSize + i] != 2) { // ���� ���� �� ���� ������ �� ������� ��������� �� �������� �����, �� ������ � ��� ���
            mainDiagonalWin = false;
            break;
        }
    }
    if (mainDiagonalWin) return true; // ���� ��� ������ �� ������� ��������� �������� ������, �� ��� ������

    // �������� �������� ���������
    bool secondaryDiagonalWin = true; // ����, ������������ ������ � �������� ���������
    for (int i = 0; i < gridSize; ++i) {
        if (board[i * gridSize + (gridSize - 1 - i)] != 2) { // ���� ���� �� ���� ������ �� �������� ��������� �� �������� �����, �� ������ � ��� ���
            secondaryDiagonalWin = false;
            break;
        }
    }
    if (secondaryDiagonalWin) return true; // ���� ��� ������ �� �������� ��������� �������� ������, �� ��� ������

    return false; // ���� �� ���� �� ������� �� ���������, �� ������ ������� ���
}

// ������� ��� �������� �����
bool CheckDraw() {
    // ���������, �������� �� ��� ������ ������

    for (int i = 0; i < gridSize * gridSize; ++i) {
        if (board[i] == 0)
            return false; // ������� ������ ������, ���� �� ��������
    }
    // ���� ��� ������ ���������, � ��� ����������, �� ��� �����
    return true;
}

// ������� �������� ���� ������� ����� ����������� � ��������� ������
DWORD WINAPI background(LPVOID)
{
    while (true)
    {
        // ������ ����� ���� � ���������������� ����
        config.bg_color.r = rand() % 256;
        config.bg_color.g = rand() % 256;
        config.bg_color.b = rand() % 256;

        hBackgroundBrush = CreateSolidBrush(RGB(config.bg_color.r, config.bg_color.g, config.bg_color.b));
        // ��������� ����� ����� ��� ����
        SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hBackgroundBrush);
        // ������ ����������� ���� � ����� �����
        InvalidateRect(hwnd, NULL, TRUE);
        Sleep(500);
    }
    return 0;
}

// �������� ������� ��������� ��������� ����
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    switch (message)
    {
    case WM_DESTROY:
        // ��������� ������������
        if (str == "m1") SaveConfig1(config);
        else if (str == "m2") SaveConfig2(config);
        else if (str == "m3") SaveConfig3(config);
        else if (str == "m4") SaveConfig4(config);

        // ���������� ������ ���������� ��� �������� ����
        PostQuitMessage(0);
        // ������������ ���������� ������
        CloseHandle(backThread);
        UnmapViewOfFile(board);
        UnmapViewOfFile(commandsArray);
        return 0;
    case WM_PAINT:
    {
        // ��������� ��������� � ������������� ����������� ����
        PAINTSTRUCT ps;
        hdc = BeginPaint(hwnd, &ps);

        // ������� ���� ������ ����
        FillRect(hdc, &ps.rcPaint, hBackgroundBrush);

        // ��������� ����� � �������� �� ������� ����
        DrawGrid(hdc);

        // ������ �������� ����� ������� ������
        WaitForSingleObject(hMutex, INFINITE);

        // �������� ��������� � ����������� ������
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

        // ������������ �������� ����� ������ ������
        ReleaseMutex(hMutex);

        // ���������� �����������
        EndPaint(hwnd, &ps);
    }
    return 0;
    case WM_SIZE:
    {
        // ����������� �������� ���� ����� ��������� ��� �������
        RECT rcClient;
        GetWindowRect(hwnd, &rcClient);
        config.width = rcClient.right - rcClient.left;
        config.height = rcClient.bottom - rcClient.top;

        // ������ ����������� ����
        InvalidateRect(hwnd, NULL, TRUE);
    }
    return 0;
    case WM_TIMER:
    {
        // ��������� ������� ��� �������������� ���������� ����������
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

                    // ������� ������� ��������
                    DWORD dwWaitResult = WaitForSingleObject(hMutex, INFINITE);

                    if (dwWaitResult == WAIT_OBJECT_0)
                    {
                        // ��������, ��� ������ �� ������
                        if (board[index] == 0)
                        {
                            board[index] = 1; // ���������� 1 ��� ��������
                            commandsArray[2]++;

                            // ������ ����������� ���� ����� ��������� �������� ����
                            InvalidateRect(hwnd, NULL, TRUE);
                        }

                        // ������������ ��������
                        ReleaseMutex(hMutex);
                    }
                    if (CheckCrossWin())
                    {
                        commandsArray[5] = 1;
                        MessageBox(NULL, L"�������� ��������", L"���������", MB_OK | MB_ICONINFORMATION);
                        break;
                    }
                    if (CheckDraw())
                    {
                        commandsArray[5] = 1;
                        MessageBox(NULL, L"�����", L"���������", MB_OK | MB_ICONINFORMATION);
                    }

                }
                else
                    MessageBox(NULL, L"��� �������", L"������", MB_OK | MB_ICONINFORMATION);
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

                    // ������� ������� ��������
                    DWORD dwWaitResult = WaitForSingleObject(hMutex, INFINITE);

                    if (dwWaitResult == WAIT_OBJECT_0)
                    {
                        // ��������, ��� ������ �� ������
                        if (board[index] == 0)
                        {
                            board[index] = 2; // ���������� 2 ��� �����
                            commandsArray[2]++;

                            // ������ ����������� ���� ����� ��������� �������� ����
                            InvalidateRect(hwnd, NULL, TRUE);
                        }

                        // ������������ ��������
                        ReleaseMutex(hMutex);
                    }
                    if (CheckCircleWin())
                    {
                        commandsArray[5] = 1;
                        MessageBox(NULL, L"������ ��������", L"���������", MB_OK | MB_ICONINFORMATION);
                        break;
                    }
                    if (CheckDraw())
                    {
                        commandsArray[5] = 1;
                        MessageBox(NULL, L"�����", L"���������", MB_OK | MB_ICONINFORMATION);
                    }
                }
                else
                    MessageBox(NULL, L"��� ���������", L"������", MB_OK | MB_ICONINFORMATION);
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

                    // ������� ������� ��������
                    DWORD dwWaitResult = WaitForSingleObject(hMutex, INFINITE);

                    if (dwWaitResult == WAIT_OBJECT_0)
                    {
                        // ��������, ��� ������ �� ������
                        if (board[index] == 0)
                        {
                            board[index] = 1; // ���������� 1 ��� ��������
                            commandsArray[2]++;

                            // ������ ����������� ���� ����� ��������� �������� ����
                            InvalidateRect(hwnd, NULL, TRUE);
                        }

                        // ������������ ��������
                        ReleaseMutex(hMutex);
                    }
                    if (CheckCrossWin())
                    {
                        commandsArray[5] = 1;
                        MessageBox(NULL, L"�������� ��������", L"���������", MB_OK | MB_ICONINFORMATION);
                        break;
                    }
                    if (CheckDraw())
                    {
                        commandsArray[5] = 1;
                        MessageBox(NULL, L"�����", L"���������", MB_OK | MB_ICONINFORMATION);
                    }

                }
                else
                    MessageBox(NULL, L"��� �������", L"������", MB_OK | MB_ICONINFORMATION);
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

                    // ������� ������� ��������
                    DWORD dwWaitResult = WaitForSingleObject(hMutex, INFINITE);

                    if (dwWaitResult == WAIT_OBJECT_0)
                    {
                        // ��������, ��� ������ �� ������
                        if (board[index] == 0)
                        {
                            board[index] = 2; // ���������� 2 ��� �����
                            commandsArray[2]++;

                            // ������ ����������� ���� ����� ��������� �������� ����
                            InvalidateRect(hwnd, NULL, TRUE);
                        }

                        // ������������ ��������
                        ReleaseMutex(hMutex);
                    }
                    if (CheckCircleWin())
                    {
                        commandsArray[5] = 1;
                        MessageBox(NULL, L"������ ��������", L"���������", MB_OK | MB_ICONINFORMATION);
                        break;
                    }
                    if (CheckDraw())
                    {
                        commandsArray[5] = 1;
                        MessageBox(NULL, L"�����", L"���������", MB_OK | MB_ICONINFORMATION);
                    }
                }
                else
                    MessageBox(NULL, L"��� ���������", L"������", MB_OK | MB_ICONINFORMATION);
            }
        }
    }
    return 0;
    case WM_KEYDOWN:
    {
        // ��������� ��������� � ������� ������
        if ((wParam == 'C' || wParam == 'c') && (GetKeyState(VK_SHIFT) & 0x8000))
        {
            // ������ Notepad ��� ������� "Shift + C"
            RunNotepad();
        }
        else if (wParam == VK_RETURN)
        {
            // ������ ����� ���� � ���������������� ����
            config.bg_color.r = rand() % 256;
            config.bg_color.g = rand() % 256;
            config.bg_color.b = rand() % 256;

            // ��������� ����� ���� �� ��������� ��� ������� "Enter"
            hBackgroundBrush = CreateSolidBrush(RGB(config.bg_color.r, config.bg_color.g, config.bg_color.b));
            // ��������� ����� ����� ��� ����
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hBackgroundBrush);
            // ������ ����������� ���� � ����� �����
            InvalidateRect(hwnd, NULL, TRUE);
        }
        else if (wParam == VK_ESCAPE || ((wParam == 'Q' || wParam == 'q') && (GetKeyState(VK_CONTROL) & 0x8000)))
        {
            // ���������� ������ ���������� ��� ������� "Ctrl + Q" ��� "Esc"
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

                SuspendThread(backThread); // ��������� ���������� ������

                check = 2;
            }
            else if (check == 2)
            {

                ResumeThread(backThread); // ��������� ���������� ������

                check = 1;
            }
        }
        else if (wParam == '1')
        {
            if (check > 0)
            {
                SetThreadPriority(backThread, THREAD_PRIORITY_IDLE);
                int currentPriority = GetThreadPriority(backThread);
                cout << "��������� ������: " << currentPriority << " ����� ������ ���������." << endl << endl;
            }
            else
            {
                cout << "��������� ���������� ����������." << endl;
            }
        }
        else if (wParam == '2')
        {
            if (check > 0)
            {
                SetThreadPriority(backThread, THREAD_PRIORITY_LOWEST);
                int currentPriority = GetThreadPriority(backThread);
                cout << "��������� ������: " << currentPriority << " ������ ���������." << endl << endl;
            }
            else
            {
                cout << "��������� ���������� ����������." << endl;
            }
        }
        else if (wParam == '3')
        {
            if (check > 0)
            {
                SetThreadPriority(backThread, THREAD_PRIORITY_BELOW_NORMAL);
                int currentPriority = GetThreadPriority(backThread);
                cout << "��������� ������: " << currentPriority << " ���� �������� ����������." << endl << endl;
            }
            else
            {
                cout << "��������� ���������� ����������." << endl;
            }
        }
        else if (wParam == '4')
        {
            if (check > 0)
            {
                SetThreadPriority(backThread, THREAD_PRIORITY_NORMAL);
                int currentPriority = GetThreadPriority(backThread);
                cout << "��������� ������: " << currentPriority << " ����������� ���������." << endl << endl;
            }
            else
            {
                cout << "��������� ���������� ����������." << endl;
            }
        }
        else if (wParam == '5')
        {
            if (check > 0)
            {
                SetThreadPriority(backThread, THREAD_PRIORITY_ABOVE_NORMAL);
                int currentPriority = GetThreadPriority(backThread);
                cout << "��������� ������: " << currentPriority << " ���� �������� ����������." << endl << endl;
            }
            else
            {
                cout << "��������� ���������� ����������." << endl;
            }
        }
        else if (wParam == '6')
        {
            if (check > 0)
            {
                SetThreadPriority(backThread, THREAD_PRIORITY_HIGHEST);
                int currentPriority = GetThreadPriority(backThread);
                cout << "��������� ������: " << currentPriority << " ����� ������� ���������." << endl << endl;
            }
            else
            {
                cout << "��������� ���������� ����������." << endl;
            }
        }
        else if (wParam == '7')
        {
            if (check > 0)
            {
                SetThreadPriority(backThread, THREAD_PRIORITY_TIME_CRITICAL);
                int currentPriority = GetThreadPriority(backThread);
                cout << "��������� ������: " << currentPriority << " ����������� �� ������� ���������." << endl << endl;
            }
            else
            {
                cout << "��������� ���������� ����������." << endl;
            }
        }
        break;
    }
    return 0;
    case WM_MOUSEWHEEL:
    {
        // ��������� ��������� �� ������ ���� ��� ��������� ����� ����� �����
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        int steps = delta / WHEEL_DELTA * 10;  // ���������� ����������������

        // ��������� �������� ��������� ����������
        HDC hdc = GetDC(hwnd);

        // ��������� �������� ����� ����� �����
        COLORREF currentColor;
        if (GetObject(hGridPen, sizeof(logpen), &logpen))
        {
            currentColor = logpen.lopnColor;
        }
        else
        {
            // ���� �� ������� �������� ������� ����, ���������� �������
            currentColor = RGB(255, 0, 0);
        }

        // ��������� ����� ����� ����� � ����������� �� ���������� �����
        config.grid_line_color.r = (GetRValue(currentColor) + steps) % 256;
        config.grid_line_color.g = (GetGValue(currentColor) + steps) % 256;
        config.grid_line_color.b = (GetBValue(currentColor) + steps) % 256;

        // �������� ����� ����� � ���������� ������
        hGridPen = CreatePen(PS_SOLID, 2, RGB(config.grid_line_color.r, config.grid_line_color.g, config.grid_line_color.b));

        // ������ ����������� ���� � ����� ������ ����� �����
        InvalidateRect(hwnd, NULL, TRUE);

        // ������������ ��������� ����������
        ReleaseDC(hwnd, hdc);
    }
    return 0;
    }

    // ����� ����������� ������� ��������� ��������� ��� �������������� �������
    return DefWindowProc(hwnd, message, wParam, lParam);
}