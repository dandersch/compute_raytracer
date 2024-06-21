#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <assert.h>

#define WINDOW_TITLE "compute raytracer"
#define WINDOW_WIDTH  (1280/2)
#define WINDOW_HEIGHT (720/2)

#define SHADER_STRINGIFY(x) "#version 430 core\n" #x

#ifdef COMPILE_DLL
#if defined(_MSC_VER)
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT __attribute__((visibility("default")))
#endif

typedef struct vertex_t
{
    float x,y,z,w;
    float u,v;
} vertex_t;

void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) { fprintf(stderr, "%s\n", message); }

typedef struct state_t
{
    /* create texture */
    unsigned int texture_id;
    unsigned int texture_format;

    /* generate vao & vbo for texture */
    unsigned int texture_vbo;
    unsigned int texture_vao;

    /* create vertex shader */
    unsigned int vertex_shader_id;

    /* create fragment shader */
    unsigned int frag_shader_id;

    /* create shader */
    unsigned int shader_program_id;

    /* create compute shader & program */
    unsigned int compute_shader_id;
    unsigned int cs_program_id;
} state_t;

EXPORT void on_load(state_t* state)
{
    /* init glew */
    {
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
    unsigned int* texture_id = &state->texture_id;
    unsigned int* texture_format = &state->texture_format;
    *texture_format = GL_RGBA8;
    {
        assert(glGetError() == GL_NO_ERROR);

        //glEnable(GL_TEXTURE_2D); // NOTE: causes error...
        glGenTextures(1, texture_id);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, *texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, *texture_format, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        //glBindTexture(GL_TEXTURE_2D, 0);
    }

    /* generate vao & vbo for texture */
    unsigned int* texture_vbo = &state->texture_vbo;
    unsigned int* texture_vao = &state->texture_vao;
    vertex_t vertices[] = {{-1, -1, 0, 1,    0, 0}, { 1, -1, 0, 1,    1, 0},
                           {-1,  1, 0, 1,    0, 1}, { 1,  1, 0, 1,    1, 1}};
    {
        assert(glGetError() == GL_NO_ERROR);

        glGenVertexArrays(1, texture_vao);
        glBindVertexArray(*texture_vao);

        glGenBuffers(1, texture_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, *texture_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_t), 0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*) offsetof(vertex_t, u));
        glEnableVertexAttribArray(1);
    }

    /* create vertex shader */
    unsigned int* vertex_shader_id = &state->vertex_shader_id;
    {
        assert(glGetError() == GL_NO_ERROR);

        *vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
        const char* vs_source = SHADER_STRINGIFY(
            layout(location=0) in vec4 pos;
            layout(location=1) in vec2 tex_pos;
            out vec2 o_tex_coord;
            void main(void) {
            gl_Position = pos;
            o_tex_coord = tex_pos;
            });
        glShaderSource(*vertex_shader_id, 1, &vs_source, NULL);
        glCompileShader(*vertex_shader_id);

        /* print compile errors */
        int success;
        char infoLog[512];
        glGetShaderiv(*vertex_shader_id, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(*vertex_shader_id, 512, NULL, infoLog);
            printf("Vertex shader compilation failed: %s\n", infoLog);
        };
    }

    /* create fragment shader */
    unsigned int* frag_shader_id = &state->frag_shader_id;
    {
        assert(glGetError() == GL_NO_ERROR);

        *frag_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
        const char* fs_source = SHADER_STRINGIFY(
                                in vec2 o_tex_coord;
                                out vec4 color;
                                uniform sampler2D u_texture;
                                void main(void) {
                                color = texture(u_texture, o_tex_coord);
                                //color = vec4(1.0,0.0,0.0,1.0);
                                });
        glShaderSource(*frag_shader_id, 1, &fs_source, NULL);
        glCompileShader(*frag_shader_id);

        /* print any compile errors */
        int success;
        char infoLog[512];
        glGetShaderiv(*frag_shader_id, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(*frag_shader_id, 512, NULL, infoLog);
            printf("Fragment shader compilation failed: %s\n", infoLog);
        };
    }

    /* create shader */
    unsigned int* shader_program_id = &state->shader_program_id;
    {
        assert(glGetError() == GL_NO_ERROR);

        *shader_program_id = glCreateProgram();
        glAttachShader(*shader_program_id, *vertex_shader_id);
        glAttachShader(*shader_program_id, *frag_shader_id);
        glLinkProgram(*shader_program_id);

        /* print any compile errors */
        int success;
        char infoLog[512];
        glGetProgramiv(*shader_program_id, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(*shader_program_id, 512, NULL, infoLog);
            printf("Shader linking failed: %s\n", infoLog);
        }
    }

    /* create compute shader & program */
    unsigned int* compute_shader_id = &state->compute_shader_id;
    unsigned int* cs_program_id     = &state->cs_program_id;
    {
        assert(glGetError() == GL_NO_ERROR);

        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, state->texture_id);
        glBindImageTexture(0, state->texture_id, 0, GL_FALSE, 0, GL_WRITE_ONLY, state->texture_format);


        assert(glGetError() == GL_NO_ERROR);

        *compute_shader_id = glCreateShader(GL_COMPUTE_SHADER);
        const char* cs_source =
                                #include "compute.glsl"
                                ;
        glShaderSource(*compute_shader_id, 1, &cs_source, NULL);
        glCompileShader(*compute_shader_id);

        assert(glGetError() == GL_NO_ERROR);

        /* print any compile errors */
        int success;
        char infoLog[512];
        glGetShaderiv(*compute_shader_id, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(*compute_shader_id, 512, NULL, infoLog);
            printf("Compute shader compilation failed: %s\n", infoLog);
        };

        *cs_program_id = glCreateProgram();
        glAttachShader(*cs_program_id, *compute_shader_id);
        glLinkProgram(*cs_program_id);
        glDeleteShader(*compute_shader_id);

        /* print any linking errors */
        glGetProgramiv(*cs_program_id, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(*cs_program_id, 512, NULL, infoLog);
            printf("Shader linking failed: %s\n", infoLog);
        }

        glUseProgram(*cs_program_id);

        assert(glGetError() == GL_NO_ERROR);
    }

    typedef struct test_t
    {
        float a;
        float b;
        float c;
    } test_t;
    #define TEST_COUNT 2
    test_t test_buf[TEST_COUNT] = {{0.5,0,0}, {1,1,1}};
    /* upload buffers to compute shader */
    {
        GLuint ssbo; // shader storage buffer object
        glGenBuffers(1, &ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(test_t) * TEST_COUNT, test_buf, GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
    }
}

