#pragma once


typedef struct {
    int x;
    int y;
} Vector2i;

typedef struct {
    int width;
    int height;
} Size2i;

typedef struct {
    int screen_width;
    int screen_height;
    int api_version;
    int bitsPerPixel;
    BOOLEAN ok;

} ClientSettings;

typedef struct {
    int screen_width;
    int screen_height;
    int api_version;
    BOOLEAN ok;

} ServerSettings;

typedef struct {
    SOCKET socket;
    struct sockaddr_in addr_in;
} MySocket;

typedef enum {
    MYSOCKER_OK,
    MYSOCKER_INVALID_IP,
    MYSOCKER_INVALID_PORT,
    MYSOCKER_NOT_INITIALIZED,
    MYSOCKER_ACCEPT_ERROR,
    MYSOCKER_CLIENT_HAS_DISCONNECTED,
    MYSOCKER_INVALID_SOCKET,

    // Other - is WSA error

} MySocketError;
