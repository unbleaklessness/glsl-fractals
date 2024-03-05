#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <iomanip>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <png.h>

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

void savePNG(const char *filePath, GLubyte *pixels, int width, int height) {
    FILE *file = fopen(filePath, "wb");
    if (!file) return;

    png_structp image = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!image) return;

    png_infop info = png_create_info_struct(image);
    if (!info) return;

    if (setjmp(png_jmpbuf(image))) return;

    png_init_io(image, file);

    // Output is 8-bit depth, RGB format.
    png_set_IHDR(
        image,
        info,
        width, height,
        8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    png_write_info(image, info);

    // Write image data.
    for (int y = 0; y < height; y++) {
        png_write_row(image, pixels + (y * width * 3));
    }

    // End write.
    png_write_end(image, nullptr);

    if (file != nullptr) fclose(file);
    if (info != nullptr) png_free_data(image, info, PNG_FREE_ALL, -1);
    if (image != nullptr) png_destroy_write_struct(&image, (png_infopp) nullptr);
}

std::string padNumberWithZeros(int number, int width) {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(width) << number;
    return oss.str();
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
    int aspectN = 200;
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

    std::string vertexShaderSource = readFile("vertex_shader.vert");
    std::string fragmentShaderSource = readFile("fragment_shader.frag");

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

    size_t frameNumber = 0;

    std::filesystem::path directoryPath = "images";
    std::filesystem::remove_all(directoryPath);
    try {
        if (std::filesystem::create_directory(directoryPath)) {
            std::cout << "Directory created successfully." << std::endl;
        } else {
            std::cout << "Directory already exists or an error occurred." << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "An error occurred: `" << e.what() << "`." << std::endl;
    }

    float deltaTime = 0.025f;
    float time = 0.f;

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
        glUniform1f(timeLocation, time);

        // Render the screen

        glUseProgram(shaderProgram);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Capture

        GLubyte* pixels = new GLubyte[3 * screenWidth * screenHeight]; // 3 channels (RGB)
        glReadPixels(0, 0, screenWidth, screenHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels);

        if (false) {
            std::string fileName = padNumberWithZeros(frameNumber, 5) + ".png";
            std::filesystem::path filePath = directoryPath / fileName;
            savePNG(filePath.c_str(), pixels, screenWidth, screenHeight);
            frameNumber++;
        }

        // Time

        time += deltaTime;

        if (time >= 12. * 2. * M_PI) {
            break;
        }

        // Swap buffers and poll events

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup

    glDeleteProgram(shaderProgram);
    glfwTerminate();

    return 0;
}
