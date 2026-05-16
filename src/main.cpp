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

// Константы(пока) цвета фона
const float SKY_R = 0.5f;
const float SKY_G = 0.8f;
const float SKY_B = 1.0f;

//  ФИЗИКА ДВИЖЕНИЯ 
const float PHYSICS_GRAVITY = 0.015f;      // сила притяжения (чем меньше, тем медленнее падение)
const float PHYSICS_JUMP_POWER = 0.2f;     // высота прыжка
const float PHYSICS_WALK_SPEED = 0.12f;      // максимальная скорость ходьбы
const float PHYSICS_ACCEL = 0.008f;          // ускорение (плавность разгона)
const float PHYSICS_FRICTION = 6.0f;       // трение (плавность остановки)

// Типы блоков
enum BlockType { AIR = 0, GRASS = 1, DIRT = 2, STONE = 3, WOOD = 4, LEAVES = 5 };

// Двухслойная карта: [0] - задний слой, [1] - передний слой (игрок ходит здесь)
int worldMap[2][MAP_HEIGHT][MAP_WIDTH] = { 0 };

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

// Флаг для отслеживания клика мыши (чтобы ломать один блок за один клик)
bool mousePressedLastFrame = false;

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
// Создание дерева в указанных координатах (деревья спавнятся на заднем слое [0])
void addTree(int x, int y) {
    if (x < 2 || x >= MAP_WIDTH - 2 || y >= MAP_HEIGHT - 6) return;
    for (int i = 1; i <= 5; i++) worldMap[0][y + i][x] = WOOD; // Ствол 
    int leafBase = y + 3;
    for (int ly = 0; ly <= 1; ly++) { // Листва сзади
        for (int lx = -2; lx <= 2; lx++) {
            if (worldMap[0][leafBase + ly][x + lx] == AIR) worldMap[0][leafBase + ly][x + lx] = LEAVES;
        }
    }for (int ly = 2; ly <= 3; ly++) {
        for (int lx = -1; lx <= 1; lx++) {
            if (worldMap[0][leafBase + ly][x + lx] == AIR) worldMap[0][leafBase + ly][x + lx] = LEAVES;
        }
    }
    for (int ly = 0; ly <= 1; ly++) { // спереди
        for (int lx = -2; lx <= 2; lx++) {
            worldMap[1][leafBase + ly][x + lx] = LEAVES;
        }
    }for (int ly = 2; ly <= 3; ly++) {
        for (int lx = -1; lx <= 1; lx++) {
            worldMap[1][leafBase + ly][x + lx] = LEAVES;
        }
    }
}

