#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>

typedef long long ll;

// константы для быстрой отладки.

//  НАСТРОЙКИ МИРА  
const int MAP_WIDTH = 200;      // ширина всей карты в блоках
const int MAP_HEIGHT = 100;     // высота карты в блоках
const int GROUND_LEVEL = 20;    // уровень поверхности (т.е травы)
const float VISIBLE_HEIGHT = 14.0f; // сколько блоков помещается в экран по вертикали

//  ФИЗИКА ДВИЖЕНИЯ 
const float PHYSICS_GRAVITY = 0.015f;      // сила притяжения (чем меньше, тем медленнее падение)
const float PHYSICS_JUMP_POWER = 0.2f;     // высота прыжка
const float PHYSICS_WALK_SPEED = 0.12f;      // максимальная скорость ходьбы
const float PHYSICS_ACCEL = 0.008f;          // ускорение (плавность разгона)
const float PHYSICS_FRICTION = 6.0f;       // трение (плавность остановки)

// Типы блоков
enum BlockType { AIR = 0, GRASS = 1, DIRT = 2, STONE = 3, WOOD = 4, LEAVES = 5 };
int worldMap[MAP_HEIGHT][MAP_WIDTH] = { 0 };

// Структура игрока
struct Player {
    float x, y;             // Позиция центра персонажа
    float velX, velY;       // Текущая скорость по осям
    float width = 0.6f;     // Ширина хитбокса
    float height = 1.8f;    // Высота хитбокса
    bool isOnGround;        // Флаг: стоит ли персонаж на земле
    Player() : x(20.0f), y(25.0f), velX(0), velY(0), isOnGround(false) {}
};

Player player;
float cameraX = 0.0f;       // Позиция камеры по X
float cameraY = 0.0f;       // Позиция камеры по Y

//  Переделала шейдеры... (комменты для меня надо)
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

// Технические штуки.
void glfwWindowSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

//  ГЕНЕРАЦИЯ ОБЪЕКТОВ
// Создание дерева в указанных координатах
void addTree(int x, int y) {
    if (x < 2 || x >= MAP_WIDTH - 2 || y >= MAP_HEIGHT - 6) return;
    for (int i = 1; i <= 5; i++) worldMap[y + i][x] = WOOD; // Ствол
    int leafBase = y + 3;
    for (int ly = 0; ly <= 1; ly++) { // Листва
        for (int lx = -2; lx <= 2; lx++) {
            if (worldMap[leafBase + ly][x + lx] == AIR) worldMap[leafBase + ly][x + lx] = LEAVES;
        }
    }for (int ly = 2; ly <= 3; ly++) {
        for (int lx = -1; lx <= 1; lx++) {
            if (worldMap[leafBase + ly][x + lx] == AIR) worldMap[leafBase + ly][x + lx] = LEAVES;
        }
    }
}

// Заполнение карты начальными блоками
void generateWorld() {
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            if (y > GROUND_LEVEL) worldMap[y][x] = AIR;
            else if (y == GROUND_LEVEL) worldMap[y][x] = GRASS;
            else if (y < GROUND_LEVEL && y >= GROUND_LEVEL - 3) worldMap[y][x] = DIRT;
            else worldMap[y][x] = STONE;
        }
    }
    for (int x = 10; x < MAP_WIDTH; x += 15) addTree(x, GROUND_LEVEL);
}

