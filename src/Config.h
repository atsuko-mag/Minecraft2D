#pragma once
#include <vector>
#include <cstdlib>

typedef long long ll;

//extern unsigned long long next_random;
//int get_random();
enum ItemType { AIR = 0, GRASS = 1, DIRT = 2, STONE = 3, WOOD = 4, LEAVES = 5, APPLE = 6, PLANKS = 7 };

struct InventorySlot {
    int type = AIR;
    int count = 0;
};

// СТРУКТУРЫ
struct DroppedItem {
    int type;
    float x, y;
    float velX, velY;
    bool isOnGround;
    float animTime;
    float groundY;

    DroppedItem() = default;
    DroppedItem(int t, float px, float py)
        : type(t), x(px), y(py), velY(0.12f), isOnGround(false), animTime(0.0f), groundY(0.0f) {
        int randStep = rand() % 7;
        velX = -0.12f + (randStep * 0.04f);
    }
};



struct Player {
    float x = 250.0f;
    float y = 60.1f;
    float velX = 0.0f;
    float velY = 0.0f;
    float width = 0.6f;
    float height = 1.8f;
    bool isOnGround = false;

    Player::Player() : x(250.0f), y(50.0), velX(0), velY(0), isOnGround(false) {}
};

struct Sheep {
    float x = 0.0f;
    float y = 0.0f;
    float velX = 0.0f;
    float velY = 0.0f;
    float width = 1.35f;
    float height = 1.0f;
    bool isOnGround = false;
    float aiTimer = 0.0f;
    int currentAction = 0;

    Sheep::Sheep(float x, float y) : x(x), y(y) {}
};



// КОНСТАНТЫ
extern const int GROUND_LEVEL;
extern const float VISIBLE_HEIGHT;

extern const float SKY_R;
extern const float SKY_G;
extern const float SKY_B;

extern const float PHYSICS_GRAVITY;
extern const float PHYSICS_JUMP_POWER;
extern const float PHYSICS_WALK_SPEED;
extern const float PHYSICS_MOBS_WALK_SPEED;
extern const float PHYSICS_ACCEL;
extern const float PHYSICS_FRICTION;


const int MAP_WIDTH = 500;
const int MAP_HEIGHT = 100;
extern int worldMap[2][MAP_HEIGHT][MAP_WIDTH];
extern std::vector<Sheep> sheeps;
extern Player player;

extern float cameraX;
extern float cameraY;
extern bool mousePressedLastFrame;
extern bool rmousePressedLastFrame;

extern InventorySlot cursorSlot; // Предмет, удерживаемый мышкой

// Для инвентаря (UI)
extern float lastBorderScaleX;
extern float lastStartUiX;
extern float lastSlotStepX;
extern float lastBorderScaleY;
extern float lastUiY;

// Переменные для меню крафта
extern bool isCraftingOpen;

// Размеры кнопки открытия крафта
extern float craftBtnScaleX;
extern float craftBtnScaleY;
extern float craftBtnCenterX;
extern float craftBtnCenterY;

// Переменные для поднятых предметов
extern InventorySlot draggedItem;
extern int sourceSlotIndex;
extern int sourceCraftRow;
extern int sourceCraftCol;
extern int sourceIsResult; 
extern bool isDragging;

// Размеры и координаты интерфейса крафта
extern float lastMenuW;
extern float lastMenuH;
extern float lastMenuCenterX;
extern float lastMenuCenterY;
extern float lastCraftSlotW;
extern float lastCraftSlotH;
extern float lastGridStartX;
extern float lastGridStartY;
extern float lastCraftPaddingX;
extern float lastCraftPaddingY;
extern float lastResultCenterX;
extern float lastResultCenterY;


extern std::vector<DroppedItem> droppedItems;
extern InventorySlot inventory[9];
extern int selectedSlot;

extern InventorySlot craftGrid[2][2];
extern InventorySlot craftResult;