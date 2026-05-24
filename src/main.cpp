#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <ctime>

#include "Config.h"
#include "World.h"
#include "Physics.h"

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

// Для инвентаря (UI)
float lastBorderScaleX = 0.0f;
float lastStartUiX = 0.0f;
float lastSlotStepX = 0.0f;
float lastBorderScaleY = 0.0f;
float lastUiY = -0.85f;


// Технические штуки.
// Изменене размеров окна
void glfwWindowSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Колёсико мыши для быстрой смены слотов инвентаря
void glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (yoffset > 0) {
        selectedSlot = (selectedSlot - 1 + 9) % 9;
    }
    else if (yoffset < 0) {
        selectedSlot = (selectedSlot + 1) % 9;
    }
}

// Клавиши (Esc и цифры 1-9)
void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if (action == GLFW_PRESS && key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
        selectedSlot = key - GLFW_KEY_1;
    }
}

// ЛКМ для выбора ячейки инвентаря на экране
void glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        int w, h;
        glfwGetWindowSize(window, &w, &h);
        float aspect = (float)w / (float)h;

        // Переводим пиксельные координаты экрана в координаты от -1.0f до 1.0f
        float ndcX = (2.0f * (float)mouseX) / (float)w - 1.0f;
        float ndcY = 1.0f - (2.0f * (float)mouseY) / (float)h;

        // Проверяем, попал ли клик по высоте инвентаря
        float halfHeight = lastBorderScaleY / 2.0f;
        if (ndcY >= (lastUiY - halfHeight) && ndcY <= (lastUiY + halfHeight)) {

            // Проверяем каждый из 9 слотов
            for (int i = 0; i < 9; i++) {
                float slotCenterX = lastStartUiX + i * lastSlotStepX;

                float leftEdge = (slotCenterX - (lastBorderScaleX / 2.0f)) / aspect;
                float rightEdge = (slotCenterX + (lastBorderScaleX / 2.0f)) / aspect;

                if (ndcX >= leftEdge && ndcX <= rightEdge) {
                    selectedSlot = i; // Активируем выбранный слот
                    return; // Выходим, чтобы клик не улетел в игровой мир
                }
            }
        }
    }
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

