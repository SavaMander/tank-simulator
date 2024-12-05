// Sava Janjic SV51/2021
#define _CRT_SECURE_NO_WARNINGS
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <vector>
#include <cmath>
#include <list>

const int SEGMENTS = 100;
const float PI = 3.14159265358979323846f;

unsigned int compileShader(GLenum type, const char* source);
unsigned int createShader(const char* vsSource, const char* fsSource);
static unsigned loadImageToTexture(const char* filePath);
unsigned int createRectangleVAO(float x, float y, float width, float height, glm::vec3 color, bool textured=false);
std::vector<float> generateCircleVertices(float screenHeight);

float previousTime = 0.0f;
int digitIndex1 = 6;
int digitIndex2 = 0;
int number = 61;

void updateDigits() {
    float currentTime = glfwGetTime();
    if (currentTime - previousTime >= 1.0f) {
        number = number - 1;
        digitIndex1 = number / 10;
        digitIndex2 = number % 10;
        previousTime = currentTime;
    }
}

const int screenWidth = 1920;
const int screenHeight = 1080;

const float lampWidth = 50.0f;
const float lampHeight = 50.0f;
const float barWidth = 20.0f;
const float barHeight = 60.0f;
const float barSpacing = 10.0f;
int ammoCount = 10;

const float voltMeterWidth = 200.0f;
const float voltMeterHeight = 30.0f;

