#pragma once


BOOL RegisterServerMainWndClass(HINSTANCE hInstance, WNDPROC windowProc) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = windowProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"MyWindowClass";

    if (!RegisterClass(&wc))
        return FALSE;

    return TRUE;
}

HWND CreateServerMainWindow(int x, int y, int width, int height) {
    return CreateWindow(
        L"MyWindowClass", // Имя класса окна
        L"MyRemoteControl Server", // Заголовок окна
        WS_OVERLAPPEDWINDOW, // Стиль окна
        x, y, // Позиция окна
        width, height, // Размер окна
        NULL, // Родительское окно
        NULL, // Нет меню
        NULL, // Дескриптор приложения
        NULL // Дополнительные данные окна
    );
}


BOOL RegisterServerSubClass(HINSTANCE hInstance, WNDPROC windowProc) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = windowProc; // Установка функции обработки сообщений
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"MyWindowClass2";

    if (!RegisterClass(&wc))
    {
        return FALSE;
    }

    return TRUE;
}


HWND CreateServerStreamWindow(int x, int y, int width, int height) {
    return CreateWindow(
        L"MyWindowClass2", // Имя класса окна
        L"MyRemoteControl Server [video]", // Заголовок окна
        WS_OVERLAPPEDWINDOW, // Стиль окна
        x, y, // Позиция окна
        width, height, // Размер окна
        NULL, // Родительское окно
        NULL, // Нет меню
        NULL, // Дескриптор приложения
        NULL // Дополнительные данные окна
    );
}

BOOL RegisterClientMainWndClass(HINSTANCE hInstance, WNDPROC windowProc) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = windowProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"MyWindowClass";

    if (!RegisterClass(&wc))
        return FALSE;

    return TRUE;
}

HWND CreateClientMainWindow(int x, int y, int width, int height) {
    return CreateWindow(
        L"MyWindowClass", // Имя класса окна
        L"MyRemoteControl Client", // Заголовок окна
        WS_OVERLAPPEDWINDOW, // Стиль окна
        x, y, // Позиция окна
        width, height, // Размер окна
        NULL, // Родительское окно
        NULL, // Нет меню
        NULL, // Дескриптор приложения
        NULL // Дополнительные данные окна
    );
}