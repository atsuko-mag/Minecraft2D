#pragma once
#include <vector>
#include <cstdlib> // Нужен для rand() в конструкторе дропа

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
const float PHYSICS_GRAVITY = 0.010f;      // сила притяжения (чем меньше, тем медленнее падение)
const float PHYSICS_JUMP_POWER = 0.16f;     // высота прыжка
const float PHYSICS_WALK_SPEED = 0.12f;      // максимальная скорость ходьбы
const float PHYSICS_ACCEL = 0.008f;          // ускорение (плавность разгона)
const float PHYSICS_FRICTION = 6.0f;       // трение (плавность остановки)

// Типы предметов (изм.)
enum ItemType { AIR = 0, GRASS = 1, DIRT = 2, STONE = 3, WOOD = 4, LEAVES = 5, APPLE = 6 };

// Структура для выпавших предметов
struct DroppedItem {
    int type;
    float x, y;
    float velX, velY;
    bool isOnGround;
    float animTime; // для плавной левитации :D
    float groundY;  // Точка земли, над которой завис предмет

    DroppedItem(int t, float px, float py)
        : type(t), x(px), y(py), velY(0.12f), isOnGround(false), animTime(0.0f), groundY(0.0f) {
        // Задаем случайное направление вылета
        int randStep = rand() % 7;
        velX = -0.12f + (randStep * 0.04f);
    }
};

extern std::vector<DroppedItem> droppedItems;

// Структура ячейки инвентаря
struct InventorySlot {
    int type = AIR;
    int count = 0;
};

extern InventorySlot inventory[9];
extern int selectedSlot; // Выбранный слот от 0 до 8

// Двухслойная карта: [0] - задний слой, [1] - передний слой (игрок ходит здесь)
extern int worldMap[2][MAP_HEIGHT][MAP_WIDTH];

// Структура игрока
struct Player {
    float x, y;             // Позиция центра персонажа
    float velX, velY;       // Текущая скорость по осям
    float width = 0.6f;     // Ширина хитбокса
    float height = 1.8f;    // Высота хитбокса
    bool isOnGround;        // Флаг: стоит ли персонаж на земле
    Player() : x(20.0f), y(25.0f), velX(0), velY(0), isOnGround(false) {}
};

extern Player player;
extern float cameraX;       // Позиция камеры по X
extern float cameraY;       // Позиция камеры по Y

// Флаги для отслеживания кликов мыши
extern bool mousePressedLastFrame;
extern bool rmousePressedLastFrame;

// Вспомогательная функция для добавления предметов в инвентарь
void addItemToInventory(int type);