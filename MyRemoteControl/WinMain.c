#define UNICODE
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <remotelib.h>

#pragma comment(lib, "ws2_32.lib")

// Window wnd procedurs
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SubWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Thread workers
DWORD WINAPI RecvVideostream();

// Windows Descriptors
HWND hMainWindow;
HWND hChildWindow;

// App instance
HINSTANCE hMainInstance;

// This device screen size
Size2i screen_size;

// Client & server settings
ClientSettings client_settings;
ServerSettings server_settings;

// Sockets
MySocket server_controls_socket, server_videostream_socket, client_controls_socket, client_videostream_socket;

// Input VideoStream params & data
Size2i videostream_size;
int bits_per_pixel;
int video_data_size;
BYTE* video_data_recv_buffer = { 0 };
BYTE* video_data = { 0 };
BITMAPINFOHEADER videostream_header = { 0 };
BITMAPINFO bmi_video = { 0 };

// Threads
HANDLE recv_video_thread = { 0 };
HANDLE draw_video_thread = { 0 };

CRITICAL_SECTION g_videodata_cs = { 0 };


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    // Инициализация библиотеки WinSock2
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        MessageBox(NULL, L"Ошибка WSAStartup", L"Ошибка", MB_OK);
        return 1;
    }

    if (!RegisterServerMainWndClass(hInstance, MainWndProc))
    {
        MessageBox(NULL, L"Failed to register window class", L"Error", MB_ICONERROR);
        return 1;
    }

    if (!RegisterServerSubClass(hInstance, SubWndProc))
    {
        MessageBox(NULL, L"Failed to register window class [video]", L"Error", MB_ICONERROR);
        return 1;
    }


    hMainWindow = CreateServerMainWindow(0, 0, 220, 400);

    if (hMainWindow == NULL)
    {
        MessageBox(NULL, L"Failed to create window", L"Error", MB_ICONERROR);
        return 1;
    }

    // Отображение окна
    ShowWindow(hMainWindow, nCmdShow);
    UpdateWindow(hMainWindow);

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

        CreateWindow(L"BUTTON", L"Создать сервер",
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

        CreateWindow(L"BUTTON", L"Отключить клиента",
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
            err_code = InitMySocket(&server_controls_socket, AF_INET, SOCK_STREAM, IPPROTO_TCP, ip_text, port_controls);
            if (MySocketMessageIfError(hWnd, err_code)) {
                CloseMySocket(&server_controls_socket);
                CloseMySocket(&server_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                return 0;
            }
            
            // Init videostream socket
            err_code = InitMySocket(&server_videostream_socket, AF_INET, SOCK_STREAM, IPPROTO_TCP, ip_text, port_image);
            if (MySocketMessageIfError(hWnd, err_code)) {
                CloseMySocket(&server_controls_socket);
                CloseMySocket(&server_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                return 0;
            }
            
            // Set zero memory for clients socket
            SetZeroMySocket(&client_controls_socket);
            SetZeroMySocket(&client_videostream_socket);

            // Listening controls socket
            if (MySocketMessageIfError(hWnd,
                ListenMySocket(&server_controls_socket, SOMAXCONN))
            ) {
                CloseMySocket(&server_controls_socket);
                CloseMySocket(&server_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                return 0;
            }

            // Listening videostream socket
            if (MySocketMessageIfError(hWnd,
                ListenMySocket(&server_videostream_socket, SOMAXCONN))
            ) {
                CloseMySocket(&server_controls_socket);
                CloseMySocket(&server_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                return 0;
            }

            // Accept client
            if (MySocketMessageIfError(hWnd,
                AcceptMySocket(&server_controls_socket, &client_controls_socket))
            ) {
                CloseMySocket(&server_controls_socket);
                CloseMySocket(&server_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                return 0;
            }

            int recv_size;
            err_code = RecvMySocket(&client_controls_socket, &client_settings, sizeof(client_settings), &recv_size, 0);
            if (MySocketMessageIfError(hWnd, err_code))
            {
                CloseMySocket(&server_controls_socket);
                CloseMySocket(&server_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                return 0;
            }

            // This app settings
            server_settings.api_version = API_VERSION;
            server_settings.screen_width = screen_size.width;
            server_settings.screen_height = screen_size.height;

            if (recv_size == sizeof(client_settings))
            {
                server_settings.ok = TRUE;
            }
            else {
                server_settings.ok = FALSE;
            }

            int send_size;
            if (MySocketMessageIfError(hWnd, SendMySocket(&client_controls_socket, (LPSTR)&server_settings, sizeof(server_settings), &send_size, 0))) {
                CloseMySocket(&server_controls_socket);
                CloseMySocket(&server_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                return 0;
            }

            if (sizeof(client_settings) != recv_size || (!client_settings.ok))
            {
                WCHAR error_message[200];
                wsprintf(error_message, L"Ошибка при получении данных клиента. Получено: %d, ожидалось: %d байтов. Версия клиента: %d, сервера: %d. Ok: %d", recv_size, (int)sizeof(client_settings), client_settings.api_version, server_settings.api_version);
                CloseMySocket(&server_controls_socket);
                CloseMySocket(&server_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                MessageBox(hWnd, error_message, L"Ошибка", MB_OK);
                return 0;
            }

            // Accept client [videostream]
            if (MySocketMessageIfError(hWnd,
                AcceptMySocket(&server_videostream_socket, &client_videostream_socket))
                ) {
                CloseMySocket(&server_controls_socket);
                CloseMySocket(&server_videostream_socket);
                EnableDlgItem(hWnd, ID_CREATE_BTN, TRUE);
                return 0;
                
            }

            EnableDlgItem(hWnd, ID_CREATE_BTN, FALSE);
            EnableDlgItem(hWnd, ID_START_BTN, TRUE);
            SetDlgItemText(hWnd, ID_STATE_STATIC, L"Соединено");
        }
        break;
        case ID_START_BTN:
        {
            EnableDlgItem(hWnd, ID_START_BTN, FALSE);
            EnableDlgItem(hWnd, ID_STOP_BTN, TRUE);

            hChildWindow = CreateServerStreamWindow(0, 0, screen_size.width, screen_size.height);

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
    case WM_CREATE:
    {
        InitializeCriticalSection(&g_videodata_cs);

        videostream_size.width = client_settings.screen_width;
        videostream_size.height = client_settings.screen_height;
        bits_per_pixel = client_settings.bitsPerPixel;

        video_data_size = videostream_size.width * videostream_size.height * bits_per_pixel / 8;
        video_data = (BYTE*)malloc(video_data_size);
        video_data_recv_buffer = (BYTE*)malloc(video_data_size);


        if (!(video_data || video_data_recv_buffer))
        {
            MessageBox(hWnd, L"Ошибка при выделении памяти.", L"Ошибка", MB_OK);
            return 0;
        }

        bmi_video.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi_video.bmiHeader.biWidth = videostream_size.width;
        bmi_video.bmiHeader.biHeight = -videostream_size.height;
        bmi_video.bmiHeader.biPlanes = 1;
        bmi_video.bmiHeader.biBitCount = bits_per_pixel;
        bmi_video.bmiHeader.biCompression = BI_RGB;

        recv_video_thread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)RecvVideostream, NULL, NULL, NULL);
    }
        break;

    case WM_PAINT:
    {
        HDC hdc = GetDC(hWnd);

        EnterCriticalSection(&g_videodata_cs);

        HBITMAP hBitMap = CreateDIBitmap(hdc, &bmi_video.bmiHeader, CBM_INIT, video_data, &bmi_video, DIB_RGB_COLORS);
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hBitmapOld = (HBITMAP)SelectObject(hdcMem, hBitMap);

        BitBlt(hdc, 0, 0, videostream_size.width, videostream_size.height, hdcMem, 0, 0, SRCCOPY);
        
        LeaveCriticalSection(&g_videodata_cs);

        SelectObject(hdcMem, hBitmapOld);
        DeleteDC(hdcMem);
        DeleteObject(hBitMap);

        ReleaseDC(hWnd, hdc);
    }
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}

DWORD WINAPI RecvVideostream() {
    while (TRUE)
    {
        if (MySocketMessageIfError(hChildWindow,
            RecvMySocketPartial(&client_videostream_socket, video_data_recv_buffer, video_data_size, 0)
        ))
        {
            free(video_data);
            free(video_data_recv_buffer);
            CloseMySocket(&server_controls_socket);
            CloseMySocket(&server_videostream_socket);
            ExitThread(0);
        }

        // Блокирование потока для копирования в глобальные данные
        EnterCriticalSection(&g_videodata_cs);

        memcpy(video_data, video_data_recv_buffer, video_data_size);
        
        LeaveCriticalSection(&g_videodata_cs);

        InvalidateRect(hChildWindow, NULL, FALSE);

        BOOL response_ok = 1;

        int sent_size;
        if (MySocketMessageIfError(hChildWindow,
                SendMySocket(&client_videostream_socket, &response_ok, sizeof(response_ok), &sent_size, 0)
            ))
        {
            free(video_data);
            free(video_data_recv_buffer);
            CloseMySocket(&server_controls_socket);
            CloseMySocket(&server_videostream_socket);
            ExitThread(0);
        }
    }

    return 0;
}
