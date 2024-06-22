#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <assert.h>
#include <string.h> // for memset

typedef unsigned int uint;
typedef struct vec3 { union { struct { float x,y,z; }; float e[3]; }; } vec3;
typedef struct vec4 { union { struct { float x,y,z,w; }; float e[4]; }; } vec4; // TODO use for vertex
typedef struct vertex_t {
    float x,y,z,w;
    float u,v;
} vertex_t;

#define T(name, def) typedef struct name def name;
#include "common.h"
#undef T

#define _STRINGIFY(...) #__VA_ARGS__ "\n"
#define S(...) _STRINGIFY(__VA_ARGS__)
#define T(name,def) "struct " #name " " #def ";\n"
#define SHADER_VERSION_STRING "#version 430 core\n"


char teapot_obj[] =
                   #include "teapot.obj.inc"
                   ;
#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"

primitive_t prim_buf[PRIMITIVE_COUNT];

#ifdef COMPILE_DLL
#if defined(_MSC_VER)
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT __attribute__((visibility("default")))
#endif


typedef struct state_t
{
    int initialized;

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

    /* movable camera */
    camera_t camera;
} state_t;

void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) { fprintf(stderr, "%s\n", message); }
void get_file_data(void* c, const char* f, int m, const char* o, char **buf, size_t *len) { *buf = teapot_obj; *len = sizeof(teapot_obj);}
EXPORT int on_load(state_t* state)
{
    tinyobj_attrib_t attrib;
    tinyobj_shape_t* shapes = NULL;
    size_t num_shapes;
    tinyobj_material_t* materials = NULL;
    size_t num_materials;
    unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;

    int ret = tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials,
                                &num_materials, "teapot.obj", get_file_data, NULL, flags);
    if (ret != TINYOBJ_SUCCESS) {
        printf("Failure\n");
        return 0;
    }
    printf("# of shapes    = %d\n", (int)num_shapes);
    printf("# of materials = %d\n", (int)num_materials);
    printf("# of vertices = %d\n", attrib.num_vertices);
    printf("# of faces    = %d\n", attrib.num_faces);


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
        const char* vs_source = SHADER_VERSION_STRING S(
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
        const char* fs_source = SHADER_VERSION_STRING S(
                                  in vec2 o_tex_coord;
                                  out vec4 color;
                                  uniform sampler2D u_texture;
                                  void main(void) {
                                  color = texture(u_texture, o_tex_coord);
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
            printf("%s\n", cs_source);
            printf("Compute shader compilation failed: %s\n", infoLog);
            return 0;
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
            return 0;
        }

        glUseProgram(*cs_program_id);

        assert(glGetError() == GL_NO_ERROR);
    }

    memset(prim_buf, 0, sizeof(prim_buf)); // NOTE needs zero initialization
    prim_buf[0].type = PRIMITIVE_TYPE_TRIANGLE;
    prim_buf[0].t = (triangle_t){{{{ 0.7, 0.2, -10}}}, 0,
                                   {{{ 0.5, 0.7, -10}}}, 0,
                                   {{{ 0.3, 0.2, -10}}}, 0,
                                   {{{0,1,0,1}}} };
    prim_buf[1].type = PRIMITIVE_TYPE_TRIANGLE;
    prim_buf[1].t = (triangle_t){{{{ 0.4, 0.5, -5}}}, 0,
                                   {{{ 0.3, 0.8, -5}}}, 0,
                                   {{{ 0.2, 0.4, -5}}}, 0,
                                   {{{1,0,0,1}}} };
    prim_buf[2].type = PRIMITIVE_TYPE_TRIANGLE;
    prim_buf[2].t = (triangle_t){{{{ 0.8, 0.5, -1}}}, 0,
                                   {{{ 0.7, 0.8, -1}}}, 0,
                                   {{{ 0.5, 0.4, -1}}}, 0,
                                   {{{0,0,1,1}}} };
    prim_buf[3].type = PRIMITIVE_TYPE_SPHERE;
    prim_buf[3].s = (sphere_t){{{{ 0.8, 0.5, -3}}}, 1.0, {{{1,1,0,1}}} };
    /* upload buffers to compute shader */
    {
        GLuint ssbo; // shader storage buffer object
        glGenBuffers(1, &ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(primitive_t) * PRIMITIVE_COUNT, prim_buf, GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
    }

    if (!state->initialized)
    {
        camera_t* camera = &state->camera;
        camera->dir.x = 0; camera->dir.y = 0; camera->dir.z =-1; camera->dir.w = 1;

        state->initialized = 1;
    }

    return 1;
}

/* helper */
vec4 vec4_add(const vec4 lhs, const vec4 rhs) { vec4 ret = {{{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w}}}; return ret; }
vec4 vec4_sub(const vec4 lhs, const vec4 rhs) { vec4 ret = {{{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w}}}; return ret; }
vec4 vec4_cross(const vec4 v1, vec4 v2) { vec4 ret = {{{v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x, 1.0f}}}; return ret; }

#include <math.h> // for fmod, sqrt, atan2, cos, sin, ...
EXPORT void update(state_t* state, char input, double delta_cursor_x, double delta_cursor_y)
{
    vec4* dir = &state->camera.dir;
    /* rotate_camera */
    {
        const float rot_speed = 0.005f; // NOTE hardcoded rotation speed

        /* rotate around Y axis */
        float yaw = atan2(dir->z, dir->x);
        yaw += delta_cursor_x * rot_speed;
        dir->x = cos(yaw);
        dir->z = sin(yaw);

        /* rotate around X axis */
        float pitch = atan2(dir->y, sqrt(dir->x * dir->x + dir->z * dir->z));
        pitch += delta_cursor_y * rot_speed;
        if (pitch > M_PI / 2)  { pitch =  M_PI / 2; }
        if (pitch < -M_PI / 2) { pitch = -M_PI / 2; }
        dir->x = cos(pitch) * cos(yaw);
        dir->y = sin(pitch);
        dir->z = cos(pitch) * sin(yaw);
    }

    switch(input)
    {
        case 'w': { state->camera.pos = vec4_add(state->camera.pos, *dir); } break;
        case 'a': { state->camera.pos = vec4_sub(state->camera.pos, vec4_cross(*dir, (vec4){{{0,1,0,1}}})); } break;
        case 's': { state->camera.pos = vec4_sub(state->camera.pos, *dir); } break;
        case 'd': { state->camera.pos = vec4_add(state->camera.pos, vec4_cross(*dir, (vec4){{{0,1,0,1}}})); } break;
        case 'q': { state->camera.pos.y += 0.5; } break;
        case 'e': { state->camera.pos.y -= 0.5; } break;
        default: {} break;
    }
}

EXPORT void draw(state_t* state)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glUseProgram(state->cs_program_id);

    /* upload uniforms */
    {
        camera_t* camera                = &state->camera;
        unsigned int* cs_program_id     = &state->cs_program_id;
        glUniform4f(glGetUniformLocation(*cs_program_id, "camera.pos"), camera->pos.x, camera->pos.y, camera->pos.z, camera->pos.w);
        glUniform4f(glGetUniformLocation(*cs_program_id, "camera.dir"), camera->dir.x, camera->dir.y, camera->dir.z, camera->dir.w);
    }

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
static int  (*on_load)(state_t*);
static void (*update)(state_t*, char, double, double);
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
    update       = dlsym(dll_handle, "update");
    draw         = dlsym(dll_handle, "draw");
    struct stat attr;
    stat(DLL_FILENAME, &attr);
    dll_last_mod = attr.st_mtime;
    #endif

    state_t* state = malloc(1024 * 1024);
    memset(state, 0, 1024 * 1024);
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
                update     = NULL;
                draw       = NULL;
            }
            dll_handle = dlopen(DLL_FILENAME, RTLD_NOW);
            if (dll_handle == NULL) { printf("Opening DLL failed. Trying again...\n"); }
            while (dll_handle == NULL) /* NOTE keep trying to load dll */
            {
                dll_handle = dlopen(DLL_FILENAME, RTLD_NOW);
            }
            on_load      = dlsym(dll_handle, "on_load");
            update       = dlsym(dll_handle, "update");
            draw         = dlsym(dll_handle, "draw");

            on_load(state);
            dll_last_mod = attr.st_mtime;
        }
        #endif

        static double time;
        float dt = glfwGetTime() - time;
        const double fps_cap = 1.f / 60.f;
        if (dt > fps_cap) {
            time = glfwGetTime();

            /* poll events */
            static double cursor_x = 0, cursor_y = 0;
            {
                char input = ' ';

                glfwPollEvents();

                /* key inputs */
                if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) { glfwSetWindowShouldClose(window, 1); }
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)      { input = 'w'; }
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)      { input = 'a'; }
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)      { input = 's'; }
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)      { input = 'd'; }
                if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)      { input = 'q'; }
                if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)      { input = 'e'; }

                /* cursor pos */
                double x,y;
                glfwGetCursorPos(window, &x, &y);
                double dx = x - cursor_x;
                double dy = y - cursor_y;
                cursor_x = x;
                cursor_y = y;

                update(state, input, dx, dy);
            }


            draw(state);
            glfwSwapBuffers(window);
        }
    }

    printf("Terminated\n");

    return 0;
};
#endif /* COMPILE_EXE */
