#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Config.h"
#include "Graphics.h"


void updateCraftingRecipe();
bool tryMoveToInventory(InventorySlot item);
void glfwWindowSizeCallback(GLFWwindow* window, int width, int height);
void glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void handleSlotClick(InventorySlot& cell, int button, bool isResultSlot);
