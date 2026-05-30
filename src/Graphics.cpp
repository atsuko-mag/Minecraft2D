#include <glad/glad.h>
#include "Config.h"
#include "Graphics.h"

// Шейдеры
const char* vertexShaderSource = "#version 460 core\n" // так. это - верся языка шейдеров...
"layout (location = 0) in vec2 aPos;\n" // вершины блока
"layout (location = 1) in vec2 instanceOffset;\n" // где блок находится
"layout (location = 2) in vec3 instanceColor;\n"  // цвет для блока
"out vec3 blockColor;\n" // что на выходе по цвету
"uniform vec2 scale;\n" // ..по величине блока (это для всех )
"uniform float aspect;\n" // пропорции экрана
"void main() {\n"
"   gl_Position = vec4((aPos.x * scale.x + instanceOffset.x) / aspect, aPos.y * scale.y + instanceOffset.y, 0.0, 1.0);\n" //ПОЛОЖЕНИЕ ТОЧКИ НА ЭКРАНЕ В ИТОГЕ
"   blockColor = instanceColor;\n"
"}\0";

const char* fragmentShaderSource = "#version 460 core\n" // эта штука красит каждый пиксель
"out vec4 FragColor;\n"
"in vec3 blockColor;\n"
"void main() {\n"
"   FragColor = vec4(blockColor, 1.0f);\n"
"}\n\0";


float getBlockColorR(int type) {
    if (type == GRASS)  return 0.17f;
    if (type == DIRT)   return 0.45f;
    if (type == STONE)  return 0.4f;
    if (type == WOOD)   return 0.55f;
    if (type == LEAVES) return 0.1f;
    if (type == APPLE)  return 0.9f;
    if (type == PLANKS) return 0.75f;
    return 0.0f; // AIR
}

float getBlockColorG(int type) {
    if (type == GRASS)  return 0.7f;
    if (type == DIRT)   return 0.27f;
    if (type == STONE)  return 0.4f;
    if (type == WOOD)   return 0.37f;
    if (type == LEAVES) return 0.5f;
    if (type == APPLE)  return 0.1f;
    if (type == PLANKS) return 0.55f;
    return 0.0f;
}

float getBlockColorB(int type) {
    if (type == GRASS)  return 0.17f;
    if (type == DIRT)   return 0.08f;
    if (type == STONE)  return 0.42f;
    if (type == WOOD)   return 0.15f;
    if (type == LEAVES) return 0.1f;
    if (type == APPLE)  return 0.1f;
    if (type == PLANKS) return 0.3f;
    return 0.0f;
}



// Матрицы пиксельных цифр 3x5 для количества предметов
const int font3x5[10][5] = {
    {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
    {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
    {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
    {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
    {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
    {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
    {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
    {0b111, 0b001, 0b010, 0b010, 0b010}, // 7
    {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
    {0b111, 0b101, 0b111, 0b001, 0b111}  // 9
};

// Рендеринг одной цифры шрифтом 3x5 (масштаб х3 пикселя)
void drawPixelDigit(int digit, float startPixelX, float startPixelY, float pixelSizeX, float pixelSizeY, int scaleLoc, float aspect) {
    if (digit < 0 || digit > 9) return;

    glUniform2f(scaleLoc, pixelSizeX * 3.0f * aspect, pixelSizeY * 3.0f);
    float whiteColor[] = { 1.0f, 1.0f, 1.0f };
    glVertexAttrib3fv(2, whiteColor);

    for (int row = 0; row < 5; row++) {
        int rowBits = font3x5[digit][row];
        for (int col = 0; col < 3; col++) {
            if ((rowBits >> (2 - col)) & 1) {
                float px = startPixelX + (col * 3.0f + 1.5f) * pixelSizeX * aspect;
                float py = startPixelY - (row * 3.0f + 1.5f) * pixelSizeY;

                float digitOffset[] = { px, py };
                glVertexAttrib2fv(1, digitOffset);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
    }
}

// Рисует кол-во блоков в ячейке
void drawPixelNumber(int number, float cellRightX, float cellBottomY, int screenW, int screenH, int scaleLoc, float aspect) {
    float pixelSizeX = 2.0f / (float)screenW;
    float pixelSizeY = 2.0f / (float)screenH;

    float endX = cellRightX - 5.0f * pixelSizeX * aspect;
    float startY = cellBottomY + 17.0f * pixelSizeY;

    if (number >= 10) {
        int d1 = number / 10;
        int d2 = number % 10;
        drawPixelDigit(d2, endX - 3.0f * pixelSizeX * 3.0f * aspect, startY, pixelSizeX, pixelSizeY, scaleLoc, aspect);
        drawPixelDigit(d1, endX - 7.0f * pixelSizeX * 3.0f * aspect, startY, pixelSizeX, pixelSizeY, scaleLoc, aspect);
    }
    else {
        drawPixelDigit(number, endX - 3.0f * pixelSizeX * 3.0f * aspect, startY, pixelSizeX, pixelSizeY, scaleLoc, aspect);
    }
}