struct Target {
    unsigned int vao;
    int yPos;
    int xPos;
    float size;
};

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Tank Simulator", glfwGetPrimaryMonitor(), nullptr);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create GLFW window" << std::endl;
        return -1;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    glfwSwapInterval(1);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    // Shader loading
    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);
    unsigned int basicShader = createShader("basic.vert", "basic.frag");
    unsigned int texShader = createShader("tex.vert", "tex.frag");
    unsigned int cursorShader = createShader("cursor.vert", "cursor.frag");
    glUseProgram(cursorShader);
    glUniformMatrix4fv(glGetUniformLocation(cursorShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUseProgram(0);
    glUseProgram(basicShader);
    glUniformMatrix4fv(glGetUniformLocation(basicShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1i(glGetUniformLocation(basicShader, "useTexture"), 0);

    //VAO for the tank interior
    unsigned int tankVAO = createRectangleVAO(0, 0, screenWidth, screenHeight, glm::vec3(0.0f, 1.0f, 0.0f), true);
    unsigned tankTexture = loadImageToTexture("res/tank.png");
    glBindTexture(GL_TEXTURE_2D, tankTexture); //Podesavamo teksturu
    glGenerateMipmap(GL_TEXTURE_2D); //Generisemo mipmape 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//S = U = X    GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);// T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(texShader);
    unsigned ourTexLoc = glGetUniformLocation(texShader, "ourTexture");
    glUniform1i(ourTexLoc, 0); // Indeks teksturne jedinice (sa koje teksture ce se citati boje)
    glUseProgram(0);

    //VAO for the panorama
    unsigned int panoramaVAO = createRectangleVAO(-3112, 0, 8144,screenHeight, glm::vec3(0.0f, 1.0f, 0.0f), true);
    unsigned panoramaTexture = loadImageToTexture("res/panorama.jpg");
    glBindTexture(GL_TEXTURE_2D, panoramaTexture); //Podesavamo teksturu
    glGenerateMipmap(GL_TEXTURE_2D); //Generisemo mipmape 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//S = U = X    GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);// T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);

    // VAOs for targets
    const int tankNum = 3;
    std::list<Target> targets;
    unsigned targetTexture = loadImageToTexture("res/target.png");
    glBindTexture(GL_TEXTURE_2D, targetTexture); //Podesavamo teksturu
    glGenerateMipmap(GL_TEXTURE_2D); //Generisemo mipmape 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//S = U = X    GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);// T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);
    int randPosX = 0;
    int randPosY = 0;
    srand(time(NULL));
    for (int i = 0; i < tankNum; i++) {
        randPosX = rand() % (2500 - (-2500) + 1) - 2500;
        randPosY = rand() % (screenHeight / 2 - 50 + 1);

        float size = 700.0f * (1.0f - (randPosY / (screenHeight / 2.0f+100.0f)));

        if (size < 50.0f) size = 50.0f;

        unsigned int vao = createRectangleVAO(randPosX, randPosY, size, size, glm::vec3(0.0f, 1.0f, 0.0f), true);
        targets.push_back({ vao, randPosY,randPosX,size });
    }
    
    targets.sort([](const Target& a, const Target& b) {
        return a.yPos > b.yPos;
        });

    // VAO for the dashboard
    unsigned int dashboardVAO = createRectangleVAO(0, 0, screenWidth, 400, glm::vec3(0.0f, 1.0f, 0.0f),true);
    unsigned dashboardTexture = loadImageToTexture("res/milit.jpg"); //Ucitavamo teksturu
    glBindTexture(GL_TEXTURE_2D, dashboardTexture); //Podesavamo teksturu
    glGenerateMipmap(GL_TEXTURE_2D); //Generisemo mipmape 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//S = U = X    GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);// T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);

    // VAO for the voltmeter
    unsigned int voltmeterVAO = createRectangleVAO(110, 90, 270, 230, glm::vec3(0.0f, 1.0f, 0.0f), true);
    unsigned voltmeterTexture = loadImageToTexture("res/voltmeter.png"); //Ucitavamo teksturu
    glBindTexture(GL_TEXTURE_2D, voltmeterTexture); //Podesavamo teksturu
    glGenerateMipmap(GL_TEXTURE_2D); //Generisemo mipmape 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//S = U = X    GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);// T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);

    // VAOs for ammo bars
    unsigned int ammoVAOs[10];
    float startX = screenWidth - 585;
    for (int i = 0; i < ammoCount; ++i) {
        float x = startX + i * (barWidth + barSpacing);
        ammoVAOs[i] = createRectangleVAO(x, 150.0f, barWidth, barHeight, glm::vec3(1.0f, 1.0f, 1.0f));
    }

    // VAO for ammo screen
    unsigned int ammoScreenVAO = createRectangleVAO(screenWidth-620, 100, 600, 217, glm::vec3(0.0f, 1.0f, 0.0f), true);
    unsigned ammoScreenTexture = loadImageToTexture("res/screen.jpg"); //Ucitavamo teksturu
    glBindTexture(GL_TEXTURE_2D, ammoScreenTexture); //Podesavamo teksturu
    glGenerateMipmap(GL_TEXTURE_2D); //Generisemo mipmape 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//S = U = X    GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);// T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);

    // VAO for lamp
    unsigned int lampicaVAO = createRectangleVAO(450, 120, 30, 180, glm::vec3(0.0f, 1.0f, 0.0f), true);
    unsigned lampicaTextureOff = loadImageToTexture("res/lampica1.jpg"); //Ucitavamo teksturu
    unsigned lampicaTextureOn = loadImageToTexture("res/lampica2.jpg"); //Ucitavamo teksturu
    glBindTexture(GL_TEXTURE_2D, lampicaTextureOff); //Podesavamo teksturu
    glGenerateMipmap(GL_TEXTURE_2D); //Generisemo mipmape 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//S = U = X    GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);// T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindTexture(GL_TEXTURE_2D, lampicaTextureOn); //Podesavamo teksturu
    glGenerateMipmap(GL_TEXTURE_2D); //Generisemo mipmape 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//S = U = X    GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);// T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);

    // VAO for the emblem
    unsigned int emblemVAO = createRectangleVAO(screenWidth/2-80, 120, 150, 205, glm::vec3(0.0f, 1.0f, 0.0f), true);
    unsigned emblemTexture = loadImageToTexture("res/grb.png"); //Ucitavamo teksturu
    glBindTexture(GL_TEXTURE_2D, emblemTexture); //Podesavamo teksturu
    glGenerateMipmap(GL_TEXTURE_2D); //Generisemo mipmape 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//S = U = X    GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);// T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);

    // VAO for fail
    unsigned int failVAO = createRectangleVAO(screenWidth / 2 - 250, screenHeight-350, 500, 500, glm::vec3(0.0f, 1.0f, 0.0f), true);
    unsigned failTexture = loadImageToTexture("res/fail.png"); //Ucitavamo teksturu
    glBindTexture(GL_TEXTURE_2D, failTexture); //Podesavamo teksturu
    glGenerateMipmap(GL_TEXTURE_2D); //Generisemo mipmape 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//S = U = X    GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);// T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);

    // VAO for success
    unsigned int successVAO = createRectangleVAO(screenWidth / 2 - 250, screenHeight - 350, 500, 500, glm::vec3(0.0f, 1.0f, 0.0f), true);
    unsigned successTexture = loadImageToTexture("res/success.png"); //Ucitavamo teksturu
    glBindTexture(GL_TEXTURE_2D, successTexture); //Podesavamo teksturu
    glGenerateMipmap(GL_TEXTURE_2D); //Generisemo mipmape 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//S = U = X    GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);// T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);

    // VAO for name
    unsigned int nameVAO = createRectangleVAO(0, screenHeight-300, 500, 500, glm::vec3(0.0f, 1.0f, 0.0f), true);
    unsigned nameTexture = loadImageToTexture("res/name.png"); //Ucitavamo teksturu
    glBindTexture(GL_TEXTURE_2D, nameTexture); //Podesavamo teksturu
    glGenerateMipmap(GL_TEXTURE_2D); //Generisemo mipmape 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//S = U = X    GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);// T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);

    // Circle
    unsigned int VAO, VBO;
    std::vector<float> circleVertices = generateCircleVertices(screenHeight);
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glm::mat4 circleModel = glm::mat4(1.0f);  // Initialize as an identity matrix
    circleModel = glm::translate(circleModel, glm::vec3(screenWidth/2, screenHeight/2, 0.0f));

    float centerX = 0.0f;
    float centerY = 0.0f;
    float size = 10.0f;

    std::vector<float> xVertices = {
        - size, - size, 0.0,1.0,0.0,
        size, size, 0.0,1.0,0.0,

         - size, size, 0.0,1.0,0.0,
        size, - size, 0.0,1.0,0.0
    };

    std::vector<float> plusVertices = {
    0.0f, - size, 1.0,1.0,0.0,
    0.0f, size, 1.0,1.0,0.0,

    - size, 0.0f, 1.0,1.0,0.0,
    size, 0.0f, 1.0,1.0,0.0
    };

    // VAO for X
    unsigned int xVAO, xVBO;
    glGenVertexArrays(1, &xVAO);
    glGenBuffers(1, &xVBO);
    glBindVertexArray(xVAO);
    glBindBuffer(GL_ARRAY_BUFFER, xVBO);
    glBufferData(GL_ARRAY_BUFFER, xVertices.size() * sizeof(float), xVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // VAO for plus
    unsigned int plusVAO, plusVBO;
    glGenVertexArrays(1, &plusVAO);
    glGenBuffers(1, &plusVBO);
    glBindVertexArray(plusVAO);
    glBindBuffer(GL_ARRAY_BUFFER, plusVBO);
    glBufferData(GL_ARRAY_BUFFER, plusVertices.size() * sizeof(float), plusVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // VAO for line
    float lineVertices[] = {
    screenWidth/2, screenHeight/2, 0.0f,1.0f,0.0f,
    screenWidth / 2+1000, screenHeight / 2, 0.0f, 1.0f,0.0f
    };
    unsigned int lineVAO, lineVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // VAO for the tank gun
    unsigned int gunVAO = createRectangleVAO(screenWidth/2-25, -60.0f, 50,310, glm::vec3(0.0f, 1.0f, 0.0f), true);

    glm::mat4 gunModel = glm::mat4(1.0f);
    // Settings
    static float needleAngle = 0.0f;
    const float needleWidth = 3.0f;
    const float needleHeight = 100.0f;
    unsigned int needleVAO = createRectangleVAO(248, 200, needleWidth, needleHeight, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(200 + needleWidth / 2, 200, 0.0f));
    model = glm::rotate(model, glm::radians(needleAngle), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(-(200 + needleWidth / 2), -200, 0.0f));
    float voltmeterMinAngle = 57.0f;
    float voltmeterMaxAngle = -60.0;
    auto startTime = std::chrono::high_resolution_clock::now();
    bool lamp = true;
    float bounceTime = 0.2f;
    float lastPressTime = - 1.0f;
    float finalNeedleAngle= voltmeterMinAngle;
    int counter = 0;
    float currentBTime = 0.0;
    auto currentTime = std::chrono::high_resolution_clock::now();
    float elapsedTime = std::chrono::duration<float>(currentTime - startTime).count();
    float oscillationAmplitude = 2.0f; 
    float oscillationFrequency = 3.0f; 
    float oscillationAngle;
    bool interiorView = true;
    glm::mat4 basicModel = glm::mat4(1.0f);
    glm::mat4 cupolaModel = glm::mat4(1.0f);
    float currentCupolaPos = -3112.0f;
    float maxCupolaPos = 0;
    float minCupolaPos = -(8144-screenWidth);
    float moveAmount = 0;
    bool end = false;
    unsigned int digitsTextures[10];
    for (int i = 0; i < 10; i++) {
        char filename[256];
        sprintf(filename, "res/digits/%d.png", i);
        digitsTextures[i] = loadImageToTexture(filename);
        glBindTexture(GL_TEXTURE_2D, digitsTextures[i]); //Podesavamo teksturu
        glGenerateMipmap(GL_TEXTURE_2D); //Generisemo mipmape 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//S = U = X    GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);// T = V = Y
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   //GL_NEAREST, GL_LINEAR
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    unsigned int digit1VAO = createRectangleVAO(screenWidth/2-55, screenHeight-120, 60,98, glm::vec3(0.0f, 1.0f, 0.0f), true);
    unsigned int digit2VAO = createRectangleVAO(screenWidth / 2, screenHeight - 120, 60, 98, glm::vec3(0.0f, 1.0f, 0.0f), true);

    double mouseX, mouseY;
    float dx, dy, distance;
    glm::mat4 xModel = circleModel;

    float lastShot = 1.0f;
    float currentShotTime;
    while (!glfwWindowShouldClose(window)) {
        if (number != 0) {
            updateDigits();
        }
        glClear(GL_COLOR_BUFFER_BIT);
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);
        currentBTime = (float)glfwGetTime();
        currentShotTime = (float)glfwGetTime();
        moveAmount = (counter+1) * 1.5f;
        if (currentBTime - lastShot > 7.5) {
            lamp = true;
        }
        if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS) {
            if (currentBTime - lastPressTime >= bounceTime && counter!=10) {
                needleAngle -= 11.0;
                counter++;
                lastPressTime = currentBTime;
            }
        }
        if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) {
            if (currentBTime - lastPressTime >= bounceTime && counter != 0) {
                needleAngle += 11.0;
                counter--;
                lastPressTime = currentBTime;
            }
        }

        if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
            if (currentBTime - lastPressTime >= bounceTime) {
                interiorView = false;
                lastPressTime = currentBTime;
            }
        }

        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
            if (currentBTime - lastPressTime >= bounceTime) {
                interiorView = true;
                lastPressTime = currentBTime;
            }
        }

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            if (currentCupolaPos+moveAmount<maxCupolaPos) {
                cupolaModel = glm::translate(cupolaModel, glm::vec3(moveAmount, 0.0f, 0.0f));
                currentCupolaPos += moveAmount;
            }
        }

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            if (currentCupolaPos - moveAmount > minCupolaPos) {
                cupolaModel = glm::translate(cupolaModel, glm::vec3(-moveAmount, 0.0f, 0.0f));
                currentCupolaPos -= moveAmount;
            }
        }

        currentTime = std::chrono::high_resolution_clock::now();
        elapsedTime = std::chrono::duration<float>(currentTime - startTime).count();
        if(counter == 0)
            oscillationFrequency = 0.0f;
        else
            oscillationFrequency = 3.0f;
        oscillationAngle = oscillationAmplitude * sin(oscillationFrequency * elapsedTime * glm::pi<float>());

        finalNeedleAngle = voltmeterMinAngle + needleAngle  + oscillationAngle;
        finalNeedleAngle = glm::clamp(finalNeedleAngle, voltmeterMaxAngle, voltmeterMinAngle);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(248.0f, 200.0f, 0.0f));
        model = glm::rotate(model, glm::radians(finalNeedleAngle), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-248.0f, -200.0f, 0.0f));
        glUseProgram(texShader);
        glUniformMatrix4fv(glGetUniformLocation(texShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        if (!interiorView) {
            // Render panorama
            glUniformMatrix4fv(glGetUniformLocation(texShader, "model"), 1, GL_FALSE, glm::value_ptr(cupolaModel));
            glUniform1f(glGetUniformLocation(texShader, "coef"), 1.0);
            glBindTexture(GL_TEXTURE_2D, panoramaTexture);
            glBindVertexArray(panoramaVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            // Render targets
            glBindTexture(GL_TEXTURE_2D, targetTexture);
            std::list<Target>::iterator iter = targets.begin();
            while (iter != targets.end()) {
                glBindVertexArray(iter -> vao);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                iter++;
            }
            glfwGetCursorPos(window, &mouseX, &mouseY);
            mouseY = screenHeight - mouseY - 1;
            dx = mouseX - screenWidth / 2;
            dy = mouseY - screenHeight / 2;
            distance = sqrt(dx * dx + dy * dy);
            static float lastValidMouseX = screenWidth / 2;
            static float lastValidMouseY = screenHeight / 2;
            if (distance <= screenHeight / 2) {
                lastValidMouseX = mouseX;
                lastValidMouseY = mouseY;
            }
            else {
                mouseX = lastValidMouseX;
                mouseY = lastValidMouseY;
            }
            glm::mat4 inverseCupolaModel = glm::inverse(cupolaModel);

            // Render circle
            glUseProgram(basicShader);
            glUniformMatrix4fv(glGetUniformLocation(basicShader, "model"), 1, GL_FALSE, glm::value_ptr(circleModel));
            glBindVertexArray(VAO);
            glDrawArrays(GL_LINE_LOOP, 0, SEGMENTS + 1);
            glBindVertexArray(0);

            // Render x crosshair
            glm::vec3 position = glm::vec3(xModel[3][0], xModel[3][1], xModel[3][2]);
            glm::vec3 direction = glm::vec3(mouseX, mouseY, 0.0f) - position;
            glm::vec3 movement = glm::normalize(direction) * 0.5f*float(counter+1);
            if (glm::length(direction) > 3.0f) {
                xModel = glm::translate(xModel, movement);
            }
            glUniformMatrix4fv(glGetUniformLocation(basicShader, "model"), 1, GL_FALSE, glm::value_ptr(xModel));
            glBindVertexArray(xVAO);
            glDrawArrays(GL_LINES, 0, 4);
            glBindVertexArray(0);


            // Render plus crosshair
            glUseProgram(cursorShader);
            position = glm::vec3(xModel[3][0], xModel[3][1], xModel[3][2]);
            glUniform2f(glGetUniformLocation(cursorShader, "cursorPos"), mouseX, mouseY);
            glBindVertexArray(plusVAO);
            glDrawArrays(GL_LINES, 0, 4);
            glBindVertexArray(0);
            glUniform2f(glGetUniformLocation(cursorShader, "cursorPos"), 0, 0);
            float lineVertices[] = {
            position.x, position.y, 0.0f, 1.0f, 0.0f,
            mouseX, mouseY, 0.0f, 1.0f, 0.0f
            };

            // Render line
            glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineVertices), lineVertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(lineVAO);
            glDrawArrays(GL_LINES, 0, 2);
            glBindVertexArray(0);
            position = glm::vec3(inverseCupolaModel * glm::vec4(position, 1.0f));
            currentShotTime = glfwGetTime();
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
                if (lamp && ammoCount>0) {
                    lamp = false;
                    auto iter = targets.begin();
                    while (iter != targets.end()) {
                        if (position.x > iter->xPos && position.x < iter->xPos + iter->size &&
                            position.y > iter->yPos && position.y < iter->yPos + iter->size) {
                            iter = targets.erase(iter);
                        }
                        else {
                            ++iter;
                        }
                    }
                    ammoCount--;
                    lastShot = currentShotTime;
                }
            }
            
            // Render tank gun
            glUseProgram(texShader);
            glBindTexture(GL_TEXTURE_2D, dashboardTexture);
            glm::vec3 posXa=glm::vec3(xModel[3][0], xModel[3][1], xModel[3][2]);
            float angle = atan2(posXa.y, posXa.x - screenWidth / 2.0f);
            gunModel = glm::mat4(1.0f);
            gunModel = glm::translate(gunModel, glm::vec3(screenWidth / 2.0f, 0.0f, 0.0f));
            gunModel = glm::rotate(gunModel,-1.571f+angle, glm::vec3(0.0f, 0.0f, 1.0f));
            gunModel = glm::translate(gunModel, glm::vec3(-screenWidth / 2.0f, 0.0f, 0.0f));
            glUniformMatrix4fv(glGetUniformLocation(texShader, "model"), 1, GL_FALSE, glm::value_ptr(gunModel));
            glBindVertexArray(gunVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        else {
            glUniformMatrix4fv(glGetUniformLocation(texShader, "model"), 1, GL_FALSE, glm::value_ptr(basicModel));
            //Render tank interior
            glBindTexture(GL_TEXTURE_2D, tankTexture);
            glBindVertexArray(tankVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            //Render dashboard
            glUniform1f(glGetUniformLocation(texShader, "coef"), 6.0);
            glBindTexture(GL_TEXTURE_2D, dashboardTexture);
            glBindVertexArray(dashboardVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            //Render voltmeter
            glUniform1f(glGetUniformLocation(texShader, "coef"), 1.0);
            glBindTexture(GL_TEXTURE_2D, voltmeterTexture);
            glBindVertexArray(voltmeterVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            //Render ammo screen
            glUniform1f(glGetUniformLocation(texShader, "coef"), 1.0);
            glBindTexture(GL_TEXTURE_2D, ammoScreenTexture);
            glBindVertexArray(ammoScreenVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            //Render emblem
            glUniform1f(glGetUniformLocation(texShader, "coef"), 1.0);
            glBindTexture(GL_TEXTURE_2D, emblemTexture);
            glBindVertexArray(emblemVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            //Render light
            glUniform1f(glGetUniformLocation(texShader, "coef"), 1.0);
            if (!lamp) {
                glBindTexture(GL_TEXTURE_2D, lampicaTextureOff);
            }
            else {
                glBindTexture(GL_TEXTURE_2D, lampicaTextureOn);
            }
            glBindVertexArray(lampicaVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            // Rendering objects with no textures
            glUseProgram(basicShader);
            glUniformMatrix4fv(glGetUniformLocation(basicShader, "model"), 1, GL_FALSE, glm::value_ptr(basicModel));

            // Render ammo bars
            for (int i = 0; i < ammoCount; ++i) {
                glBindVertexArray(ammoVAOs[i]);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            // Render needle
            glUniformMatrix4fv(glGetUniformLocation(basicShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(needleVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        }

        // Render digits
        glUseProgram(texShader);
        glUniformMatrix4fv(glGetUniformLocation(texShader, "model"), 1, GL_FALSE, glm::value_ptr(basicModel));
        glBindVertexArray(nameVAO);
        glBindTexture(GL_TEXTURE_2D, nameTexture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        if (digitIndex1 != 0 || digitIndex2 != 0) {
            if (targets.size() == 0) {
                glBindVertexArray(successVAO);
                glBindTexture(GL_TEXTURE_2D, successTexture);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                end = true;
            }
            else {
                glBindVertexArray(digit1VAO);
                glBindTexture(GL_TEXTURE_2D, digitsTextures[digitIndex1]);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                glBindVertexArray(digit2VAO);
                glBindTexture(GL_TEXTURE_2D, digitsTextures[digitIndex2]);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
        }
        else {
            if (targets.size() != 0) {
                glBindVertexArray(failVAO);
                glBindTexture(GL_TEXTURE_2D, failTexture);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
            else {
                end = true;
            }
        }
        if (end) {
            glfwSwapBuffers(window);
            glfwPollEvents();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
            glfwSwapBuffers(window);
            glfwPollEvents();
    }

    glDeleteVertexArrays(1, &tankVAO);
    glDeleteVertexArrays(1, &panoramaVAO);
    glDeleteVertexArrays(1, &dashboardVAO);
    glDeleteVertexArrays(1, &voltmeterVAO);
    glDeleteVertexArrays(1, &ammoScreenVAO);
    glDeleteVertexArrays(1, &lampicaVAO);
    glDeleteVertexArrays(1, &emblemVAO);
    glDeleteVertexArrays(1, &needleVAO);
    glDeleteVertexArrays(1, &failVAO);
    glDeleteVertexArrays(1, &successVAO);
    glDeleteVertexArrays(1, &nameVAO);
    for (int i = 0; i < 10; ++i) {
        glDeleteVertexArrays(1, &ammoVAOs[i]);
    }
    for (std::list<Target>::iterator i = targets.begin(); i != targets.end(); i++) {
        glDeleteVertexArrays(1, &i->vao);
    }

    glDeleteTextures(1, &tankTexture);
    glDeleteTextures(1, &panoramaTexture);
    glDeleteTextures(1, &dashboardTexture);
    glDeleteTextures(1, &voltmeterTexture);
    glDeleteTextures(1, &ammoScreenTexture);
    glDeleteTextures(1, &lampicaTextureOff);
    glDeleteTextures(1, &lampicaTextureOn);
    glDeleteTextures(1, &emblemTexture);
    glDeleteTextures(1, &targetTexture);
    glDeleteTextures(1, &failTexture);
    glDeleteTextures(1, &successTexture);
    glDeleteTextures(1, &nameTexture);
    glDeleteProgram(basicShader);
    glDeleteProgram(texShader);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

unsigned int compileShader(GLenum type, const char* source)
{
    //Uzima kod u fajlu na putanji "source", kompajlira ga i vraca sejder tipa "type"
    //Citanje izvornog koda iz fajla
    std::string content = "";
    std::ifstream file(source);
    std::stringstream ss;
    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
        std::cout << "Uspjesno procitao fajl sa putanje \"" << source << "\"!" << std::endl;
    }
    else {
        ss << "";
        std::cout << "Greska pri citanju fajla sa putanje \"" << source << "\"!" << std::endl;
    }
    std::string temp = ss.str();
    const char* sourceCode = temp.c_str(); //Izvorni kod sejdera koji citamo iz fajla na putanji "source"

    int shader = glCreateShader(type); //Napravimo prazan sejder odredjenog tipa (vertex ili fragment)

    int success; //Da li je kompajliranje bilo uspjesno (1 - da)
    char infoLog[512]; //Poruka o gresci (Objasnjava sta je puklo unutar sejdera)
    glShaderSource(shader, 1, &sourceCode, NULL); //Postavi izvorni kod sejdera
    glCompileShader(shader); //Kompajliraj sejder

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success); //Provjeri da li je sejder uspjesno kompajliran
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog); //Pribavi poruku o gresci
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" sejder ima gresku! Greska: \n");
        printf(infoLog);
    }
    return shader;
}

unsigned int createShader(const char* vsSource, const char* fsSource)
{
    //Pravi objedinjeni sejder program koji se sastoji od Vertex sejdera ciji je kod na putanji vsSource

    unsigned int program; //Objedinjeni sejder
    unsigned int vertexShader; //Verteks sejder (za prostorne podatke)
    unsigned int fragmentShader; //Fragment sejder (za boje, teksture itd)

    program = glCreateProgram(); //Napravi prazan objedinjeni sejder program

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource); //Napravi i kompajliraj vertex sejder
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource); //Napravi i kompajliraj fragment sejder

    //Zakaci verteks i fragment sejdere za objedinjeni program
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program); //Povezi ih u jedan objedinjeni sejder program
    glValidateProgram(program); //Izvrsi provjeru novopecenog programa

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success); //Slicno kao za sejdere
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(program, 512, NULL, infoLog);
        std::cout << "Objedinjeni sejder ima gresku! Greska: \n";
        std::cout << infoLog << std::endl;
    }

    //Posto su kodovi sejdera u objedinjenom sejderu, oni pojedinacni programi nam ne trebaju, pa ih brisemo zarad ustede na memoriji
    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}

static unsigned loadImageToTexture(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);
    if (ImageData != NULL)
    {
        //Slike se osnovno ucitavaju naopako pa se moraju ispraviti da budu uspravne
        stbi__vertical_flip(ImageData, TextureWidth, TextureHeight, TextureChannels);

        // Provjerava koji je format boja ucitane slike
        GLint InternalFormat = -1;
        switch (TextureChannels) {
        case 1: InternalFormat = GL_RED; break;
        case 2: InternalFormat = GL_RG; break;
        case 3: InternalFormat = GL_RGB; break;
        case 4: InternalFormat = GL_RGBA; break;
        default: InternalFormat = GL_RGB; break;
        }
        unsigned int Texture;
        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, TextureWidth, TextureHeight, 0, InternalFormat, GL_UNSIGNED_BYTE, ImageData);
        glBindTexture(GL_TEXTURE_2D, 0);
        // oslobadjanje memorije zauzete sa stbi_load posto vise nije potrebna
        stbi_image_free(ImageData);
        return Texture;
    }
    else
    {
        std::cout << "Textura nije ucitana! Putanja texture: " << filePath << std::endl;
        stbi_image_free(ImageData);
        return 0;
    }
}

unsigned int createRectangleVAO(float x, float y, float width, float height, glm::vec3 color, bool textured) {
    float vertices[28];
    if (textured) {
        float texturedVertices[] = {
            x, y,             color.r, color.g, color.b,  0.0f, 0.0f,
            x + width, y,     color.r, color.g, color.b,  1.0f, 0.0f,
            x + width, y + height, color.r, color.g, color.b,  1.0f, 1.0f,
            x, y + height,    color.r, color.g, color.b,  0.0f, 1.0f 
        };
        std::copy(std::begin(texturedVertices), std::end(texturedVertices), vertices);
    }
    else {
        float nonTexturedVertices[] = {
            x, y,             color.r, color.g, color.b,
            x + width, y,     color.r, color.g, color.b,
            x + width, y + height, color.r, color.g, color.b, 
            x, y + height,    color.r, color.g, color.b  
        };
        std::copy(std::begin(nonTexturedVertices), std::end(nonTexturedVertices), vertices);
    }

    unsigned int indices[] = {
        0, 1, 2, 
        0, 2, 3   
    };

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, textured ? sizeof(float) * 28 : sizeof(float) * 20, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (textured ? 7 : 5) * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (textured ? 7 : 5) * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    if (textured) {
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(5 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

std::vector<float> generateCircleVertices(float screenHeight) {
    std::vector<float> vertices;
    float radius = screenHeight / 2.0f;

    for (int i = 0; i <= SEGMENTS; ++i) {
        float angle = 2.0f * PI * i / SEGMENTS;
        float x = radius * cos(angle);
        float y = radius * sin(angle);

        vertices.push_back(x);
        vertices.push_back(y);

        vertices.push_back(0.0f);
        vertices.push_back(1.0f);
        vertices.push_back(0.0f); 
    }

    return vertices;
}