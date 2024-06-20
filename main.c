#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <assert.h>

#define WINDOW_TITLE "compute raytracer"
#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720

void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id,
                                  GLenum severity, GLsizei length,
                                  const GLchar* message, const void* userParam)
{
    fprintf(stderr, "%s\n", message);
}

int main()
{
    /* init glfw & glew */
    GLFWwindow* window;
    {
        if (!glfwInit()) { printf("Failed to initalize glfw.\n"); }

        glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
        glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 ); // compute shaders added in 4.3
        glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
        glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT,1);

        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
        if (!window) { printf("Failed to create GLFW window.\n"); glfwTerminate(); }

        glfwSetWindowTitle(window, WINDOW_TITLE);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // VSYNC = 1

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) { printf("Failed to initialize glew.\n"); }

        const GLubyte* renderer = glGetString( GL_RENDERER );
        const GLubyte* version  = glGetString( GL_VERSION );
        printf("Renderer: %s\n", renderer);
        printf("OpenGL version %s\n", version);
    }

    /* enable debugging abilities */
    {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_debug_callback, NULL);
    }









    return 0;
};
