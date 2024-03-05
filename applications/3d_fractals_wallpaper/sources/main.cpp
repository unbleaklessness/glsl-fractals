#include <chrono>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <iomanip>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <Imlib2.h>
#include <thread>

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
    // X11

    Imlib_Image image;
    Display *display;
    Pixmap pixelMap;
    Window rootWindow;
    Screen *screen;

    display = XOpenDisplay(NULL);
    if (!display) {
        std::cerr << "Could not open display!" << std::endl;
        return 0;
    }
    screen = DefaultScreenOfDisplay(display);
    rootWindow = RootWindow(display, DefaultScreen(display));
    XSync(display, False);

    // Initialize GLFW and create a window

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    int aspectW = 16;
    int aspectH = 9;
    int aspectN = 160;
    int screenWidth = aspectW * aspectN;
    int screenHeight = aspectH * aspectN;
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

    float deltaTime = 0.025f;
    float time = 0.f;

    GLubyte* pixels = new GLubyte[3 * screenWidth * screenHeight]; // 3 channels (RGB)
    unsigned int *ARGBData = (unsigned int *) malloc(screenWidth * screenHeight * sizeof(unsigned int));

    pixelMap = XCreatePixmap(
        display,
        rootWindow,
        screenWidth,
        screenHeight,
        DefaultDepthOfScreen(screen)
    );

    Atom propertyRoot, propertyESetRoot;
    propertyRoot = XInternAtom(display, "_XROOTPMAP_ID", False);
    propertyESetRoot = XInternAtom(display, "ESETROOT_PMAP_ID", False);

    if (propertyRoot == None || propertyESetRoot == None) {
        std::cerr << "Creation of pixmap property failed!" << std::endl;
        return 1;
    }

    // Rendering loop

    while (!glfwWindowShouldClose(window))
    {
        // Clear the screen

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

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

        // Wallpaper

        glReadPixels(0, 0, screenWidth, screenHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels);

        for (int y = 0; y < screenHeight; y++) {
            for (int x = 0; x < screenWidth; x++) {
                int i = (y * screenWidth + x) * 3; // Index in the RGB array
                int index = ((screenHeight - 1 - y) * screenWidth + x); // Flipping the image vertically
                ARGBData[index] = (0xFF << 24) | (pixels[i] << 16) | (pixels[i + 1] << 8) | pixels[i + 2];
            }
        }

        // Create an image from the ARGB data
        image = imlib_create_image_using_data(screenWidth, screenHeight, ARGBData);

        imlib_context_set_image(image);
        imlib_context_set_display(display);
        imlib_context_set_visual(DefaultVisualOfScreen(screen));
        imlib_context_set_colormap(DefaultColormapOfScreen(screen));
        imlib_context_set_drawable(pixelMap);

        XChangeProperty(
            display,
            rootWindow,
            propertyRoot,
            XA_PIXMAP,
            32,
            PropModeReplace,
            (unsigned char *) &pixelMap,
            1
        );
        XChangeProperty(
            display,
            rootWindow,
            propertyESetRoot,
            XA_PIXMAP,
            32,
            PropModeReplace,
            (unsigned char *) &pixelMap,
            1
        );

        imlib_render_image_on_drawable(0, 0);

#if 0 // Probabliy you don't need this chunk of code.
        
        XSetWindowBackgroundPixmap(display, rootWindow, pixelMap);
        XClearWindow(display, rootWindow);
        XFlush(display);
        XSetCloseDownMode(display, RetainPermanent);

        while (XPending(display)) {
            XEvent event;
            XNextEvent(display, &event);
        }
#endif

        imlib_free_image();

        // Time

        time += deltaTime;

        // Swap buffers and poll events

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup

    XFreePixmap(display, pixelMap);
    XCloseDisplay(display);

    free(pixels);
    free(ARGBData);

    glDeleteProgram(shaderProgram);
    glfwTerminate();

    return 0;
}
