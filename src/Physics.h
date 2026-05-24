#pragma once
#include <GLFW/glfw3.h>
#include "Config.h"
#include <cmath>
#include <algorithm>
#include <vector>

// Выделяем переменные из Config.h, чтобы линкер не ругался...
std::vector<DroppedItem> droppedItems;
InventorySlot inventory[9];
int selectedSlot = 0;
int worldMap[2][MAP_HEIGHT][MAP_WIDTH] = { 0 };
Player player;
float cameraX = 0.0f;
float cameraY = 0.0f;
bool mousePressedLastFrame = false;
bool rmousePressedLastFrame = false;

void addItemToInventory(int type) {
    for (int i = 0; i < 9; i++) {
        if (inventory[i].type == type && inventory[i].count < 64) {
            inventory[i].count++;
            return;
        }
    }
    for (int i = 0; i < 9; i++) {
        if (inventory[i].type == AIR) {
            inventory[i].type = type;
            inventory[i].count = 1;
            return;
        }
    }
}

// Функция для проверки: можно ли поставить блок в указанную точку
bool canPlaceBlock(int targetX, int targetY, int layerToPlace, bool& outBuildingUnderSelf) {
    if (targetX < 0 || targetX >= MAP_WIDTH || targetY < 0 || targetY >= MAP_HEIGHT) return false;
    if (layerToPlace < 0 || layerToPlace > 1) return false;

    // Проверка на то, свободно ли место под блок
    if (worldMap[layerToPlace][targetY][targetX] != AIR) return false;

    // Если хотим поставить на задний слой, а передний уже занят, строить низя
    if (layerToPlace == 0 && worldMap[1][targetY][targetX] != AIR) return false;

    // Какой блок ставим
    int currentBlockInHand = inventory[selectedSlot].type;
    if (currentBlockInHand == AIR || currentBlockInHand == APPLE) return false;

    // Предсказание положения игрока через полтора кадра (убирает рассинхрон курсора)
    float leadFactor = 1.5f;
    float predictedPlayerX = player.x + (player.velX * leadFactor * 0.016f);
    float predictedPlayerY = player.y + (player.velY * leadFactor * 0.016f);

    // Границы игрока (используя предсказанные координаты)
    int pGridX = (int)std::floor(predictedPlayerX + 0.5f);
    float playerBottomY = predictedPlayerY - player.height / 2.0f;
    int cellUnderFeetY = (int)std::floor(playerBottomY + 0.05f + 0.5f);
    int targetBuildY = cellUnderFeetY - 1;

    outBuildingUnderSelf = false;

    // Снарая стройка под себя
    if (std::abs(targetX - pGridX) <= 1 && targetY <= cellUnderFeetY) {
        // Жесткая проверка индексов для затыкания ворнинга C6386
        if (targetBuildY >= 0 && targetBuildY < MAP_HEIGHT && pGridX >= 0 && pGridX < MAP_WIDTH) {
            if (worldMap[layerToPlace][targetBuildY][pGridX] == AIR) {
                if (layerToPlace == 0 && worldMap[1][targetBuildY][pGridX] != AIR) {
                    // пропускаем, нельзя сквозь блок
                }
                else {
                    targetX = pGridX;
                    targetY = targetBuildY;
                    outBuildingUnderSelf = true;
                }
            }
        }
    }
    if (targetX < 0 || targetX >= MAP_WIDTH || targetY < 0 || targetY >= MAP_HEIGHT) return false;

    float distX = (float)targetX - predictedPlayerX;
    float distY = (float)targetY - predictedPlayerY;
    if (std::sqrt(distX * distX + distY * distY) > 5.0f) return false;

    // Флаг, есть ли блоки рядом с тем, куда хотим поставить
    bool hasNeighbor = outBuildingUnderSelf;

    // Противоположный слой от того, где хотим поставить блок
    int oppositeLayer = 1 - layerToPlace;

    if (!hasNeighbor) {
        if (worldMap[oppositeLayer][targetY][targetX] != AIR) hasNeighbor = true;
        if (targetX > 0 && targetX - 1 < MAP_WIDTH) {
            if (worldMap[layerToPlace][targetY][targetX - 1] != AIR) hasNeighbor = true;
        }
        if (targetX >= 0 && targetX + 1 < MAP_WIDTH) {
            if (worldMap[layerToPlace][targetY][targetX + 1] != AIR) hasNeighbor = true;
        }
        if (targetY > 0 && targetY - 1 < MAP_HEIGHT) {
            if (worldMap[layerToPlace][targetY - 1][targetX] != AIR) hasNeighbor = true;
        }
        if (targetY >= 0 && targetY + 1 < MAP_HEIGHT) {
            if (worldMap[layerToPlace][targetY + 1][targetX] != AIR) hasNeighbor = true;
        }
    }

    if (!hasNeighbor) return false;

    if (layerToPlace == 1 && !outBuildingUnderSelf) {
        float bLeft = (float)targetX - 0.5f;
        float bRight = (float)targetX + 0.5f;
        float bBottom = (float)targetY - 0.5f;
        float bTop = (float)targetY + 0.5f;

        // Хитбокс чут помешне 
        float tolerance = 0.02f;
        float pLeft = (predictedPlayerX - player.width / 2.0f) + tolerance;
        float pRight = (predictedPlayerX + player.width / 2.0f) - tolerance;
        float pBottom = (predictedPlayerY - player.height / 2.0f) + tolerance;
        float pTop = (predictedPlayerY + player.height / 2.0f) - tolerance;

        if (pLeft < bRight && pRight > bLeft && pBottom < bTop && pTop > bBottom) {
            return false;
        }
    }

    return true;
}

