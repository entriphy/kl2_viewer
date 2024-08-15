#include <GL/gl.h>
#include <stdlib.h>
#include <EGL/egl.h>

EGLDisplay display = NULL;
EGLSurface surface = NULL;

void backend_renderInit(int width, int height) {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint major, minor;
    eglInitialize(display, &major, &minor);

    EGLint numConfigs;
    EGLConfig cfg;
    EGLint pbattr[] = {
        EGL_WIDTH, width,
        EGL_HEIGHT, height,
        EGL_NONE,
    };
    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_NONE
    };    

    eglChooseConfig(display, configAttribs, &cfg, 1, &numConfigs);
    surface = eglCreatePbufferSurface(display, cfg, pbattr);
    eglBindAPI(EGL_OPENGL_API);
    EGLContext eglCtx = eglCreateContext(display, cfg, EGL_NO_CONTEXT, NULL);    
    eglMakeCurrent(display, surface, surface, eglCtx);
}

void backend_renderClear() {
    eglTerminate(display);
}

void backend_renderFrame(void (*renderFunc)(), int width, int height, void *pixels) {
    (*renderFunc)();
    eglSwapBuffers(display, surface);
    glFlush();
    if (pixels != NULL) {
        glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixels);
    }
}
