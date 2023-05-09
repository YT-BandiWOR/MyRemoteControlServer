#define UNICODE
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <stdlib.h>
#include <limits.h>
#include <remotelib.h>

#pragma comment(lib, "ws2_32.lib")


LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SubWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI StartNewWindow(LPVOID lParam);

HANDLE newWindowHandleThread;
HWND hChildWindow;

HINSTANCE hMainInstance;
Size2i screen_size;

ClientSettings client_settings;

struct sockaddr_in server_addr_controls, client_addr_controls, server_addr_image, client_addr_image;
SOCKET server_socket_controls, server_socket_image, client_socket_controls, client_socket_image;


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    // Инициализация библиотеки WinSock2
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        MessageBox(NULL, L"Ошибка WSAStartup", L"Ошибка", MB_OK);
        return 1;
    }

    // Регистрация класса окна
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = MainWndProc; // Установка функции обработки сообщений
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"MyWindowClass";

    hMainInstance = hInstance;

    screen_size = GetScreenSize();

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"Failed to register window class", L"Error", MB_ICONERROR);
        return 1;
    }

    // Создание окна
    HWND hWnd = CreateWindow(
        L"MyWindowClass", // Имя класса окна
        L"MyRemoteControl Server", // Заголовок окна
        WS_OVERLAPPEDWINDOW, // Стиль окна
        0, 0, // Позиция окна
        220, 400, // Размер окна
        NULL, // Родительское окно
        NULL, // Нет меню
        hInstance, // Дескриптор приложения
        NULL // Дополнительные данные окна
    );

    if (hWnd == NULL) {
        MessageBox(NULL, L"Failed to create window", L"Error", MB_ICONERROR);
        return 1;
    }

    // Отображение окна
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Основной цикл обработки сообщений
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        WSACleanup();
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        if (MessageBox(hWnd, L"Вы уверены, что хотите выйти?", L"Выход", MB_YESNO) != IDYES)
            return 0;

        SendMessage(hWnd, WM_DESTROY, wParam, lParam);

        break;
    case WM_CREATE:
    {
        CreateWindow(
            L"STATIC", L"Введите IP",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 20, 100, 20,
            hWnd, NULL, hMainInstance, NULL
        );

        HWND hIpEdit = CreateWindow(L"EDIT", L"127.0.0.1",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
            20, 40, 150, 20,
            hWnd, (HMENU)ID_IP_EDIT, hMainInstance, NULL);

        CreateWindow(
            L"STATIC", L"Порт №1",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 70, 100, 20,
            hWnd, NULL, hMainInstance, NULL
        );

        HWND hPortControlsEdit = CreateWindow(L"EDIT", L"15678",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
            20, 90, 150, 20,
            hWnd, (HMENU)ID_PORT_CONTROLS_EDIT, hMainInstance, NULL);

        CreateWindow(
            L"STATIC", L"Порт №2",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 120, 100, 20,
            hWnd, NULL, hMainInstance, NULL
        );

        HWND hPortImageEdit = CreateWindow(L"EDIT", L"15679",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
            20, 150, 150, 20,
            hWnd, (HMENU)ID_PORT_IMAGE_EDIT, hMainInstance, NULL);

        CreateWindow(L"BUTTON", L"Создать сервер",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 180, 150, 25,
            hWnd, (HMENU)ID_CREATE_BTN, NULL, NULL);

        HWND hStateStatic = CreateWindow(L"STATIC", L"Нет подключения",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 210, 150, 25,
            hWnd, (HMENU)ID_STATE_STATIC, hMainInstance, NULL
        );

        HWND hStartBtn = CreateWindow(L"BUTTON", L"Начать стриминг",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 240, 150, 25,
            hWnd, (HMENU)ID_START_BTN, hMainInstance, NULL);

        HWND hStopBtn = CreateWindow(L"BUTTON", L"Отключить клиента",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 270, 150, 25,
            hWnd, (HMENU)ID_STOP_BTN, hMainInstance, NULL);

        EnableWindow(hStateStatic, FALSE);
        EnableWindow(hStartBtn, FALSE);
        EnableWindow(hStopBtn, FALSE);
    }
        break;

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case ID_CREATE_BTN:
        {
            WCHAR ip_text[32];
            GetDlgItemText(hWnd, ID_IP_EDIT, ip_text, 32);

            WCHAR port_controls_text[16];
            GetDlgItemText(hWnd, ID_PORT_CONTROLS_EDIT, port_controls_text, 16);

            WCHAR port_image_text[16];
            GetDlgItemText(hWnd, ID_PORT_IMAGE_EDIT, port_image_text, 16);

            int port_controls_number = _wtoi(port_controls_text);
            int port_image_number = _wtoi(port_image_text);
            
            WCHAR error_message[256];
            if ((port_controls_number <= 0 || port_controls_number > USHRT_MAX) ||
                (port_image_number <= 0 || port_image_number > USHRT_MAX)
                ) {
                wsprintf(error_message, L"Порты %d & %d и/или не валидны. Условие: 0 <= port <= 65535", port_controls_number, port_image_number);
                MessageBox(hWnd, error_message, L"Ошибка", MB_OK);
                return 0;
            }

            struct in_addr server_addr_controls_ip;
            BOOL iResultIp1 = InetPton(AF_INET, ip_text, &server_addr_controls_ip);
            struct in_addr server_addr_image_ip;
            BOOL iResultIp2 = InetPton(AF_INET, ip_text, &server_addr_image_ip);

            if (!(iResultIp1 == 1 && iResultIp2 == 1))
            {
                wsprintf(error_message, L"IP-адреса %ws & %ws и/или не валидны. Требуемый формат x.x.x.x , где x: 0 <= x <= 255", ip_text, ip_text);
            }

            server_socket_controls =    socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            server_socket_image    =    socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

            memset(&server_addr_controls, 0, sizeof(server_addr_controls));
            memset(&server_addr_image, 0, sizeof(server_addr_image));

            server_addr_controls.sin_family = AF_INET;
            server_addr_image.sin_family = AF_INET;

            server_addr_controls.sin_addr.s_addr = server_addr_controls_ip.s_addr;
            server_addr_image.sin_addr.s_addr = server_addr_image_ip.s_addr;

            server_addr_controls.sin_port = htons(port_controls_number);
            server_addr_image.sin_port = htons(port_image_number);

            HWND hCreateButton = GetDlgItem(hWnd, ID_CREATE_BTN);

            EnableWindow(hCreateButton, FALSE);

            BOOL iResultBindControls = bind(server_socket_controls, (struct sockaddr*)&server_addr_controls, sizeof(server_addr_controls));
            BOOL iResultBindImage = bind(server_socket_image, (struct sockaddr*)&server_addr_image, sizeof(server_addr_image));

            if (iResultBindControls == SOCKET_ERROR || iResultBindImage == SOCKET_ERROR)
            {
                wsprintf(error_message, L"Ошибка, не удалось привязать сокет сервера к адресу: %d", WSAGetLastError());
                MessageBox(hWnd, error_message, L"Ошибка", MB_OK);
                closesocket(server_socket_controls);
                closesocket(server_socket_image);
                return 0;
            }

            BOOL iResultListen = listen(server_socket_controls, SOMAXCONN);
            if (iResultListen == SOCKET_ERROR)
            {
                wsprintf(error_message, L"Не удалось прослушать сокет сервера: %d", WSAGetLastError());
                MessageBox(hWnd, error_message, L"Ошибка", MB_OK);
                closesocket(server_socket_controls);
                closesocket(server_socket_image);
                return 0;
            }

            int clientAddrSize = sizeof(client_addr_controls);
            while (1)
            {
                client_socket_controls = accept(server_socket_controls, (struct sockaddr*)&client_addr_controls, &clientAddrSize);

                if (client_socket_controls == INVALID_SOCKET)
                {
                    wsprintf(error_message, L"Ошибка: не удалось принять подключение от клиента: %d", WSAGetLastError());
                    MessageBox(hWnd, error_message, L"Ошибка", MB_OK);
                    return 0;
                }

                char recvBuf[1024];
                int recvSize = recv(client_socket_controls, recvBuf, sizeof(recvBuf), 0);

                if (recvSize == sizeof(client_settings))
                {
                    memcpy(&client_settings, recvBuf, sizeof(client_settings));

                    ServerSettings server_settings;
                    server_settings.api_version = API_VERSION;
                    server_settings.screen_width = screen_size.width;
                    server_settings.screen_height = screen_size.height;

                    int bufferSize = sizeof(server_settings);

                    int bytesSent = send(client_socket_controls, (char*)&server_settings, bufferSize, NULL);
                    if (bytesSent == SOCKET_ERROR)
                    {
                        wsprintf(error_message, L"Не удалось отправить данные клиенту");
                        MessageBox(hWnd, error_message, L"Ошибка", MB_OK);
                        return 0;
                    }

                    MessageBox(hWnd, L"Соединение установлено.", L"Успешно", MB_OK);

                    HWND hStartBtn = GetDlgItem(hWnd, ID_START_BTN);
                    EnableWindow(hStartBtn, TRUE);

                    return 0;
                }
                else {
                    wsprintf(error_message, L"Ошибка: от клиента принято %d байт, но ожидалось %d байт.", recvSize, (int)sizeof(client_settings));
                    MessageBox(hWnd, error_message, L"Ошибка", MB_OK);
                    return 0;
                }
            }
        }
        break;
        case ID_START_BTN:
        {
            DWORD threadId;

            WCHAR error_message[256];

            HWND hStartBtn = GetDlgItem(hWnd, ID_START_BTN);
            HWND hStopBtn = GetDlgItem(hWnd, ID_STOP_BTN);
            EnableWindow(hStartBtn, FALSE);
            EnableWindow(hStopBtn, TRUE);


            BOOL iResultListen = listen(server_socket_image, SOMAXCONN);
            if (iResultListen == SOCKET_ERROR)
            {
                wsprintf(error_message, L"Не удалось прослушать сокет сервера: %d", WSAGetLastError());
                MessageBox(hWnd, error_message, L"Ошибка", MB_OK);
                EnableWindow(hStartBtn, FALSE);
                return 0;
            }

            int clientAddrSize = sizeof(client_addr_image);
            while (1)
            {
                client_socket_image = accept(server_socket_image, (struct sockaddr*)&client_addr_image, &clientAddrSize);

                if (client_socket_image == INVALID_SOCKET)
                {
                    wsprintf(error_message, L"Ошибка: не удалось принять подключение [видеопоток] от клиента: %d", WSAGetLastError());
                    MessageBox(hWnd, error_message, L"Ошибка", MB_OK);
                    return 0;
                }
                
                // Регистрация класса окна
                WNDCLASS wc = { 0 };
                wc.lpfnWndProc = SubWndProc; // Установка функции обработки сообщений
                wc.hInstance = hMainInstance;
                wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
                wc.lpszClassName = L"MyWindowClass2";

                if (!RegisterClass(&wc)) {
                    MessageBox(NULL, L"Failed to register window class", L"Error", MB_ICONERROR);
                    return 1;
                }

                // Создание окна
                hChildWindow = CreateWindow(
                    L"MyWindowClass2", // Имя класса окна
                    L"MyRemoteControl Server", // Заголовок окна
                    WS_OVERLAPPEDWINDOW, // Стиль окна
                    0, 0, // Позиция окна
                    screen_size.width, screen_size.height, // Размер окна
                    NULL, // Родительское окно
                    NULL, // Нет меню
                    hMainInstance, // Дескриптор приложения
                    NULL // Дополнительные данные окна
                );

                if (hChildWindow == NULL) {
                    MessageBox(NULL, L"Failed to create window", L"Error", MB_ICONERROR);
                    return 1;
                }

                // Отображение окна
                ShowWindow(hChildWindow, SW_MAXIMIZE);
                UpdateWindow(hChildWindow);

                // Основной цикл обработки сообщений
                MSG msg;
                while (GetMessage(&msg, NULL, 0, 0)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                return 0;
            }
        }
        break;
        case ID_STOP_BTN:
            SendMessage(hChildWindow, WM_CLOSE, NULL, NULL);
            break;
        }
    }
        break;


    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

LRESULT SubWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        closesocket(client_socket_image);
        closesocket(client_socket_controls);
        closesocket(server_socket_image);
        closesocket(server_socket_controls);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}

DWORD WINAPI StartNewWindow(LPVOID lParam) {
    
}
