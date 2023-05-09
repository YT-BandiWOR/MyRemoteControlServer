#define UNICODE
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <remotelib.h>

#pragma comment(lib, "ws2_32.lib")


LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SubWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI StartNewWindow(LPVOID lParam);

HANDLE newWindowHandleThread;
HWND hChildWindow;

HINSTANCE hMainInstance;
Size2i screen_size;

ServerSettings server_settings;
ClientSettings client_settings;

MySocket server_controls_socket, server_videostream_socket, client_controls_socket, client_videostream_socket;


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    // Инициализация библиотеки WinSock2
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        MessageBox(NULL, L"Ошибка WSAStartup", L"Ошибка", MB_OK);
        return 1;
    }

    if (!RegisterClientMainWndClass(hInstance, MainWndProc))
    {
        MessageBox(NULL, L"Failed to register window class", L"Error", MB_ICONERROR);
        return 1;
    }

    HWND hMainWnd = CreateClientMainWindow(0, 0, 220, 400);

    if (hMainWnd == NULL)
    {
        MessageBox(NULL, L"Failed to create window", L"Error", MB_ICONERROR);
        return 1;
    }

    // Отображение окна
    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

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
        screen_size = GetScreenSize();

        CreateWindow(
            L"STATIC", L"Введите IP",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 20, 100, 20,
            hWnd, NULL, hMainInstance, NULL
        );

        CreateWindow(L"EDIT", DEFAULT_IP,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
            20, 40, 150, 20,
            hWnd, (HMENU)ID_IP_EDIT, hMainInstance, NULL);

        CreateWindow(
            L"STATIC", L"Порт №1",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 70, 100, 20,
            hWnd, NULL, hMainInstance, NULL
        );

        CreateWindow(L"EDIT", DEFAULT_PORT_CONTROLS,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
            20, 90, 150, 20,
            hWnd, (HMENU)ID_PORT_CONTROLS_EDIT, hMainInstance, NULL);

        CreateWindow(
            L"STATIC", L"Порт №2",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 120, 100, 20,
            hWnd, NULL, hMainInstance, NULL
        );

        CreateWindow(L"EDIT", DEFAULT_PORT_IMAGE,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
            20, 150, 150, 20,
            hWnd, (HMENU)ID_PORT_IMAGE_EDIT, hMainInstance, NULL);

        CreateWindow(L"BUTTON", L"Подключиться",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 180, 150, 25,
            hWnd, (HMENU)ID_CREATE_BTN, NULL, NULL);

        CreateWindow(L"STATIC", L"Нет подключения",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 210, 150, 25,
            hWnd, (HMENU)ID_STATE_STATIC, hMainInstance, NULL
        );

        CreateWindow(L"BUTTON", L"Начать стриминг",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 240, 150, 25,
            hWnd, (HMENU)ID_START_BTN, hMainInstance, NULL);

        CreateWindow(L"BUTTON", L"Отключиться",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 270, 150, 25,
            hWnd, (HMENU)ID_STOP_BTN, hMainInstance, NULL);

        EnableDlgItem(hWnd, ID_START_BTN, FALSE);
        EnableDlgItem(hWnd, ID_STOP_BTN, FALSE);
    }
    break;

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case ID_CREATE_BTN:
        {
            EnableDlgItem(hWnd, ID_CREATE_BTN, FALSE);

            // Defines buffers
            WCHAR ip_text[32];
            WCHAR port_controls_text[16];
            WCHAR port_image_text[16];

            // Get edit TEXTS
            GetDlgItemText(hWnd, ID_IP_EDIT, ip_text, 32);
            GetDlgItemText(hWnd, ID_PORT_CONTROLS_EDIT, port_controls_text, 16);
            GetDlgItemText(hWnd, ID_PORT_IMAGE_EDIT, port_image_text, 16);

            // Convert port string to int
            intmax_t port_controls = _wtoi64(port_controls_text);
            intmax_t port_image = _wtoi64(port_image_text);

            // Variable for my socket error code
            MySocketError err_code;

            // Init controls socket
            err_code = InitMySocketClient(&client_controls_socket, AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (MySocketMessageIfError(hWnd, err_code)) {
                CloseMySocket(&client_controls_socket);
                CloseMySocket(&client_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                return 0;
            }

            // Init videostream socket
            err_code = InitMySocketClient(&client_videostream_socket, AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (MySocketMessageIfError(hWnd, err_code)) {
                CloseMySocket(&client_controls_socket);
                CloseMySocket(&client_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                return 0;
            }

            err_code = InitMySocketConnectedServer(&server_controls_socket, AF_INET, SOCK_STREAM, IPPROTO_TCP, ip_text, port_controls);
            if (MySocketMessageIfError(hWnd, err_code)) {
                CloseMySocket(&client_controls_socket);
                CloseMySocket(&client_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                return 0;
            }

            err_code = InitMySocketConnectedServer(&server_videostream_socket, AF_INET, SOCK_STREAM, IPPROTO_TCP, ip_text, port_image);
            if (MySocketMessageIfError(hWnd, err_code)) {
                CloseMySocket(&client_controls_socket);
                CloseMySocket(&client_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                return 0;
            }

            // Connecting to server
            if (MySocketMessageIfError(hWnd,
                ConnectMySocket(&server_controls_socket, &client_controls_socket)
                ))
            {
                CloseMySocket(&client_controls_socket);
                CloseMySocket(&client_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                return 0;
            }

            client_settings.screen_width = screen_size.width;
            client_settings.screen_height = screen_size.height;
            client_settings.api_version = API_VERSION;
            client_settings.ok = TRUE;

            int send_size;
            if (MySocketMessageIfError(hWnd,
                SendMySocket(&client_controls_socket, (LPSTR)&client_settings, sizeof(client_settings), &send_size, NULL)
                ))
            {
                CloseMySocket(&server_controls_socket);
                CloseMySocket(&server_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                return 0;
            }

            int recv_size;
            if (MySocketMessageIfError(hWnd,
                RecvMySocket(&client_controls_socket, (LPSTR)&server_settings, sizeof(server_settings), &recv_size, NULL)
                ))
            {
                CloseMySocket(&server_controls_socket);
                CloseMySocket(&server_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                return 0;
            }

            if (recv_size != sizeof(server_settings) || server_settings.api_version != client_settings.api_version || !(server_settings.ok))
            {
                WCHAR error_message[200];
                wsprintf(error_message, L"Ошибка при получении данных сервера. Получено: %d, ожидалось: %d байтов. Версия клиента: %d, сервера: %d.", recv_size, (int)sizeof(server_settings), client_settings.api_version, server_settings.api_version);
                CloseMySocket(&client_controls_socket);
                CloseMySocket(&client_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                MessageBox(hWnd, error_message, L"Ошибка", MB_OK);
                return 0;
            }

            // Connecting to server [videostream]
            if (MySocketMessageIfError(hWnd,
                ConnectMySocket(&server_videostream_socket, &client_videostream_socket)
            ))
            {
                CloseMySocket(&client_controls_socket);
                CloseMySocket(&client_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                return 0;
            }

            MessageBox(hWnd, L"Соединение установлено.", L"Подключение к серверу", MB_OK);
            EnableDlgItem(hWnd, ID_CREATE_BTN, FALSE);
            EnableDlgItem(hWnd, ID_START_BTN, TRUE);
            SetDlgItemText(hWnd, ID_STATE_STATIC, L"Соединено");
        }
        break;
        case ID_STOP_BTN:
            SendMessage(hChildWindow, WM_DESTROY, NULL, NULL);
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
        //closesocket(client_socket_image);
        //closesocket(client_socket_controls);
        //closesocket(server_socket_image);
        //closesocket(server_socket_controls);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}

DWORD WINAPI StartNewWindow(LPVOID lParam) {

}
