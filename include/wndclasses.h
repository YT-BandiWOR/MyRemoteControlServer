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
        L"MyWindowClass", // ��� ������ ����
        L"MyRemoteControl Server", // ��������� ����
        WS_OVERLAPPEDWINDOW, // ����� ����
        x, y, // ������� ����
        width, height, // ������ ����
        NULL, // ������������ ����
        NULL, // ��� ����
        NULL, // ���������� ����������
        NULL // �������������� ������ ����
    );
}


BOOL RegisterServerSubClass(HINSTANCE hInstance, WNDPROC windowProc) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = windowProc; // ��������� ������� ��������� ���������
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
        L"MyWindowClass2", // ��� ������ ����
        L"MyRemoteControl Server [video]", // ��������� ����
        WS_OVERLAPPEDWINDOW, // ����� ����
        x, y, // ������� ����
        width, height, // ������ ����
        NULL, // ������������ ����
        NULL, // ��� ����
        NULL, // ���������� ����������
        NULL // �������������� ������ ����
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
        L"MyWindowClass", // ��� ������ ����
        L"MyRemoteControl Client", // ��������� ����
        WS_OVERLAPPEDWINDOW, // ����� ����
        x, y, // ������� ����
        width, height, // ������ ����
        NULL, // ������������ ����
        NULL, // ��� ����
        NULL, // ���������� ����������
        NULL // �������������� ������ ����
    );
}