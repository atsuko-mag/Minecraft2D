#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Config.h"
#include "CallBackFunctions.h"


void updateCraftingRecipe() {

    // ПОКА КОСТЫЛЬНО ДАА, потом нормально будет, если я до этого дойду
    int woodCount = 0;
    int occupiedSlots = 0;

    // Считаем, сколько ячеек занято в сетке 2x2
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 2; col++) {
            if (craftGrid[row][col].type != AIR) {
                occupiedSlots++;
                if (craftGrid[row][col].type == WOOD) {
                    woodCount = craftGrid[row][col].count;
                }
            }
        }
    }

    if (occupiedSlots == 1 && woodCount > 0) {
        craftResult.type = PLANKS;
        craftResult.count = 4;
    }
    else {
        craftResult = { AIR, 0 };
    }
}

bool tryMoveToInventory(InventorySlot item) {
    if (item.type == AIR || item.count <= 0) return false;

    // Сначала ищем ячейку с таким же типом (доски), где есть место
    for (int i = 0; i < 9; i++) {
        if (inventory[i].type == item.type && inventory[i].count < 64) {
            int room = 64 - inventory[i].count;
            if (item.count <= room) {
                inventory[i].count += item.count;
                return true;
            }
            else {
                inventory[i].count = 64;
                item.count -= room;
            }
        }
    }

    // Если не добрали, или таких блоков не было, ищем пустую ячейку
    for (int i = 0; i < 9; i++) {
        if (inventory[i].type == AIR) {
            inventory[i] = item;
            return true;
        }
    }

    return false; // Нет места в инвентаре
}


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
    if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        isCraftingOpen = !isCraftingOpen;
    }
    if (action == GLFW_PRESS && key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
        selectedSlot = key - GLFW_KEY_1;
    }
}

// ЛКМ для выбора ячейки инвентаря на экране

// 1. Сначала вспомогательная функция
void handleSlotClick(InventorySlot& cell, int button, bool isResultSlot) {
    if (cursorSlot.type == AIR) {
        if (cell.type == AIR || cell.count <= 0) return;

        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            cursorSlot = cell;
            cell.type = AIR;
            cell.count = 0;
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            int taken = (cell.count + 1) / 2;
            int left = cell.count - taken;

            cursorSlot.type = cell.type;
            cursorSlot.count = taken;

            if (isResultSlot) {
                cell.type = AIR;
                cell.count = 0;
            }
            else {
                if (left > 0) cell.count = left;
                else { cell.type = AIR; cell.count = 0; }
            }
        }
    }
    else {
        if (isResultSlot) return;

        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (cell.type == AIR) {
                cell = cursorSlot;
                cursorSlot = { AIR, 0 };
            }
            else if (cell.type == cursorSlot.type) {
                int roomLeft = 64 - cell.count;
                if (roomLeft > 0) {
                    if (cursorSlot.count <= roomLeft) {
                        cell.count += cursorSlot.count;
                        cursorSlot = { AIR, 0 };
                    }
                    else {
                        cell.count = 64;
                        cursorSlot.count -= roomLeft;
                    }
                }
            }
            else {
                InventorySlot temp = cell;
                cell = cursorSlot;
                cursorSlot = temp;
            }
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (cell.type == AIR) {
                cell.type = cursorSlot.type;
                cell.count = 1;
                cursorSlot.count--;
                if (cursorSlot.count <= 0) cursorSlot = { AIR, 0 };
            }
            else if (cell.type == cursorSlot.type) {
                if (cell.count < 64) {
                    cell.count++;
                    cursorSlot.count--;
                    if (cursorSlot.count <= 0) cursorSlot = { AIR, 0 };
                }
            }
        }
    }
}

void glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (action != GLFW_PRESS) return;
    if (button != GLFW_MOUSE_BUTTON_LEFT && button != GLFW_MOUSE_BUTTON_RIGHT) return;

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    int w, h;
    glfwGetWindowSize(window, &w, &h);
    float aspect = (float)w / (float)h;

    // Переводим пиксельные координаты экрана в координаты от -1.0f до 1.0f
    float ndcX = (2.0f * (float)mouseX) / (float)w - 1.0f;
    float ndcY = 1.0f - (2.0f * (float)mouseY) / (float)h;

    bool isShift = (mods & GLFW_MOD_SHIFT);

    // Кнопка открытия/закрытия крафта
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (ndcX >= (craftBtnCenterX - craftBtnScaleX / 2.0f) / aspect &&
            ndcX <= (craftBtnCenterX + craftBtnScaleX / 2.0f) / aspect &&
            ndcY >= (craftBtnCenterY - craftBtnScaleY / 2.0f) &&
            ndcY <= (craftBtnCenterY + craftBtnScaleY / 2.0f)) {
            isCraftingOpen = !isCraftingOpen;
            updateCraftingRecipe(); // Обновляем при открытии/закрытии
            return;
        }
    }

    bool clickedOnUI = false;

    // 1. Клик по инвентарю
    float halfHeight = lastBorderScaleY / 2.0f;
    if (ndcY >= (lastUiY - halfHeight) && ndcY <= (lastUiY + halfHeight)) {
        for (int i = 0; i < 9; i++) {
            float slotCenterX = lastStartUiX + i * lastSlotStepX;
            if (ndcX >= (slotCenterX - lastBorderScaleX / 2.0f) / aspect &&
                ndcX <= (slotCenterX + lastBorderScaleX / 2.0f) / aspect) {

                clickedOnUI = true;
                handleSlotClick(inventory[i], button, isShift);
                updateCraftingRecipe();
                return;
            }
        }
    }

    // 2. Клик по крафту
    if (isCraftingOpen) {
        for (int row = 0; row < 2; row++) {
            for (int col = 0; col < 2; col++) {
                float cx = lastGridStartX + col * (lastCraftSlotW + lastCraftPaddingX);
                float cy = lastGridStartY - row * (lastCraftSlotH + lastCraftPaddingY);

                if (ndcX >= (cx - lastCraftSlotW / 2.0f) / aspect && ndcX <= (cx + lastCraftSlotW / 2.0f) / aspect &&
                    ndcY >= (cy - lastCraftSlotH / 2.0f) && ndcY <= (cy + lastCraftSlotH / 2.0f)) {

                    clickedOnUI = true;
                    handleSlotClick(craftGrid[row][col], button, isShift);
                    updateCraftingRecipe();
                    return;
                }
            }
        }

        // 3. Ячейка результат крафта
        if (ndcX >= (lastResultCenterX - lastCraftSlotW / 2.0f) / aspect && ndcX <= (lastResultCenterX + lastCraftSlotW / 2.0f) / aspect &&
            ndcY >= (lastResultCenterY - lastCraftSlotH / 2.0f) && ndcY <= (lastResultCenterY + lastCraftSlotH / 2.0f)) {

            clickedOnUI = true;
            InventorySlot& cell = craftResult;

            if (cell.type != AIR && cell.count > 0) {
                InventorySlot* sourceWoodSlot = nullptr;
                for (int r = 0; r < 2; r++) {
                    for (int c = 0; c < 2; c++) {
                        if (craftGrid[r][c].type == WOOD) {
                            sourceWoodSlot = &craftGrid[r][c];
                        }
                    }
                }

                if (!sourceWoodSlot) return;

                if (isShift) {
                    if (button == GLFW_MOUSE_BUTTON_LEFT) {
                        InventorySlot planksToInv = { PLANKS, 4 };
                        if (tryMoveToInventory(planksToInv)) {
                            sourceWoodSlot->count -= 1;
                            if (sourceWoodSlot->count <= 0) *sourceWoodSlot = { AIR, 0 };
                        }
                    }
                    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                        int possiblePlanks = sourceWoodSlot->count * 4;
                        InventorySlot planksToInv = { PLANKS, possiblePlanks };
                        if (tryMoveToInventory(planksToInv)) {
                            *sourceWoodSlot = { AIR, 0 };
                        }
                    }
                }
                else {
                    if (button == GLFW_MOUSE_BUTTON_LEFT) {
                        if (cursorSlot.type == AIR) {
                            cursorSlot.type = PLANKS;
                            cursorSlot.count = 4;
                            sourceWoodSlot->count -= 1;
                            if (sourceWoodSlot->count <= 0) *sourceWoodSlot = { AIR, 0 };
                        }
                        else if (cursorSlot.type == PLANKS && cursorSlot.count <= 60) {
                            cursorSlot.count += 4;
                            sourceWoodSlot->count -= 1;
                            if (sourceWoodSlot->count <= 0) *sourceWoodSlot = { AIR, 0 };
                        }
                    }
                    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                        int possiblePlanks = sourceWoodSlot->count * 4;
                        if (cursorSlot.type == AIR) {
                            cursorSlot.type = PLANKS;
                            cursorSlot.count = possiblePlanks;
                            *sourceWoodSlot = { AIR, 0 };
                        }
                        else if (cursorSlot.type == PLANKS && cursorSlot.count + possiblePlanks <= 64) {
                            cursorSlot.count += possiblePlanks;
                            *sourceWoodSlot = { AIR, 0 };
                        }
                    }
                }
            }
            updateCraftingRecipe();
            return;
        }
    }
    // 4. Клик мимо UI
    if (!clickedOnUI) {
        if (cursorSlot.type != AIR) {
            // Переводим координаты мыши в координаты мира.......
            float worldMouseX = (ndcX * aspect * VISIBLE_HEIGHT / 2.0f) + cameraX;
            float worldMouseY = (ndcY * VISIBLE_HEIGHT / 2.0f) + cameraY;

            int blockGridX = (int)floor(worldMouseX);
            int blockGridY = (int)floor(worldMouseY);

            // Проверяем, куда кликнули: в воздух или блок
            bool isMouseOverAir = true;
            if (blockGridY >= 0 && blockGridY < MAP_HEIGHT && blockGridX >= 0 && blockGridX < MAP_WIDTH) {
                if (worldMap[1][blockGridY][blockGridX] != AIR) {
                    isMouseOverAir = false;
                }
            }

            // Если в воздух - выбос лута
            if (isMouseOverAir) {
                int dropCount = (button == GLFW_MOUSE_BUTTON_LEFT) ? cursorSlot.count : 1;

                for (int c = 0; c < dropCount; c++) {
                    DroppedItem drop;
                    drop.x = worldMouseX;
                    drop.y = worldMouseY;
                    drop.type = cursorSlot.type;
                    drop.velX = 0.0f;
                    drop.velY = 0.0f;
                    drop.isOnGround = false;
                    drop.animTime = (float)(rand() % 100);
                    droppedItems.push_back(drop);
                }

                cursorSlot.count -= dropCount;
                if (cursorSlot.count <= 0) {
                    cursorSlot.type = AIR;
                    cursorSlot.count = 0;
                }
            }
            // Еначе возврат в инвентарь
            else {
                if (tryMoveToInventory(cursorSlot)) {
                    cursorSlot = { AIR, 0 };
                }
            }

            updateCraftingRecipe();
            return;
            
        }
    }
}