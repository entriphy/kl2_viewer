#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include <cimgui_impl.h>
#include <stdio.h>
#include "take/object.h"

static GLFWwindow *window = NULL;
static bool leftClick = false;
static bool rightClick = false;

static void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        glm_translated_y(Scr.WvMtx, -1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        glm_translated_y(Scr.WvMtx, 1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        glm_translated_x(Scr.WvMtx, 1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        glm_translated_x(Scr.WvMtx, -1.0f);
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        glm_rotate_x(Scr.WvMtx, 0.01f, Scr.WvMtx);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        glm_rotate_x(Scr.WvMtx, -0.01f, Scr.WvMtx);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        glm_rotate_y(Scr.WvMtx, 0.01f, Scr.WvMtx);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        glm_rotate_y(Scr.WvMtx, -0.01f, Scr.WvMtx);
    }
}

static void scrollCallback(GLFWwindow *window, double xOffset, double yOffset) {
    if (igGetIO()->WantCaptureMouse) {
        return;
    }

    glm_translated_z(Scr.WvMtx, yOffset * 10);
}

static void cursorPositioncallback(GLFWwindow* window, double xpos, double ypos) {
    static double prevX = 0.0;
    static double prevY = 0.0;
    const static float rotationSensitivity = 0.01f;
    const static float translationSensitivity = 0.1f;

    if (igGetIO()->WantCaptureMouse) {
        return;
    }

    if (prevX == 0.0 && prevY == 0.0) {
        prevX = xpos;
        prevY = ypos;
        return;
    }

    if (leftClick) {
        glm_rotate_y(Scr.WvMtx, rotationSensitivity * (xpos - prevX), Scr.WvMtx);
        glm_rotate_x(Scr.WvMtx, rotationSensitivity * (ypos - prevY), Scr.WvMtx);
    } else if (rightClick) {
        glm_translated_x(Scr.WvMtx, translationSensitivity * (xpos - prevX));
        glm_translated_y(Scr.WvMtx, translationSensitivity * (prevY - ypos));
    }

    prevX = xpos;
    prevY = ypos;
}

static void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
    leftClick = button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS;
    rightClick = button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS;
}

static void framebufferSizecallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    glm_perspective(glm_rad(45.0f), (float)width / (float)height, 0.1f, 10000000.0f, Scr.VsMtx);
    glm_mat4_mul(Scr.VsMtx, Scr.WvMtx, Scr.WsMtx);
}

int backend_windowInit(int width, int height) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(width, height, "Klonoa 2 Renderer", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to create GLFW window\n");
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizecallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetCursorPosCallback(window, cursorPositioncallback);
    glfwSetMouseButtonCallback(window, mouseCallback);
    return 0;
}

int backend_igInit() {
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");
    return 0;
}

void backend_windowLoop(void (*renderFunc)(), void (*igFunc)()) {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        processInput(window);

        (*renderFunc)();

        // ImGui overlay
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        igNewFrame();
        {
            igBegin("Controls", NULL, 0);
            (*igFunc)();
            igEnd();
        }
        igRender();
        ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());

        glfwSwapBuffers(window);
    }
}

void backend_igClear() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}

void backend_windowClear() {
    glfwDestroyWindow(window);
    glfwTerminate();
}