EXPORT void draw(state_t* state)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glUseProgram(state->cs_program_id);
    /* TODO upload uniforms */
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    #define WORK_GROUP_SIZE 1 // needs to match local_size_{x,y} in compute shader
    glDispatchCompute(WINDOW_WIDTH/WORK_GROUP_SIZE, WINDOW_HEIGHT/WORK_GROUP_SIZE, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    glUseProgram(state->shader_program_id);
    //glActiveTexture(GL_TEXTURE0 + 0);

    glBindBuffer(GL_ARRAY_BUFFER, state->texture_vbo);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
#endif /* COMPILE_DLL */


#ifdef COMPILE_EXE
#ifndef COMPILE_DLL
#include <dlfcn.h>
#include <sys/stat.h>
#define DLL_FILENAME "./code.dll"
static void*  dll_handle;
static time_t dll_last_mod;
typedef struct state_t state_t;
static void (*on_load)(state_t*);
static void (*draw)(state_t*);
#endif

#include <stdlib.h>
#include <stdio.h>
int main()
{
    /* init glfw */
    GLFWwindow* window;
    {
        if (!glfwInit()) { printf("Failed to initalize glfw.\n"); }

        glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
        glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 ); // compute shaders added in 4.3
        glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
        glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT,1);

        glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
        glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);

        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
        if (!window) { printf("Failed to create GLFW window.\n"); glfwTerminate(); }

        glfwSetWindowTitle(window, WINDOW_TITLE);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // VSYNC = 1
    }

    #ifndef COMPILE_DLL
    /* load in dll */
    dll_handle   = dlopen(DLL_FILENAME, RTLD_NOW);
    on_load      = dlsym(dll_handle, "on_load");
    draw         = dlsym(dll_handle, "draw");
    struct stat attr;
    stat(DLL_FILENAME, &attr);
    dll_last_mod = attr.st_mtime;
    #endif

    state_t* state = malloc(1024 * 1024);
    on_load(state);

    while (!glfwWindowShouldClose(window))
    {
        #ifndef COMPILE_DLL
        /* check if dll has changed on disk */
        if ((stat(DLL_FILENAME, &attr) == 0) && (dll_last_mod != attr.st_mtime))
        {
            printf("Attempting code hot reload...\n");

            if (dll_handle) /* unload dll */
            {
                dlclose(dll_handle);
                dll_handle = NULL;
                on_load    = NULL;
                draw       = NULL;
            }
            dll_handle = dlopen(DLL_FILENAME, RTLD_NOW);
            if (dll_handle == NULL) { printf("Opening DLL failed. Trying again...\n"); }
            while (dll_handle == NULL) /* NOTE keep trying to load dll */
            {
                dll_handle = dlopen(DLL_FILENAME, RTLD_NOW);
            }
            on_load      = dlsym(dll_handle, "on_load");
            draw         = dlsym(dll_handle, "draw");

            on_load(state);
            dll_last_mod = attr.st_mtime;
        }
        #endif

        /* poll events */
        {
            glfwPollEvents();
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) { glfwSetWindowShouldClose(window, 1); }
        }

        draw(state);

        glfwSwapBuffers(window);
    }

    printf("Terminated\n");

    return 0;
};
#endif /* COMPILE_EXE */