// ФИЗИКА, КОЛЛИЗИИ И ОБРАБОТКА КЛИКОВ
void updatePhysics(GLFWwindow* window, float dt, double mouseX, double mouseY) {
    if (dt > 0.033f) dt = 0.033f;

    // Позиция до движения
    float oldPlayerY = player.y;

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

    // Применение гравитации и обновление координат игрока
    player.velY -= PHYSICS_GRAVITY * dtMultiplier;
    player.y += player.velY * dtMultiplier;
    player.x += player.velX * dtMultiplier;

    // Закрываем окно игры, если игрок выпал за нижнюю границу карты в минус
    if (player.y < 0.0f) {
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }

    // КОЛЛИЗИЯ С ПОТОЛКОМ
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

    float oldFootY = oldPlayerY - (player.height / 2.0f);
    int checkY = (int)std::floor(oldFootY + 0.5f);

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

    // ОБРАБОТКА ДРОПА
    float itemSize = 0.25f;
    float itemRadius = itemSize / 2.0f;

    for (auto it = droppedItems.begin(); it != droppedItems.end();) {
        // Координаты хитбокса предмета по X
        int checkItemX1 = (int)std::floor((it->x - itemRadius + 0.01f) + 0.5f);
        int checkItemX2 = (int)std::floor((it->x + itemRadius - 0.01f) + 0.5f);

        if (!it->isOnGround) {
            float oldItemY = it->y; // Сохраняем старый Y предмета

            it->velY -= PHYSICS_GRAVITY * dtMultiplier;
            it->x += it->velX * dtMultiplier;

            int itemTileY = (int)std::floor(it->y + 0.5f);
            if (itemTileY >= 0 && itemTileY < MAP_HEIGHT) {
                if (it->velX > 0) {
                    int tileX = (int)std::floor((it->x + itemRadius) + 0.5f);
                    if (tileX >= 0 && tileX < MAP_WIDTH && worldMap[1][itemTileY][tileX] != AIR) {
                        it->x = (float)tileX - 0.5f - itemRadius - 0.005f;
                        it->velX = 0.0f;
                    }
                }
                else if (it->velX < 0) {
                    int tileX = (int)std::floor((it->x - itemRadius) + 0.5f);
                    if (tileX >= 0 && tileX < MAP_WIDTH && worldMap[1][itemTileY][tileX] != AIR) {
                        it->x = (float)tileX + 0.5f + itemRadius + 0.005f;
                        it->velX = 0.0f;
                    }
                }
            }

            it->y += it->velY * dtMultiplier;
            float itemFootY = it->y - itemRadius;

            int checkItemY = (int)std::floor((oldItemY - itemRadius) + 0.5f);

            if (checkItemY >= 0 && checkItemY < MAP_HEIGHT) {
                bool blockUnderLeft = (checkItemX1 >= 0 && checkItemX1 < MAP_WIDTH) && (worldMap[1][checkItemY][checkItemX1] != AIR);
                bool blockUnderRight = (checkItemX2 >= 0 && checkItemX2 < MAP_WIDTH) && (worldMap[1][checkItemY][checkItemX2] != AIR);

                if (blockUnderLeft || blockUnderRight) {
                    float blockTop = (float)checkItemY + 0.5f;
                    if (itemFootY <= blockTop && it->velY <= 0) {
                        it->groundY = blockTop + itemRadius;
                        it->y = it->groundY;
                        it->velY = 0.0f;
                        it->velX = 0.0f;
                        it->isOnGround = true;
                    }
                }
            }
        }
        else {
            // Проверяем под предметом есть ли пол
            int checkItemY = (int)std::floor((it->groundY - itemRadius - 0.01f) + 0.5f);

            bool groundStillExists = false;
            if (checkItemY >= 0 && checkItemY < MAP_HEIGHT) {
                bool blockUnderLeft = (checkItemX1 >= 0 && checkItemX1 < MAP_WIDTH) && (worldMap[1][checkItemY][checkItemX1] != AIR);
                bool blockUnderRight = (checkItemX2 >= 0 && checkItemX2 < MAP_WIDTH) && (worldMap[1][checkItemY][checkItemX2] != AIR);

                if (blockUnderLeft || blockUnderRight) {
                    groundStillExists = true;
                }
            }

            if (!groundStillExists) {
                it->isOnGround = false; // Летит вниз, если блок сломали под ним
            }
            else {
                it->animTime += dt * 4.0f; // Левитирует
            }
        }

        // Подбор предметов игроком
        float distToPlayerX = std::abs(it->x - player.x);
        float distToPlayerY = std::abs(it->y - player.y);
        if (distToPlayerX < (player.width / 2.0f + 0.15f) && distToPlayerY < (player.height / 2.0f + 0.15f)) {
            addItemToInventory(it->type);
            it = droppedItems.erase(it);
        }
        else {
            ++it;
        }
    }

    // Перевод координат мыши в нормальные
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    float aspect = (float)w / (float)h;

    float normX = (2.0f * (float)mouseX) / w - 1.0f;
    float normY = 1.0f - (2.0f * (float)mouseY) / h;
    float worldMouseX = (normX * aspect * VISIBLE_HEIGHT / 2.0f) + cameraX;
    float worldMouseY = (normY * VISIBLE_HEIGHT / 2.0f) + cameraY;
    int targetX = (int)std::floor(worldMouseX + 0.5f);
    int targetY = (int)std::floor(worldMouseY + 0.5f);

    // Обработка клика ЛКМ для разрушения блоков
    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (mouseState == GLFW_PRESS && !mousePressedLastFrame) {
        if (targetX >= 0 && targetX < MAP_WIDTH && targetY >= 0 && targetY < MAP_HEIGHT) {

            float maxBreakDistance = 5.0f;
            if (worldMap[1][targetY][targetX] == AIR && worldMap[0][targetY][targetX] != AIR) {
                maxBreakDistance = 4.0f;
            }

            float headX = player.x;
            float headY = player.y + (player.height / 2.0f) - 0.2f;

            float dirMouseX = worldMouseX - headX;
            float dirMouseY = worldMouseY - headY;
            float distanceToMouse = std::sqrt(dirMouseX * dirMouseX + dirMouseY * dirMouseY);

            if (distanceToMouse <= maxBreakDistance) {

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
                    float stepDist = 0.05f;
                    float maxCheckLen = distanceToCenter - 0.4f;

                    while (currentLen < maxCheckLen) {
                        float checkX = headX + stepX * currentLen;
                        float checkY_ray = headY + stepY * currentLen;

                        int bX = (int)std::floor(checkX + 0.5f);
                        int bY = (int)std::floor(checkY_ray + 0.5f);

                        if (bX >= 0 && bX < MAP_WIDTH && bY >= 0 && bY < MAP_HEIGHT) {
                            if ((bX != targetX || bY != targetY) && worldMap[1][bY][bX] != AIR) {
                                hitWall = true;
                                break;
                            }
                        }
                        currentLen += stepDist;
                    }
                }

                if (!hitWall) {
                    int brokenType = AIR;
                    int targetLayer = -1;

                    if (worldMap[1][targetY][targetX] != AIR) {
                        brokenType = worldMap[1][targetY][targetX];
                        worldMap[1][targetY][targetX] = AIR;
                        targetLayer = 1;
                    }
                    else if (worldMap[0][targetY][targetX] != AIR) {
                        brokenType = worldMap[0][targetY][targetX];
                        worldMap[0][targetY][targetX] = AIR;
                        targetLayer = 0;
                    }
                    if (targetLayer != -1 && brokenType != AIR) {
                        float randVelX = ((float)(rand() % 200 - 100) / 100.0f) * 0.05f;
                        float randVelY = ((float)(rand() % 100) / 100.0f) * 0.04f + 0.03f;

                        DroppedItem newItem(brokenType, (float)targetX, (float)targetY);
                        newItem.velX = randVelX;
                        newItem.velY = randVelY;
                        newItem.isOnGround = false;

                        if (brokenType == LEAVES) {
                            if ((rand() % 100) < 10) {
                                newItem.type = APPLE;
                                droppedItems.push_back(newItem);
                            }
                        }
                        else {
                            droppedItems.push_back(newItem);
                        }
                    }
                }
            }
        }
    }
    mousePressedLastFrame = (mouseState == GLFW_PRESS);

    // ПКМ установка блоков
    int rmouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    if (rmouseState == GLFW_PRESS && !rmousePressedLastFrame) {
        bool shiftPressed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
        int layerToPlace = shiftPressed ? 0 : 1;
        bool buildingUnderSelf = false;

        if (canPlaceBlock(targetX, targetY, layerToPlace, buildingUnderSelf)) {
            int currentBlockInHand = inventory[selectedSlot].type;

            if (buildingUnderSelf) {
                int pGridX = (int)std::floor(player.x + 0.5f);
                float playerBottomY = player.y - player.height / 2.0f;
                int cellUnderFeetY = (int)std::floor(playerBottomY + 0.05f + 0.5f);
                targetX = pGridX;
                targetY = cellUnderFeetY - 1;
            }

            if (targetX >= 0 && targetX < MAP_WIDTH && targetY >= 0 && targetY < MAP_HEIGHT) {
                worldMap[layerToPlace][targetY][targetX] = currentBlockInHand;
                inventory[selectedSlot].count--;
                if (inventory[selectedSlot].count <= 0) {
                    inventory[selectedSlot].type = AIR;
                    inventory[selectedSlot].count = 0;
                }
            }
        }
    }
    rmousePressedLastFrame = (rmouseState == GLFW_PRESS);

    // Камера
    float cameraSmoothing = 1.0f - std::pow(0.1f, dt * 5.0f);
    cameraX += (player.x - cameraX) * cameraSmoothing;
    cameraY += (player.y - cameraY) * cameraSmoothing;
}