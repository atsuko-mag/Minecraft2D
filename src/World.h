#pragma once
#include "Config.h"
#include <algorithm>

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