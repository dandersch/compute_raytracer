#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
int main()
{
    if (!glfwInit()) { printf("Failed to initalize glfw.\n"); }

    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 ); // compute shaders added in 4.3
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Window Title", NULL, NULL);
    if (!window) { printf("Failed to create GLFW window.\n"); glfwTerminate(); }

    glfwSetWindowTitle(window, "Window Title");
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // VSYNC = 1

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { printf("Failed to initialize glew.\n"); }

    const GLubyte* renderer = glGetString( GL_RENDERER );
    const GLubyte* version  = glGetString( GL_VERSION );
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version %s\n", version);




    return 0;
};
