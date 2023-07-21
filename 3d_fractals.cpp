#include <iostream>
#include <fstream>
#include <sstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

std::string readFile(const std::string& filePath)
{
    std::ifstream fileStream(filePath);
    std::stringstream stringStream;
    stringStream << fileStream.rdbuf();
    return stringStream.str();
}

GLuint compileShader(GLenum shaderType, const std::string& shaderSource)
{
    GLuint shader = glCreateShader(shaderType);
    const char *source = shaderSource.c_str();
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation error: " << infoLog << std::endl;
    }

    return shader;
}

GLuint createShaderProgram(const std::string& vertexShaderSource, const std::string& fragmentShaderSource)
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking error: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

float quadVertices[] = {
        -1.0f,  1.0f,
        -1.0f, -1.0f,
        1.0f, -1.0f,

        -1.0f,  1.0f,
        1.0f, -1.0f,
        1.0f,  1.0f
};

double offsetX = 0.0f;
double offsetY = 0.0f;
double zoom = 1.0f;

void cursorPositionCallback(GLFWwindow* window, double xPosition, double yPosition)
{
    static double lastPositionX = xPosition;
    static double lastPositionY = yPosition;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        offsetX += (xPosition - lastPositionX) * zoom;
        offsetY += (yPosition - lastPositionY) * zoom;
    }

    lastPositionX = xPosition;
    lastPositionY = yPosition;
}

void scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    zoom -= yOffset * 0.01;
}

int main()
{
    // Initialize GLFW and create a window

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int aspectW = 16;
    int aspectH = 9;
    int aspectN = 100;
    GLFWwindow* window = glfwCreateWindow(aspectW * aspectN, aspectH * aspectN, "Fractals", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Load and compile shaders

    std::string vertexShaderSource = readFile("3d_vertex_shader.glsl");
    std::string fragmentShaderSource = readFile("3d_fragment_shader.glsl");

    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    GLuint quadVBO;
    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    GLuint quadVAO;
    glGenVertexArrays(1, &quadVAO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) nullptr);
    glEnableVertexAttribArray(0);

    glUseProgram(shaderProgram);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glfwSetScrollCallback(window, scrollCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);

    // Rendering loop

    while (!glfwWindowShouldClose(window))
    {
        // Clear the screen

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        int screenWidth, screenHeight;
        glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
        GLint screenSizeLocation = glGetUniformLocation(shaderProgram, "screenSize");
        glUseProgram(shaderProgram);
        glUniform2f(screenSizeLocation, (float) screenWidth, (float) screenHeight);

        GLint offsetLocation = glGetUniformLocation(shaderProgram, "offset");
        glUseProgram(shaderProgram);
        glUniform2f(offsetLocation, (float) offsetX, (float) offsetY);

        GLint zoomLocation = glGetUniformLocation(shaderProgram, "zoom");
        glUseProgram(shaderProgram);
        glUniform1f(zoomLocation, (float) zoom);

        GLint timeLocation = glGetUniformLocation(shaderProgram, "time");
        glUseProgram(shaderProgram);
        glUniform1f(timeLocation, (float) glfwGetTime());

        // Render the screen

        glUseProgram(shaderProgram);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Swap buffers and poll events

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup

    glDeleteProgram(shaderProgram);
    glfwTerminate();

    return 0;
}