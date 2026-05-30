#pragma once
#include <GLFW/glfw3.h>
#include "Config.h"

// Объявление шейдеров
extern const char* vertexShaderSource;
extern const char* fragmentShaderSource;
extern const int font3x5[10][5];

// Объявление функций
float getBlockColorR(int type);
float getBlockColorG(int type);
float getBlockColorB(int type);

void drawPixelDigit(int digit, float startPixelX, float startPixelY, float pixelSizeX, float pixelSizeY, int scaleLoc, float aspect);
void drawPixelNumber(int number, float cellRightX, float cellBottomY, int screenW, int screenH, int scaleLoc, float aspect);