int main() {
    srand((unsigned int)time(NULL));
    // Инициализация графического окна
    if (!glfwInit()) {
        std::cerr << "Can't load GLFW." << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1024, 768, "Minecraft 2D", NULL, NULL);
    if (!window) {
        std::cerr << "Can't make a window." << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwSetWindowSizeCallback(window, glfwWindowSizeCallback);
    glfwSetKeyCallback(window, glfwKeyCallback);
    glfwSetScrollCallback(window, glfwScrollCallback);
    glfwSetMouseButtonCallback(window, glfwMouseButtonCallback);
    glfwMakeContextCurrent(window);

    if (!gladLoadGL()) {
        std::cerr << "Can't load GLAD." << std::endl;
        return -1;
    }

    std::cout << "------------------------------------------" << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL Version:  " << glGetString(GL_VERSION) << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    glfwSwapInterval(1); // врубаю V-sync
    generateWorld();

    // Сборка шейдеров
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);
    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);
    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    // Подготовка геометрии (один квадрат для всех блоков)
    float vertices[] = { -0.5f,-0.5f, 0.5f,-0.5f, 0.5f,0.5f, -0.5f,-0.5f, 0.5f,0.5f, -0.5f,0.5f };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // это нам не объясняли. разберусь может быть потом. 
    unsigned int instanceOffsetsVBO, instanceColorsVBO;
    glGenBuffers(1, &instanceOffsetsVBO);
    glGenBuffers(1, &instanceColorsVBO);

    glBindBuffer(GL_ARRAY_BUFFER, instanceOffsetsVBO);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    glBindBuffer(GL_ARRAY_BUFFER, instanceColorsVBO);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // Ужас.
    unsigned int playerVAO, playerVBO;
    glGenVertexArrays(1, &playerVAO);
    glGenBuffers(1, &playerVBO);
    glBindVertexArray(playerVAO);
    glBindBuffer(GL_ARRAY_BUFFER, playerVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    int scaleLocation = glGetUniformLocation(program, "scale");
    int aspectLocation = glGetUniformLocation(program, "aspect");

    // Выделяю память под сборщик видимых блоков заранее
    std::vector<float> activeOffsets;
    std::vector<float> activeColors;
    activeOffsets.reserve(4000);
    activeColors.reserve(6000);

    float lastFrameTime = 0.0f;

    // Главный цикл отрисовки
    while (!glfwWindowShouldClose(window)) {
        float currentFrameTime = (float)glfwGetTime();
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        int w, h;
        glfwGetWindowSize(window, &w, &h);
        float aspect = (float)w / (float)h;

        double currentMouseX, currentMouseY;
        glfwGetCursorPos(window, &currentMouseX, &currentMouseY);
        updatePhysics(window, deltaTime, currentMouseX, currentMouseY);

        // Рисуем небо
        glClearColor(SKY_R, SKY_G, SKY_B, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);

        // Масштабирование блоков относительно высоты экрана
        float sy = 2.0f / VISIBLE_HEIGHT;
        float sx = sy;

        glUniform1f(aspectLocation, aspect);
        glUniform2f(scaleLocation, sx, sy);

        // Определение границ отрисовки (для оптимизации рисуем только то, что видит камера)
        int rangeX = (int)(VISIBLE_HEIGHT * aspect / 2) + 2;
        int rangeY = (int)(VISIBLE_HEIGHT / 2) + 2;
        int startX = std::max(0, (int)cameraX - rangeX);
        int endX = std::min(MAP_WIDTH, (int)cameraX + rangeX);
        int startY = std::max(0, (int)cameraY - rangeY);
        int endY = std::min(MAP_HEIGHT, (int)cameraY + rangeY);

        // Очищаем векторы сбора данных перед каждым кадром
        activeOffsets.clear();
        activeColors.clear();

        // Сначала собираем задний слой [0], чтобы он отрисовался позади игрока и передних блоков

        for (int layerIdx = 0; layerIdx <= 1; layerIdx++) {
            for (int y = startY; y < endY; y++) {
                for (int x = startX; x < endX; x++) {
                    int block = worldMap[layerIdx][y][x];
                    if (block == AIR) continue;

                    // Передача позиции и отрисовка
                    activeOffsets.push_back((x - cameraX) * sx);
                    activeOffsets.push_back((y - cameraY) * sy);

                    // Базовый цвет блока
                    float r = 0.0f, g = 0.0f, b = 0.0f;
                    if (block == GRASS) { r = 0.2f;  g = 0.8f;  b = 0.2f; }
                    else if (block == DIRT) { r = 0.45f;  g = 0.27f;  b = 0.08f; }
                    else if (block == STONE) { r = 0.4f;  g = 0.4f;  b = 0.42f; }
                    else if (block == WOOD) { r = 0.55f;  g = 0.37f; b = 0.15f; }
                    else if (block == LEAVES) { r = 0.1f;  g = 0.5f;  b = 0.1f; }

                    // Если это задний слой, смешиваем: 75% цвета блока + 25% цвета фона (неба)
                    if (layerIdx == 0) {
                        r = r * 0.75f + SKY_R * 0.25f;
                        g = g * 0.75f + SKY_G * 0.25f;
                        b = b * 0.75f + SKY_B * 0.25f;
                    }

                    activeColors.push_back(r);
                    activeColors.push_back(g);
                    activeColors.push_back(b);
                }
            }
        }

        int blocksCountInVbo = (int)(activeOffsets.size() / 2);

        // Сбор данных о выпавшем луте (левитация)
        for (const auto& item : droppedItems) {
            float renderY = item.y;
            if (item.isOnGround) {
                renderY = item.groundY + 0.18f + std::sin(item.animTime) * 0.08f;
            }
            activeOffsets.push_back((item.x - cameraX) * sx);
            activeOffsets.push_back((renderY - cameraY) * sy);

            // цвет предмета
            float r = 0.0f, g = 0.0f, b = 0.0f;
            if (item.type == GRASS) { r = 0.2f; g = 0.8f; b = 0.2f; }
            else if (item.type == DIRT) { r = 0.45f; g = 0.27f; b = 0.08f; }
            else if (item.type == STONE) { r = 0.4f; g = 0.4f; b = 0.42f; }
            else if (item.type == WOOD) { r = 0.55f; g = 0.37f; b = 0.15f; }
            else if (item.type == LEAVES) { r = 0.1f; g = 0.5f; b = 0.1f; }
            else if (item.type == APPLE) { r = 0.9f; g = 0.1f; b = 0.1f; }

            activeColors.push_back(r);
            activeColors.push_back(g);
            activeColors.push_back(b);
        }

        int totalElementsInVbo = (int)(activeOffsets.size() / 2);

        if (totalElementsInVbo > 0) {
            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, instanceOffsetsVBO);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(activeOffsets.size() * sizeof(float)), activeOffsets.data(), GL_STREAM_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, instanceColorsVBO);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(activeColors.size() * sizeof(float)), activeColors.data(), GL_STREAM_DRAW);

            // Отрисовка blocks
            if (blocksCountInVbo > 0) {
                glUniform2f(scaleLocation, sx, sy);
                glDrawArraysInstanced(GL_TRIANGLES, 0, 6, blocksCountInVbo);
            }

            // Отрисовка выдавших предметов
            if (!droppedItems.empty()) {
                glUniform2f(scaleLocation, sx * 0.25f, sy * 0.25f);
                glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, (GLsizei)droppedItems.size(), blocksCountInVbo);
            }
        }

        // Рисуем игрока поверх заднего слоя блоков
        glBindVertexArray(playerVAO);
        float playerOffset[] = { (player.x - cameraX) * sx, (player.y - cameraY) * sy };
        float playerColor[] = { 1.0f, 0.3f, 0.3f };

        glUniform2f(scaleLocation, sx * player.width, sy * player.height);
        // это чтобы игрока как блок не считало и не применяло к нему цвета и смещение по его стандарту
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        // наше передаем вместо этого
        glVertexAttrib2fv(1, playerOffset);
        glVertexAttrib3fv(2, playerColor);
        // рисуем двумя треугольниками
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Отрисовка инвентаря
        glBindVertexArray(playerVAO);

        float slotSizePixels = 56.0f;
        float paddingPixels = 2.0f;
        float borderSizePixels = slotSizePixels + paddingPixels * 2.0f;

        lastBorderScaleX = borderSizePixels * (2.0f / (float)w) * aspect;
        lastBorderScaleY = borderSizePixels * (2.0f / (float)h);
        float slotScaleX = slotSizePixels * (2.0f / (float)w) * aspect;
        float slotScaleY = slotSizePixels * (2.0f / (float)h);

        glUniform1f(aspectLocation, aspect);

        float totalInventoryWidthNDC = (9 * borderSizePixels + 8 * paddingPixels) * (2.0f / (float)w) * aspect;
        lastStartUiX = (-totalInventoryWidthNDC / 2.0f + (borderSizePixels * (1.0f / (float)w)) * aspect);
        lastSlotStepX = (borderSizePixels + paddingPixels) * (2.0f / (float)w) * aspect;

        for (int i = 0; i < 9; i++) {
            float slotX = lastStartUiX + i * lastSlotStepX;

            // Обводка
            glUniform2f(scaleLocation, lastBorderScaleX, lastBorderScaleY);
            float borderOffset[] = { slotX, lastUiY };
            float borderColor[] = { 0.65f, 0.65f, 0.65f };
            glVertexAttrib2fv(1, borderOffset);
            glVertexAttrib3fv(2, borderColor);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Внутренний фон слота
            glUniform2f(scaleLocation, slotScaleX, slotScaleY);
            float selectedColor[] = { 0.6f, 0.6f, 0.6f };
            float normalColor[] = { 0.45f, 0.45f, 0.45f };

            if (i == selectedSlot) glVertexAttrib3fv(2, selectedColor);
            else glVertexAttrib3fv(2, normalColor);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Иконка предмета в слоте
            if (inventory[i].type != AIR) {
                glUniform2f(scaleLocation, slotScaleX * 0.52f, slotScaleY * 0.52f);
                float itemR = 0.0f, itemG = 0.0f, itemB = 0.0f;
                int type = inventory[i].type;
                if (type == GRASS) { itemR = 0.2f; itemG = 0.8f; itemB = 0.2f; }
                else if (type == DIRT) { itemR = 0.45f; itemG = 0.27f; itemB = 0.08f; }
                else if (type == STONE) { itemR = 0.4f; itemG = 0.4f; itemB = 0.42f; }
                else if (type == WOOD) { itemR = 0.55f; itemG = 0.37f; itemB = 0.15f; }
                else if (type == LEAVES) { itemR = 0.1f; itemG = 0.5f; itemB = 0.1f; }
                else if (type == APPLE) { itemR = 0.9f; itemG = 0.1f; itemB = 0.1f; }

                float itemColor[] = { itemR, itemG, itemB };
                glVertexAttrib3fv(2, itemColor);
                glDrawArrays(GL_TRIANGLES, 0, 6);

                // Вывод цифр количества
                if (inventory[i].count > 1) {
                    float rightEdgeNDC = slotX + (slotSizePixels / 2.0f) * (2.0f / (float)w) * aspect;
                    float bottomEdgeNDC = lastUiY - (slotSizePixels / 2.0f) * (2.0f / (float)h);
                    drawPixelNumber(inventory[i].count, rightEdgeNDC, bottomEdgeNDC, w, h, scaleLocation, aspect);
                }
            }
        }

        // Отрисовка крестика (временная наверн)=
        float normX = (2.0f * (float)currentMouseX) / w - 1.0f;
        float normY = 1.0f - (2.0f * (float)currentMouseY) / h;
        float worldMouseX = (normX * aspect * VISIBLE_HEIGHT / 2.0f) + cameraX;
        float worldMouseY = (normY * VISIBLE_HEIGHT / 2.0f) + cameraY;
        int targetX = (int)std::floor(worldMouseX + 0.5f);
        int targetY = (int)std::floor(worldMouseY + 0.5f);
        
        bool shiftPressed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
        int layerToPlace = shiftPressed ? 0 : 1;
        bool dummyBuildingUnderSelf = false;

        // выбор цвета курсора
        bool canBuild = canPlaceBlock(targetX, targetY, layerToPlace, dummyBuildingUnderSelf);

        glBindVertexArray(playerVAO);
        glUniform1f(aspectLocation, aspect);
        if (canBuild) {
            float greenColor[] = { 0.0f, 1.0f, 0.0f };
            glVertexAttrib3fv(2, greenColor);
        }
        else {
            float whiteColor[] = { 1.0f, 1.0f, 1.0f };
            glVertexAttrib3fv(2, whiteColor);
        }

        // Перевод коорд
        float mouseNdcX = (2.0f * (float)currentMouseX) / (float)w - 1.0f;
        float mouseNdcY = 1.0f - (2.0f * (float)currentMouseY) / (float)h;

        float crosshairOffset[] = { mouseNdcX * aspect, mouseNdcY };
        glVertexAttrib2fv(1, crosshairOffset);

        // Горизонтальная линия крестика
        float thicknessX = 4.0f / (float)w;
        float lengthX = 30.0f / (float)w;
        glUniform2f(scaleLocation, lengthX * aspect, thicknessX);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Вертикальная
        float thicknessY = 4.0f / (float)h;
        float lengthY = 30.0f / (float)h;
        glUniform2f(scaleLocation, thicknessY * aspect, lengthY);
        // рисуем всё двумя треугольниками
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // для блоков опять врубаем авточтение цветов и смещения
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // Перед выходом:
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &instanceOffsetsVBO);
    glDeleteBuffers(1, &instanceColorsVBO);
    glDeleteVertexArrays(1, &playerVAO);
    glDeleteBuffers(1, &playerVBO);
    glfwTerminate();
    return 0;
}

// НА ПОТОМ:
// Добавить руды и рандомную генерацию 
// 
// добавить инвентарь побольше, крафт
// 
// 
// 
// Потом добавить облака, музыку, мобов