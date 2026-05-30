#include "Config.h"

unsigned long long next_random = 1337; // Стартовое число
int get_random() {
    next_random = next_random * 1103515245 + 12345;
    return (int)(next_random / 65536) % 32768;
}

// КОНСТАНТЫ
const int GROUND_LEVEL = 60;
const float VISIBLE_HEIGHT = 14.0f;

const float SKY_R = 0.5f;
const float SKY_G = 0.8f;
const float SKY_B = 1.0f;

const float PHYSICS_GRAVITY = 0.010f;
const float PHYSICS_JUMP_POWER = 0.16f;
const float PHYSICS_WALK_SPEED = 0.12f;
const float PHYSICS_MOBS_WALK_SPEED = 0.03f;
const float PHYSICS_ACCEL = 0.008f;
const float PHYSICS_FRICTION = 6.0f;



std::vector<DroppedItem> droppedItems;
InventorySlot inventory[9];
int selectedSlot = 0;

InventorySlot craftGrid[2][2] = { {{AIR, 0}, {AIR, 0}}, {{AIR, 0}, {AIR, 0}} };
InventorySlot craftResult = { AIR, 0 };

int worldMap[2][MAP_HEIGHT][MAP_WIDTH] = { 0 };
std::vector<Sheep> sheeps;
Player player;

float cameraX = 0.0f;
float cameraY = 0.0f;
bool mousePressedLastFrame = false;
bool rmousePressedLastFrame = false;

//UI
InventorySlot cursorSlot = { AIR, 0 };
float lastBorderScaleX = 0.0f, lastStartUiX = 0.0f, lastSlotStepX = 0.0f, lastBorderScaleY = 0.0f, lastUiY = -0.85f;
bool isCraftingOpen = false;
float craftBtnScaleX = 0.0f, craftBtnScaleY = 0.0f, craftBtnCenterX = 0.0f, craftBtnCenterY = 0.0f;

InventorySlot draggedItem = { AIR, 0 };
int sourceSlotIndex = -1, sourceCraftRow = -1, sourceCraftCol = -1, sourceIsResult = 0;
bool isDragging = false;

// эти сами обновятся, поэтому пока ноль
float lastMenuW = 0.0f, lastMenuH = 0.0f, lastMenuCenterX = 0.0f, lastMenuCenterY = 0.0f;
float lastCraftSlotW = 0.0f, lastCraftSlotH = 0.0f, lastGridStartX = 0.0f, lastGridStartY = 0.0f;
float lastCraftPaddingX = 0.0f, lastCraftPaddingY = 0.0f, lastResultCenterX = 0.0f, lastResultCenterY = 0.0f;