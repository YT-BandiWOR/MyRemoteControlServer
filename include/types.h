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
} ClientSettings;

typedef struct {
    int screen_width;
    int screen_height;
    int api_version;

} ServerSettings;
