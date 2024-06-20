#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <assert.h>

#define WINDOW_TITLE "compute raytracer"
#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720

typedef struct vertex_t
{
    float x,y,z,w;
    float u,v;
} vertex_t;

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

    /* create texture */
    unsigned int texture_id;
    unsigned int texture_format = GL_RGBA8;
    //unsigned char texture[WINDOW_WIDTH * WINDOW_HEIGHT * 4] = {255};
    {
        assert(glGetError() == GL_NO_ERROR);

        //glEnable(GL_TEXTURE_2D); // NOTE: causes error...
        glGenTextures(1, &texture_id);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, texture_format, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    /* generate vao & vbo for texture */
    unsigned int texture_vbo;
    unsigned int texture_vao;
    vertex_t vertices[] = {{-1, -1, 0, 1,    0, 0},
                           { 1, -1, 0, 1,    1, 0},
                           {-1,  1, 0, 1,    0, 1},
                           { 1,  1, 0, 1,    1, 1}};
    //vertex_t vertices[] = {{-1, -1, 0, 0,    0, 0},
    //                       { 1, -1, 1, 0,    1, 0},
    //                       { 1,  1, 1, 1,    0, 1},
    //                       {-1,  1, 0, 1,    1, 1}};
    {
        assert(glGetError() == GL_NO_ERROR);

        glGenVertexArrays(1, &texture_vao);
        glBindVertexArray(texture_vao);

        glGenBuffers(1, &texture_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, texture_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(vertices), 0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertices), (void*) offsetof(vertex_t, u));
        glEnableVertexAttribArray(1);
    }

    /* create vertex shader */
    unsigned int vertex_shader_id;
    {
        assert(glGetError() == GL_NO_ERROR);

        vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
        const char* vs_source = "#version 430 core\n"
            "layout(location=0) in vec4 pos;\n"
            "layout(location=1) in vec2 tex_pos;\n"
            "out vec2 o_tex_coord;\n"
            "void main(void) {\n"
            "gl_Position = pos;\n"
            "o_tex_coord = tex_pos;\n"
            "}";
        glShaderSource(vertex_shader_id, 1, &vs_source, NULL);
        glCompileShader(vertex_shader_id);

        /* print compile errors */
        int success;
        char infoLog[512];
        glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(vertex_shader_id, 512, NULL, infoLog);
            printf("Vertex shader compilation failed: %s\n", infoLog);
        };
    }

    /* create fragment shader */
    unsigned int frag_shader_id;
    {
        assert(glGetError() == GL_NO_ERROR);

        frag_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
        const char* fs_source = "#version 430 core\n"
                                "in vec2 o_tex_coord;\n"
                                "out vec4 color;\n"
                                "uniform sampler2D u_texture;\n"
                                "void main(void) {\n"
                                //"color = texture(u_texture, ex_TexCoor) + vec4(1.0,0.0,0.0,1.0);\n"
                                "color = vec4(1.0,0.0,1.0,1.0);\n"
                                "}";
        glShaderSource(frag_shader_id, 1, &fs_source, NULL);
        glCompileShader(frag_shader_id);

        /* print any compile errors */
        int success;
        char infoLog[512];
        glGetShaderiv(frag_shader_id, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(frag_shader_id, 512, NULL, infoLog);
            printf("Fragment shader compilation failed: %s\n", infoLog);
        };
    }

    /* create shader */
    unsigned int shader_program_id;
    {
        assert(glGetError() == GL_NO_ERROR);

        shader_program_id = glCreateProgram();
        glAttachShader(shader_program_id, vertex_shader_id);
        glAttachShader(shader_program_id, frag_shader_id);
        glLinkProgram(shader_program_id);

        /* print any compile errors */
        int success;
        char infoLog[512];
        glGetProgramiv(shader_program_id, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(shader_program_id, 512, NULL, infoLog);
            printf("Shader linking failed: %s\n", infoLog);
        }
    }

    assert(glGetError() == GL_NO_ERROR);


    return 0;
};
