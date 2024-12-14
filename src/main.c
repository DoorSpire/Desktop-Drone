#define STB_IMAGE_IMPLEMENTATION
#define GLFW_EXPOSE_NATIVE_WIN32

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <stb_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

int screenWidth, screenHeight;
GLuint VAO, VBO;

GLuint loadTexture(const char* filepath, int* width, int* height) {
    int nrChannels;
    unsigned char* data = stbi_load(filepath, width, height, &nrChannels, 0);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, *width, *height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    return texture;
}

void drawImage(float x, float y, const char* file, float screenWidth, float screenHeight) {
    int width, height;
    GLuint texture = loadTexture(file, &width, &height);

    if (texture == 0) return;

    float imageWidth = (float)width / screenWidth * 2.0f;
    float imageHeight = (float)height / screenHeight * 2.0f;

    GLfloat vertices[] = {
        x,               y,               0.0f, 1.0f,
        x + imageWidth,  y,               1.0f, 1.0f,
        x,               y + imageHeight, 0.0f, 0.0f,
        x + imageWidth,  y + imageHeight, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    const char* vertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec2 position;\n"
    "layout (location = 1) in vec2 texCoord;\n"
    "out vec2 TexCoord;\n"
    "void main() {\n"
    "    gl_Position = vec4(position, 0.0, 1.0);\n"
    "    TexCoord = texCoord;\n"
    "}\n";

    const char* fragmentShaderSource =
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "out vec4 color;\n"
    "uniform sampler2D texture1;\n"
    "void main() {\n"
    "    vec4 texColor = texture(texture1, TexCoord);\n"
    "    color = texColor;\n"
    "}\n";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteTextures(1, &texture);
}

int random(int min, int max) {
    return min + rand() % (max - min + 1);
}

int main() {
    srand((unsigned)time(NULL));
    int state = 0;
    int previousState = state;
    float x = 0.0f;
    float y = -1.0f;
    float velocity = 0.05f;

    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Desktop Drone", NULL, NULL);
    if (!window) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
    glfwSetWindowPos(window, (mode->width - 800) / 2, (mode->height - 600) / 2);
    glfwMaximizeWindow(window);
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    HWND hwnd = glfwGetWin32Window(window);
    LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);

    SetWindowLong(hwnd, GWL_EXSTYLE, style | WS_EX_LAYERED | WS_EX_TRANSPARENT);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    double lastTime = glfwGetTime();
    double animationTime = 0.0;
    double animationDuration = 0.3;
    int currentFrame = 1;
    double currentTime = 0.0;
    int minTimeInState = 2, maxTimeInState = 5;
    int timeInBetweenAnimations = random(minTimeInState, maxTimeInState);

    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        double deltaTime = now - lastTime;
        lastTime = now;

        animationTime += deltaTime;
        currentTime += deltaTime;

        if (animationTime >= animationDuration) {
            currentFrame = (currentFrame >= 4) ? 1 : currentFrame + 1;
            animationTime = 0.0;
        }

        if (currentTime >= timeInBetweenAnimations) {
            state = random(0, 5);
            currentTime = 0.0;

            if   (state == 3) timeInBetweenAnimations = random(minTimeInState * 2, maxTimeInState * 2);
            else              timeInBetweenAnimations = random(minTimeInState    , maxTimeInState    );

            if (state != previousState) {
                currentFrame = 1;
                previousState = state;
            }
        }

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        float imageWidth = 104.0f / (float)screenWidth * 2.0f;
        float imageHeight = 104.0f / (float)screenHeight * 2.0f;
        if (state == 0) {
            char path[50];
            sprintf(path, "gif/idle/idle%d.png", currentFrame);
            drawImage(x, y, path, (float)screenWidth, (float)screenHeight);
        } else if (state == 1) {
            if (x + imageWidth < 0.95f) {
                x += velocity * deltaTime;
            }
            char path[50];
            sprintf(path, "gif/walk/right/walk%d.png", currentFrame);
            drawImage(x, y, path, (float)screenWidth, (float)screenHeight);
        } else if (state == 2) {
            if (x > -0.95f) {
                x -= velocity * deltaTime;
            }
            char path[50];
            sprintf(path, "gif/walk/left/walk%d.png", currentFrame);
            drawImage(x, y, path, (float)screenWidth, (float)screenHeight);
        } else if (state == 3) {
            char path[50];
            sprintf(path, "gif/sleep/sleep%d.png", currentFrame);
            drawImage(x, y, path, (float)screenWidth, (float)screenHeight);
        } else if (state == 4) {
            if (y + imageHeight < 0.88f) {
                y += (velocity * 3) * deltaTime;
            }
            char path[50];
            sprintf(path, "gif/flying/idle%d.png", currentFrame);
            drawImage(x, y, path, (float)screenWidth, (float)screenHeight);
        } else if (state == 5) {
            if (y > -1.0f) {
                y -= (velocity * 3) * deltaTime;
            }
            char path[50];
            sprintf(path, "gif/flying/idle%d.png", currentFrame);
            drawImage(x, y, path, (float)screenWidth, (float)screenHeight);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glViewport(0, 0, screenWidth, screenHeight);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}