//  ФИЗИКА И УПРАВЛЕНИЕ
void updatePhysics(GLFWwindow* window, float dt) {
    if (dt > 0.1f) dt = 0.1f;

    // Рассчитываем множитель шага на основе deltaTime (базовое значение берем за 60 FPS, т.е. шаг где-то 0.016с)
    float dtMultiplier = dt / 0.016666f;

    // Обработка клавиш движения (A и D) с учетом ускорения
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        player.velX -= PHYSICS_ACCEL * dtMultiplier;
        if (player.velX < -PHYSICS_WALK_SPEED) player.velX = -PHYSICS_WALK_SPEED;
    }
    else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        player.velX += PHYSICS_ACCEL * dtMultiplier;
        if (player.velX > PHYSICS_WALK_SPEED) player.velX = PHYSICS_WALK_SPEED;
    }
    else {
        // Эффект трения при остановке
        if (player.velX > 0) {
            player.velX -= PHYSICS_FRICTION * dt;
            if (player.velX < 0) player.velX = 0;
        }
        else if (player.velX < 0) {
            player.velX += PHYSICS_FRICTION * dt;
            if (player.velX > 0) player.velX = 0;
        }
    }

    // Обработка прыжка (Пробел)
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && player.isOnGround) {
        player.velY = PHYSICS_JUMP_POWER;
        player.isOnGround = false;
    }
    // (W)
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && player.isOnGround) {
        player.velY = PHYSICS_JUMP_POWER;
        player.isOnGround = false;
    }

    // Применение гравитации и обновление координат
    player.velY -= PHYSICS_GRAVITY * dtMultiplier;
    player.y += player.velY * dtMultiplier;
    player.x += player.velX * dtMultiplier;

    // Коллизия с уровнем земли (пока что так...)
    float groundTop = (float)GROUND_LEVEL + 0.5f;
    if (player.y - (player.height / 2.0f) <= groundTop) {
        player.y = groundTop + (player.height / 2.0f);
        player.velY = 0;
        player.isOnGround = true;
    }

    // Плавное следование камеры за персонажем (не зависит от частоты кадров)
    float cameraSmoothing = 1.0f - std::pow(0.1f, dt * 5.0f);
    cameraX += (player.x - cameraX) * cameraSmoothing;
    cameraY += (player.y - cameraY) * cameraSmoothing;
}

int main() {
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

    // Компиляция и сборка шейдерной программы
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
    glVertexAttribDivisor(1, 1); // Менять значение 1 раз за каждый блок

    glBindBuffer(GL_ARRAY_BUFFER, instanceColorsVBO);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1); // Менять цвет 1 раз за каждый блок

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
    activeOffsets.reserve(2000); // Примерный лимит видимых блоков на экране
    activeColors.reserve(3000);

    float lastFrameTime = 0.0f;

    // Главный цикл отрисовки
    while (!glfwWindowShouldClose(window)) {
        float currentFrameTime = (float)glfwGetTime();
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        int w, h;
        glfwGetWindowSize(window, &w, &h);
        float aspect = (float)w / (float)h;

        updatePhysics(window, deltaTime);

        // Рисуем небо
        glClearColor(0.5f, 0.8f, 1.0f, 1.0f);
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

        // Цикл отрисовки блоков мира
        for (int y = startY; y < endY; y++) {
            for (int x = startX; x < endX; x++) {
                if (worldMap[y][x] == AIR) continue;

                // Передача позиции и отрисовка
                activeOffsets.push_back((x - cameraX) * sx);
                activeOffsets.push_back((y - cameraY) * sy);

                // Выбор цвета в зависимости от типа блока. ПОТОМ ТЕКСТУРКИ
                if (worldMap[y][x] == GRASS) { activeColors.push_back(0.2f); activeColors.push_back(0.8f); activeColors.push_back(0.2f); }
                else if (worldMap[y][x] == DIRT) { activeColors.push_back(0.5f); activeColors.push_back(0.3f); activeColors.push_back(0.1f); }
                else if (worldMap[y][x] == STONE) { activeColors.push_back(0.4f); activeColors.push_back(0.4f); activeColors.push_back(0.42f); }
                else if (worldMap[y][x] == WOOD) { activeColors.push_back(0.4f); activeColors.push_back(0.25f); activeColors.push_back(0.1f); }
                else if (worldMap[y][x] == LEAVES) { activeColors.push_back(0.1f); activeColors.push_back(0.5f); activeColors.push_back(0.1f); }
            }
        }

        // ВОООТ ЭТО ВОТ ВСЁ - ОПТИМИЗАЦИЯ. 
        int totalVisibleBlocks = activeOffsets.size() / 2;

        if (totalVisibleBlocks > 0) {
            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, instanceOffsetsVBO);
            glBufferData(GL_ARRAY_BUFFER, activeOffsets.size() * sizeof(float), activeOffsets.data(), GL_STREAM_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, instanceColorsVBO);
            glBufferData(GL_ARRAY_BUFFER, activeColors.size() * sizeof(float), activeColors.data(), GL_STREAM_DRAW);

            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, totalVisibleBlocks);
        }

        // Рисуем игрока
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
// Трение не работает пока (или я не вижу...)
// Добавить руды и рандомную генерацию
// Поменять на два слоя блоков: рядом с игроком и бэкграунд
// 
// Ходить игроки и мобы смогут только на одном слое. деревья спавнятся на заднем. 
// 
// ставить блоки на переднем пкм, на заднем - с шифтом мб
// 
// добавить инвентарь
// 
// 
// 
// Потом добавить облака, музыку, мобов