#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <ctime>

#include "Graphics.h"
#include "CallBackFunctions.h"
#include "Config.h"
#include "World.h"
#include "Physics.h"

int main() {
    // Мир теперь рандомный, а не тестовый, сид генерируется сам
    srand((unsigned int)time(0));
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
                    float r = getBlockColorR(block);
                    float g = getBlockColorG(block);
                    float b = getBlockColorB(block);

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
            float r = getBlockColorR(item.type); 
            float g = getBlockColorG(item.type);
            float b = getBlockColorB(item.type);

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

            // Отрисовка блоков
            if (blocksCountInVbo > 0) {
                glUniform2f(scaleLocation, sx, sy);
                glDrawArraysInstanced(GL_TRIANGLES, 0, 6, blocksCountInVbo);
            }

            // Отрисовка выпавших предметов
            if (!droppedItems.empty()) {
                glUniform2f(scaleLocation, sx * 0.25f, sy * 0.25f);
                glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, (GLsizei)droppedItems.size(), blocksCountInVbo);
            }
        }


        // Отрисовка мобов
        // Овечка
        for (const auto& sheep : sheeps) {
            glBindVertexArray(playerVAO);
            float sheepOffset[] = { (sheep.x - cameraX) * sx, (sheep.y - cameraY) * sy };
            float sheepColor[] = { 0.95f, 0.95f, 0.95f }; // Белая шерсть овечки

            glUniform2f(scaleLocation, sx * sheep.width, sy * sheep.height);
            glDisableVertexAttribArray(1);
            glDisableVertexAttribArray(2);
            glVertexAttrib2fv(1, sheepOffset);
            glVertexAttrib3fv(2, sheepColor);
            glDrawArrays(GL_TRIANGLES, 0, 6);
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
                int type = inventory[i].type;
                float itemR = getBlockColorR(inventory[i].type);
                float itemG = getBlockColorG(inventory[i].type);
                float itemB = getBlockColorB(inventory[i].type);

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

        // Крафт
        craftBtnScaleX = slotSizePixels * (2.0f / (float)w) * aspect;
        craftBtnScaleY = slotSizePixels * (2.0f / (float)h);
        craftBtnCenterX = aspect - (craftBtnScaleX / 2.0f) - 0.05f * aspect;
        craftBtnCenterY = 1.0f - (craftBtnScaleY / 2.0f) - 0.05f;

        glUniform1f(aspectLocation, aspect);
        glUniform2f(scaleLocation, craftBtnScaleX, craftBtnScaleY);
        float craftBtnOffset[] = { craftBtnCenterX, craftBtnCenterY };
        float craftBtnColor[] = { 0.55f, 0.55f, 0.55f }; // Цвет кнопки
        if (isCraftingOpen) {
            craftBtnColor[0] = 0.7f; craftBtnColor[1] = 0.7f; craftBtnColor[2] = 0.7f; // Подсветка активной кнопки
        }
        glVertexAttrib2fv(1, craftBtnOffset);
        glVertexAttrib3fv(2, craftBtnColor);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        if (isCraftingOpen) {
            // Размеры основного светло-серого окна меню
            float menuW = 300.0f * (2.0f / (float)w) * aspect;
            float menuH = 180.0f * (2.0f / (float)h);
            float menuCenterX = craftBtnCenterX - menuW / 2.0f - 0.15f;
            float menuCenterY = craftBtnCenterY - menuH / 2.0f + (craftBtnScaleY / 2.0f);

            // Сохраняем размеры для мышки
            lastMenuW = menuW; lastMenuH = menuH;
            lastMenuCenterX = menuCenterX; lastMenuCenterY = menuCenterY;

            // 1. Рисуем подложку
            glUniform2f(scaleLocation, menuW, menuH);
            float menuOffset[] = { menuCenterX, menuCenterY };
            float menuBgColor[] = { 0.8f, 0.8f, 0.8f };
            glVertexAttrib2fv(1, menuOffset);
            glVertexAttrib3fv(2, menuBgColor);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Размеры ячеек крафта
            float craftSlotW = 44.0f * (2.0f / (float)w) * aspect;
            float craftSlotH = 44.0f * (2.0f / (float)h);
            float craftPaddingX = 6.0f * (2.0f / (float)w) * aspect;
            float craftPaddingY = 6.0f * (2.0f / (float)h);

            float gridStartX = menuCenterX - menuW / 4.0f;
            float gridStartY = menuCenterY + craftSlotH / 2.0f + craftPaddingY / 2.0f;
            float slotNormalColor[] = { 0.45f, 0.45f, 0.45f };

            // Сохраняем это для мышки
            lastCraftSlotW = craftSlotW; lastCraftSlotH = craftSlotH;
            lastGridStartX = gridStartX; lastGridStartY = gridStartY;
            lastCraftPaddingX = craftPaddingX; lastCraftPaddingY = craftPaddingY;

            // 2. Рисуем 4 ячейки крафта и иконки в них
            for (int row = 0; row < 2; row++) {
                for (int col = 0; col < 2; col++) {
                    float cx = gridStartX + col * (craftSlotW + craftPaddingX);
                    float cy = gridStartY - row * (craftSlotH + craftPaddingY);

                    // Сама ячейка
                    glUniform2f(scaleLocation, craftSlotW, craftSlotH);
                    float cOffset[] = { cx, cy };
                    glVertexAttrib2fv(1, cOffset);
                    glVertexAttrib3fv(2, slotNormalColor);
                    glDrawArrays(GL_TRIANGLES, 0, 6);

                    // Предмет в ней
                    if (craftGrid[row][col].type != AIR) {
                        glUniform2f(scaleLocation, craftSlotW * 0.52f, craftSlotH * 0.52f);
                        int type = craftGrid[row][col].type;
                        float itemR = getBlockColorR(type);
                        float itemG = getBlockColorG(type);
                        float itemB = getBlockColorB(type);

                        float itemColor[] = { itemR, itemG, itemB };
                        glVertexAttrib3fv(2, itemColor);
                        glDrawArrays(GL_TRIANGLES, 0, 6);

                        if (craftGrid[row][col].count > 1) {
                            drawPixelNumber(craftGrid[row][col].count, cx + craftSlotW / 2.0f, cy - craftSlotH / 2.0f, w, h, scaleLocation, aspect);
                        }
                        glVertexAttrib2fv(1, cOffset);
                    }
                }
            }

            // 3. Рисуем тёмно-серую стрелочку
            float arrowCenterX = gridStartX + 2 * (craftSlotW + craftPaddingX) + 5.0f * (2.0f / (float)w) * aspect;
            float arrowCenterY = menuCenterY;
            float arrowColor[] = { 0.3f, 0.3f, 0.3f };

            float arrBodyW = 24.0f * (2.0f / (float)w) * aspect;
            float arrBodyH = 8.0f * (2.0f / (float)h);
            glUniform2f(scaleLocation, arrBodyW, arrBodyH);
            float arrBodyOffset[] = { arrowCenterX, arrowCenterY };
            glVertexAttrib2fv(1, arrBodyOffset);
            glVertexAttrib3fv(2, arrowColor);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            

            float arrTipW = 6.0f * (2.0f / (float)w) * aspect;
            for (int i = 0; i < 3; i++) {
                float arrTipH = (24.0f - i * 8.0f) * (2.0f / (float)h);
                glUniform2f(scaleLocation, arrTipW, arrTipH);
                float arrTipOffset[] = { arrowCenterX + arrBodyW / 2.0f + (i * arrTipW / 2.0f), arrowCenterY };
                glVertexAttrib2fv(1, arrTipOffset);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            // 4. Рисуем 5-ю ячейку результат и иконку в ней
            float resultCenterX = arrowCenterX + arrBodyW + 25.0f * (2.0f / (float)w) * aspect;
            float resultCenterY = menuCenterY;
            lastResultCenterX = resultCenterX; lastResultCenterY = resultCenterY; // сохраняем

            glUniform2f(scaleLocation, craftSlotW, craftSlotH);
            float resOffset[] = { resultCenterX, resultCenterY };
            glVertexAttrib2fv(1, resOffset);
            glVertexAttrib3fv(2, slotNormalColor);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            if (craftResult.type != AIR) {
                glUniform2f(scaleLocation, craftSlotW * 0.52f, craftSlotH * 0.52f);
                int type = craftResult.type;
                float itemR = getBlockColorR(type); // или item.type для лута
                float itemG = getBlockColorG(type);
                float itemB = getBlockColorB(type);

                float itemColor[] = { itemR, itemG, itemB };
                glVertexAttrib3fv(2, itemColor);
                glDrawArrays(GL_TRIANGLES, 0, 6);

                if (craftResult.count > 1) {
                    drawPixelNumber(craftResult.count, resultCenterX + craftSlotW / 2.0f, resultCenterY - craftSlotH / 2.0f, w, h, scaleLocation, aspect);
                }
            }
        }

        // Предмет в руке при перетаскивании
        if (cursorSlot.type != AIR) {
            float mouseNdcX = (2.0f * (float)currentMouseX) / (float)w - 1.0f;
            float mouseNdcY = 1.0f - (2.0f * (float)currentMouseY) / (float)h;

            // Подгоняем размер иконки (масштаб ячейки 56px * 0.52 для самого предмета)
            float dragW = 56.0f * (2.0f / (float)w) * aspect * 0.52f;
            float dragH = 56.0f * (2.0f / (float)h) * 0.52f;
            glUniform2f(scaleLocation, dragW, dragH);

            float dragOffset[] = { mouseNdcX * aspect, mouseNdcY };
            glVertexAttrib2fv(1, dragOffset);

            // Определяем цвет предмета в зависимости от типа в cursorSlot
            int type = cursorSlot.type;
            float itemR = getBlockColorR(type);
            float itemG = getBlockColorG(type);
            float itemB = getBlockColorB(type);

            float itemColor[] = { itemR, itemG, itemB };
            glVertexAttrib3fv(2, itemColor);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Если предметов больше одного — выводим количество функцией цифр
            if (cursorSlot.count > 1) {
                drawPixelNumber(cursorSlot.count, mouseNdcX * aspect + dragW, mouseNdcY - dragH, w, h, scaleLocation, aspect);
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
// добавить инвентарь побольше
// 
// 
// 
// Потом добавить облака, музыку, мобов