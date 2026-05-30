#pragma once
#include "Config.h"
#include <algorithm>
#include <cmath>

// Функция, которая возвращает высоту холма для каждого X
int getSurfaceHeight(int x) {
    float baseHeight = 62.0f;
    // Волны 
    float wave1 = std::sin((float)x * 0.015f) * 12.0f; // Большие плавные возвышенности
    float wave2 = std::sin((float)x * 0.04f) * 3.0f;   // Мягкие перепады, чтобы не было плоско
    return (int)(baseHeight + wave1 + wave2);
}


//  ГЕНЕРАЦИЯ ОБЪЕКТОВ
void addTree(int x, int y) {
    int treetype = 1 + (rand() % 3);
    if (treetype == 1) {
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
    else if (treetype >= 2) {
        if (x < 2 || x >= MAP_WIDTH - 2 || y >= MAP_HEIGHT - 6) return;
        for (int i = 1; i <= 6; i++) worldMap[0][y + i][x] = WOOD; // Ствол 
        int leafBase = y + 4;
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
}


// Заполнение карты случайными плавными биомами
void generateWorld() {
    for (int x = 0; x < MAP_WIDTH; x++) {
        int currentGroundLevel = getSurfaceHeight(x);
        for (int y = 0; y < MAP_HEIGHT; y++) {
            // Передний слой [1] 
            if (y > currentGroundLevel) {
                worldMap[1][y][x] = AIR;
            }
            else if (y == currentGroundLevel) {
                worldMap[1][y][x] = GRASS;
            }
            else if (y < currentGroundLevel && y >= currentGroundLevel - 3) {
                worldMap[1][y][x] = DIRT;
            }
            else {
                worldMap[1][y][x] = STONE;
            }
            // Задний слой [0]
            if (y > currentGroundLevel) {
                worldMap[0][y][x] = AIR;
            }
            else if (y == currentGroundLevel) {
                worldMap[0][y][x] = GRASS;
            }
            else if (y < currentGroundLevel && y >= currentGroundLevel - 3) {
                worldMap[0][y][x] = DIRT;
            }
            else {
                worldMap[0][y][x] = STONE;
            }
        }
    }

    // Спавн деревьев
    for (int x = 5; x < MAP_WIDTH - 5; ) {
        int currentGroundLevel = getSurfaceHeight(x);
        addTree(x, currentGroundLevel);

        x += 8 + (rand() % 20);
    }

    // Спавн овечек
    for (int x = 5; x < MAP_WIDTH - 5; ) {
        int currentGroundLevel = getSurfaceHeight(x);
        Sheep newSheep((float)x, (float)currentGroundLevel);
        sheeps.push_back(newSheep);

        x += 8 + (rand() % 20);
    }
}