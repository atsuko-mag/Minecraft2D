#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>

typedef long long ll;

// константы для быстрой отладки.

//  НАСТРОЙКИ МИРА 
const int MAP_WIDTH = 200;      // ширина всей карты в блоках
const int MAP_HEIGHT = 100;     // высота карты в блоках
const int GROUND_LEVEL = 20;    // уровень поверхности (т.е травы)
const float VISIBLE_HEIGHT = 14.0f; // сколько блоков помещается в экран по вертикали

//  ФИЗИКА ДВИЖЕНИЯ 
const float PHYSICS_GRAVITY = 0.00004f;      // сила притяжения (чем меньше, тем медленнее падение)
const float PHYSICS_JUMP_POWER = 0.009f;     // высота прыжка
const float PHYSICS_WALK_SPEED = 0.01f;      // максимальная скорость ходьбы
const float PHYSICS_ACCEL = 0.005f;          // ускорение (плавность разгона)
const float PHYSICS_FRICTION = 0.004f;       // трение (плавность остановки) ПОКА РАБОТАЕТ СТРАННО

//  Типы блоков
enum BlockType { AIR = 0, GRASS = 1, DIRT = 2, STONE = 3, WOOD = 4, LEAVES = 5 };
int worldMap[MAP_HEIGHT][MAP_WIDTH] = { 0 };

//  Структура игрока
struct Player {
    float x, y;             // Позиция центра персонажа
    float velX, velY;       // Текущая скорость по осям
    float width = 0.6f;     // Ширина хитбокса
    float height = 1.8f;    // Высота хитбокса
    bool isOnGround;        // Флаг, стоит ли персонаж на земле
    Player() : x(20.0f), y(25.0f), velX(0), velY(0), isOnGround(false) {}
};

Player player;
float cameraX = 0.0f;       // Позиция камеры по X
float cameraY = 0.0f;       // Позиция камеры по Y

//  ИСХОДНЫЙ КОД ШЕЙДЕРОВ 
// Вершинный шейдер: отвечает за позиции объектов и учет пропорций экрана
const char* vertexShaderSource = "#version 460 core\n"
"layout (location = 0) in vec2 aPos;\n"
"uniform vec2 offset;\n"
"uniform vec2 scale;\n"
"uniform float aspect;\n"
"void main() {\n"
"   gl_Position = vec4((aPos.x * scale.x + offset.x) / aspect, aPos.y * scale.y + offset.y, 0.0, 1.0);\n"
"}\0";

// Фрагментный шейдер: отвечает за цвет пикселей
const char* fragmentShaderSource = "#version 460 core\n"
"out vec4 FragColor;\n"
"uniform vec3 blockColor;\n"
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
// Создание дерева
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

// Заполнение карты при старте
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
void updatePhysics(GLFWwindow* window) {
    // Обработка клавиш движения (A и D) с учетом ускорения
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        player.velX -= PHYSICS_ACCEL;
        if (player.velX < -PHYSICS_WALK_SPEED) player.velX = -PHYSICS_WALK_SPEED;
    }
    else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        player.velX += PHYSICS_ACCEL;
        if (player.velX > PHYSICS_WALK_SPEED) player.velX = PHYSICS_WALK_SPEED;
    }
    else {
        // Эффект трения при остановке
        if (player.velX > 0) {
            player.velX -= PHYSICS_FRICTION;
            if (player.velX < 0) player.velX = 0;
        }
        else if (player.velX < 0) {
            player.velX += PHYSICS_FRICTION;
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
    player.velY -= PHYSICS_GRAVITY;
    player.y += player.velY;
    player.x += player.velX;

    // Коллизия с уровнем земли (пока что так...)
    float groundTop = (float)GROUND_LEVEL + 0.5f;
    if (player.y - (player.height / 2.0f) <= groundTop) {
        player.y = groundTop + (player.height / 2.0f);
        player.velY = 0;
        player.isOnGround = true;
    }

    // Плавное следование камеры за персонажем
    cameraX += (player.x - cameraX) * 0.01f;
    cameraY += (player.y - cameraY) * 0.01f;
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

    // Главный цикл отрисовки
    while (!glfwWindowShouldClose(window)) {
        int w, h;
        glfwGetWindowSize(window, &w, &h);
        float aspect = (float)w / (float)h;

        updatePhysics(window);

        // Рисуем небо
        glClearColor(0.5f, 0.8f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);

        // Масштабирование блоков относительно высоты экрана
        float sy = 2.0f / VISIBLE_HEIGHT;
        float sx = sy;

        glUniform1f(glGetUniformLocation(program, "aspect"), aspect);
        glBindVertexArray(VAO);

        // Определение границ отрисовки (для оптимизации рисуем только то, что видит камера)
        int rangeX = (int)(VISIBLE_HEIGHT * aspect / 2) + 2;
        int rangeY = (int)(VISIBLE_HEIGHT / 2) + 2;
        int startX = std::max(0, (int)cameraX - rangeX);
        int endX = std::min(MAP_WIDTH, (int)cameraX + rangeX);
        int startY = std::max(0, (int)cameraY - rangeY);
        int endY = std::min(MAP_HEIGHT, (int)cameraY + rangeY);

        // Цикл отрисовки блоков мира
        for (int y = startY; y < endY; y++) {
            for (int x = startX; x < endX; x++) {
                if (worldMap[y][x] == AIR) continue;

                // Выбор цвета в зависимости от типа блока. ПОТОМ ТЕКСТУРКИ
                if (worldMap[y][x] == GRASS) glUniform3f(glGetUniformLocation(program, "blockColor"), 0.2f, 0.8f, 0.2f);
                else if (worldMap[y][x] == DIRT) glUniform3f(glGetUniformLocation(program, "blockColor"), 0.5f, 0.3f, 0.1f);
                else if (worldMap[y][x] == STONE) glUniform3f(glGetUniformLocation(program, "blockColor"), 0.4f, 0.4f, 0.42f);
                else if (worldMap[y][x] == WOOD) glUniform3f(glGetUniformLocation(program, "blockColor"), 0.4f, 0.25f, 0.1f);
                else if (worldMap[y][x] == LEAVES) glUniform3f(glGetUniformLocation(program, "blockColor"), 0.1f, 0.5f, 0.1f);

                // Передача позиции и отрисовка
                glUniform2f(glGetUniformLocation(program, "scale"), sx, sy);
                glUniform2f(glGetUniformLocation(program, "offset"), (x - cameraX) * sx, (y - cameraY) * sy);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        // Рисуем игрока
        glUniform3f(glGetUniformLocation(program, "blockColor"), 1.0f, 0.3f, 0.3f);
        glUniform2f(glGetUniformLocation(program, "scale"), sx * player.width, sy * player.height);
        glUniform2f(glGetUniformLocation(program, "offset"), (player.x - cameraX) * sx, (player.y - cameraY) * sy);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// ДЛЯ ПРОВЕРЯЮЩЕГО ЛАБОРАТОРНУЮ: 
// 
// Этот код был заготовлен мной изначально, поэтому коммиты происходят в один день.


// ПРОБЛЕМА ВЕРСИИ: отрисовка кадров может быть медленной. если пк подключен к сети, то всё рсуется нормально

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