// Заполнение карты начальными блоками
void generateWorld() {
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            // Передний слой [1]
            if (y > GROUND_LEVEL) worldMap[1][y][x] = AIR;
            else if (y == GROUND_LEVEL) worldMap[1][y][x] = GRASS;
            else if (y < GROUND_LEVEL && y >= GROUND_LEVEL - 3) worldMap[1][y][x] = DIRT;
            else worldMap[1][y][x] = STONE;

            // Задний слой [0]
            if (y > GROUND_LEVEL) worldMap[0][y][x] = AIR;
            else if (y == GROUND_LEVEL) worldMap[0][y][x] = GRASS;
            else if (y < GROUND_LEVEL && y >= GROUND_LEVEL - 3) worldMap[0][y][x] = DIRT;
            else if (y < GROUND_LEVEL - 3) worldMap[0][y][x] = STONE;
            else worldMap[0][y][x] = AIR;
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
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && player.isOnGround) {
        player.velY = PHYSICS_JUMP_POWER;
        player.isOnGround = false;
    }

    // Применение гравитации и обновление координат
    player.velY -= PHYSICS_GRAVITY * dtMultiplier;
    player.y += player.velY * dtMultiplier;
    player.x += player.velX * dtMultiplier;

    // Закрываем окно игры, если игрок выпал за нижнюю границу карты в минус
    if (player.y < 0.0f) {
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }

    // --- КОЛЛИЗИЯ С ПОТОЛКОМ (стукаемся головой) ---
    if (player.velY > 0) {
        float headY = player.y + (player.height / 2.0f);
        int checkYHeadCeil = (int)std::floor(headY + 0.5f);

        int checkXHead1 = (int)std::floor(player.x - (player.width / 2.0f) + 0.05f + 0.5f);
        int checkXHead2 = (int)std::floor(player.x + (player.width / 2.0f) - 0.05f + 0.5f);

        if (checkYHeadCeil >= 0 && checkYHeadCeil < MAP_HEIGHT) {
            bool blockAboveLeft = (checkXHead1 >= 0 && checkXHead1 < MAP_WIDTH) && (worldMap[1][checkYHeadCeil][checkXHead1] != AIR);
            bool blockAboveRight = (checkXHead2 >= 0 && checkXHead2 < MAP_WIDTH) && (worldMap[1][checkYHeadCeil][checkXHead2] != AIR);

            if (blockAboveLeft || blockAboveRight) {
                float blockBottom = (float)checkYHeadCeil - 0.5f;
                player.y = blockBottom - (player.height / 2.0f);
                player.velY = 0;
            }
        }
    }

    // Норм коллизия, УРА
    // Проверяем две точки по высоте хитбокса: район ног и район головы
    float checkYFeet = player.y - player.height / 2.0f + 0.1f;
    float checkYHead = player.y + player.height / 2.0f - 0.1f;
    int tileYFeet = (int)std::floor(checkYFeet + 0.5f);
    int tileYHead = (int)std::floor(checkYHead + 0.5f);

    // Коллизия при движении вправо
    if (player.velX > 0) {
        float rightEdge = player.x + player.width / 2.0f;
        int tileX = (int)std::floor(rightEdge + 0.5f);
        if (tileX >= 0 && tileX < MAP_WIDTH) {
            bool wallFeet = (tileYFeet >= 0 && tileYFeet < MAP_HEIGHT) && (worldMap[1][tileYFeet][tileX] != AIR);
            bool wallHead = (tileYHead >= 0 && tileYHead < MAP_HEIGHT) && (worldMap[1][tileYHead][tileX] != AIR);
            if (wallFeet || wallHead) {
                player.x = (float)tileX - 0.5f - player.width / 2.0f;
                player.velX = 0;
            }
        }
    }
    // Коллизия при движении влево
    else if (player.velX < 0) {
        float leftEdge = player.x - player.width / 2.0f;
        int tileX = (int)std::floor(leftEdge + 0.5f);
        if (tileX >= 0 && tileX < MAP_WIDTH) {
            bool wallFeet = (tileYFeet >= 0 && tileYFeet < MAP_HEIGHT) && (worldMap[1][tileYFeet][tileX] != AIR);
            bool wallHead = (tileYHead >= 0 && tileYHead < MAP_HEIGHT) && (worldMap[1][tileYHead][tileX] != AIR);
            if (wallFeet || wallHead) {
                player.x = (float)tileX + 0.5f + player.width / 2.0f;
                player.velX = 0;
            }
        }
    }
    player.isOnGround = false;
    float footY = player.y - (player.height / 2.0f);
    int checkY = (int)std::floor(footY + 0.5f);

    // Сушаем область проверки под ногами на 0.05f с каждой стороны,
    // чтобы игрок не считал стены под собой полом, когда трется об них вплотную
    int checkX1 = (int)std::floor(player.x - (player.width / 2.0f) + 0.05f + 0.5f);
    int checkX2 = (int)std::floor(player.x + (player.width / 2.0f) - 0.05f + 0.5f);

    if (checkY >= 0 && checkY < MAP_HEIGHT) {
        bool blockUnderLeft = (checkX1 >= 0 && checkX1 < MAP_WIDTH) && (worldMap[1][checkY][checkX1] != AIR);
        bool blockUnderRight = (checkX2 >= 0 && checkX2 < MAP_WIDTH) && (worldMap[1][checkY][checkX2] != AIR);

        if (blockUnderLeft || blockUnderRight) {
            float blockTop = (float)checkY + 0.5f;
            // Если игрок опустился ниже верхушки блока — ставим его на блок
            if (footY <= blockTop && player.velY <= 0) {
                player.y = blockTop + (player.height / 2.0f);
                player.velY = 0;
                player.isOnGround = true;
            }
        }
    }

    // Обработка клика ЛКМ для разрушения блоков
    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (mouseState == GLFW_PRESS && !mousePressedLastFrame) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        int w, h;
        glfwGetWindowSize(window, &w, &h);
        float aspect = (float)w / (float)h;

        // Перевод экранных пикселей в нормализованные координаты OpenGL (-1.0 до 1.0)
        float normX = (2.0f * (float)mouseX) / w - 1.0f;
        float normY = 1.0f - (2.0f * (float)mouseY) / h; // Инвертируем Y, так как в OpenGL он идет снизу вверх...

        // Перевод координат OpenGL в игровые координаты мира с учетом камеры и масштаба...
        float worldMouseX = (normX * aspect * VISIBLE_HEIGHT / 2.0f) + cameraX;
        float worldMouseY = (normY * VISIBLE_HEIGHT / 2.0f) + cameraY;

        // Точные координаты блока, на который НАЖАЛИ
        int targetX = (int)std::floor(worldMouseX + 0.5f);
        int targetY = (int)std::floor(worldMouseY + 0.5f);

        if (targetX >= 0 && targetX < MAP_WIDTH && targetY >= 0 && targetY < MAP_HEIGHT) {

            // Динамически определяем лимит дистанции: если передний блок пустой, значит целимся в задний (радиус 4), иначе в передний (радиус 5)
            float maxBreakDistance = 5.0f;
            if (worldMap[1][targetY][targetX] == AIR && worldMap[0][targetY][targetX] != AIR) {
                maxBreakDistance = 4.0f;
            }

            // Расчет позиции СЕРЕДИНЫ головы игрока (чуть ниже макушки)
            float headX = player.x;
            float headY = player.y + (player.height / 2.0f) - 0.2f;

            // Вектор от головы до КУРСОРA МЫШИ для проверки радиуса действия
            float dirMouseX = worldMouseX - headX;
            float dirMouseY = worldMouseY - headY;
            float distanceToMouse = std::sqrt(dirMouseX * dirMouseX + dirMouseY * dirMouseY);

            if (distanceToMouse <= maxBreakDistance) {

                // Теперь пускаем луч строго в ЦЕНТР целевого блока, чтобы не было случайных зацепов соседних блоков
                float targetCenterX = (float)targetX;
                float targetCenterY = (float)targetY;

                float dirX = targetCenterX - headX;
                float dirY = targetCenterY - headY;
                float distanceToCenter = std::sqrt(dirX * dirX + dirY * dirY);

                bool hitWall = false;

                if (distanceToCenter > 0.0f) {
                    float stepX = dirX / distanceToCenter;
                    float stepY = dirY / distanceToCenter;

                    float currentLen = 0.0f;
                    float stepDist = 0.05f; // Длина одного микро-шага луча для точности

                    // Шагаем лучом от головы, но ОСТАНАВЛИВАЕМСЯ чуть-чуть не доходя до центра целевого блока
                    // (минус 0.4f, чтобы не проверять сам целевой блок на роль "преграды")
                    float maxCheckLen = distanceToCenter - 0.4f;

                    while (currentLen < maxCheckLen) {
                        float checkX = headX + stepX * currentLen;
                        float checkY_ray = headY + stepY * currentLen;

                        int bX = (int)std::floor(checkX + 0.5f);
                        int bY = (int)std::floor(checkY_ray + 0.5f);

                        if (bX >= 0 && bX < MAP_WIDTH && bY >= 0 && bY < MAP_HEIGHT) {
                            // Если на пути к цели встретили ДРУГОЙ твердый блок переднего слоя — это преграда
                            if ((bX != targetX || bY != targetY) && worldMap[1][bY][bX] != AIR) {
                                hitWall = true;
                                break;
                            }
                        }
                        currentLen += stepDist;
                    }
                }

                // Если преград на пути нет — сносим именно тот блок, на который ткнули
                if (!hitWall) {
                    if (worldMap[1][targetY][targetX] != AIR) {
                        worldMap[1][targetY][targetX] = AIR; // Ломаем передний план
                    }
                    else if (worldMap[0][targetY][targetX] != AIR) {
                        worldMap[0][targetY][targetX] = AIR; // Если передний пуст, ломаем задний
                    }
                }
            }
        }
    }
    mousePressedLastFrame = (mouseState == GLFW_PRESS);

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
    activeOffsets.reserve(4000); // Резерв увеличен, так как теперь слоев два
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

        updatePhysics(window, deltaTime);

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
                    else if (block == DIRT) { r = 0.5f;  g = 0.3f;  b = 0.1f; }
                    else if (block == STONE) { r = 0.4f;  g = 0.4f;  b = 0.42f; }
                    else if (block == WOOD) { r = 0.4f;  g = 0.25f; b = 0.1f; }
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
// ставить блоки на переднем пкм, на заднем - с шифтом мб
// 
// добавить инвентарь
// 
// 
// 
// Потом добавить облака, музыку, мобов