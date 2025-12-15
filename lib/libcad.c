
#include <libcad/libcad.h>
#include <stdio.h>

#include <glad/glad.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <cglm/cglm.h>

#if WIN32
#include <Windows.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include "lcdraw.h"

static SDL_Window* window = NULL;
static SDL_GLContext gl_context = NULL;
static int width = 800;
static int height = 600;
static int vp_width = 800;
static int vp_height = 600;
static int x_pos = 100;
static int y_pos = 100;

#if WIN32
static HWND child = NULL;
#endif


static vec2 cursorPos = {0.0f, 0.0f};

static vec2 mouse3StartPos = {0.0f, 0.0f};
static bool mouse3Active = false;

static vec2 offset = {0.0f, 0.0f};


cad_ctx_t  cad_create_context()
{
    printf("cad_create_context called\n");
    
    SDL_SetAppMetadata("democad", "0.1", "org.libcad.demo");
    
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return -1;
    }

    #if WIN32
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);  // Add explicit double buffering
    #endif

    #if __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    
    #endif
  


    return (cad_ctx_t)1;
}

bool cad_create_child_window(void* parent_handle, int x, int y, int width, int height)
{   

    #if WIN32
    HWND parent = (HWND)parent_handle;
    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Register a basic window class if needed (for STATIC or custom)
    static bool registered = false;
    if (!registered) {
        WNDCLASS wc = {0};
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = "SDLChildClass";
        RegisterClass(&wc); 
        registered = true;
    }

    // Create child HWND
    child = CreateWindowEx(
        0,                          // ExStyle
        "SDLChildClass",            // Class name
        NULL,                       // Title (none)
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,  // Style: Child, visible, clips children
        0, 0,                       // Initial position
        1, 1,                       // Initial size (Avalonia resizes)
        parent,                     // Parent HWND
        NULL,                       // Menu
        hInstance,                  // Instance
        NULL                        // Param
    );

    if (!child) {
        printf("Failed to create child HWND: %lu\n", GetLastError());
        return NULL;
    }

    printf("Created child HWND: %p (parent: %p)\n", child, parent);

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 640);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 480);
    SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER, child);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);

    window = SDL_CreateWindowWithProperties(props);
    #endif
    
    #ifdef __EMSCRIPTEN__

    if (window)
    {
        cad_update_child_window();
        return false;
    }


    /* create glcanvas. N.B canvas2 is defined in the HTML file. */
    EM_ASM({
        /* * Variables 'width' and 'height' are available to EM_ASM 
            * from the C scope because it's a macro. 
            */
        var canvasElement = document.createElement('canvas');

        canvasElement.id = 'glcanvas';
        canvasElement.width = $0;
        canvasElement.height = $1;
        canvasElement.style.position = 'absolute';
        canvasElement.style.left = '0px';
        canvasElement.style.top = '0px';
        canvasElement.style.zIndex = '1000';
        canvasElement.style.pointerEvents = 'auto';
        canvasElement.style.backgroundColor = 'transparent'; 

        document.body.appendChild(canvasElement);

    }, width, height);

    printf("Created canvas element\n");

    
    printf("About to set up WebGL context\n");

    SDL_SetHint(SDL_HINT_EMSCRIPTEN_CANVAS_SELECTOR, "#glcanvas");

    SDL_PropertiesID props = SDL_CreateProperties();

    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 640);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 480);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);

    /* N.B # requested by docs */
    SDL_SetStringProperty(props,  SDL_PROP_WINDOW_CREATE_EMSCRIPTEN_CANVAS_ID_STRING, "glcanvas"); 

    printf("Creating SDL window with Emscripten canvas\n");
    
    window = SDL_CreateWindowWithProperties(props);
    printf("Created window!\n");


    #endif



    //window = SDL_CreateWindow("Child Window", width, height, SDL_WINDOW_OPENGL);

    if (!window) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return false;
    }

    gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); /* Enable vsync */

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        printf("Failed to initialize GLAD\n");
        return false;
    }
    
    make_context();



    return true;
}

uintptr_t cad_get_child_window_handle()
{
    #if WIN32
    printf("Returning child window handle: %p\n", child);
    return child;
    #else
    return 0;
    #endif
}

void cad_update_child_window()
{
    if (!window) return;

    SDL_Event event;

    while (SDL_PollEvent(&event)) 
    {

        switch (event.type) 
        {

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                cad_set_cursor_button_state(event.button.button, true);
            } break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                cad_set_cursor_button_state(event.button.button, false);
            } break;

            case SDL_EVENT_MOUSE_MOTION:
            {
                cad_set_cursor_pos((int)event.motion.x, (int)event.motion.y);    
            } break;

            case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            case SDL_EVENT_MOUSE_REMOVED:
            {
                cad_cursor_lost();
            } break;

        }
    }

    glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, width, height);

    render(x_pos, y_pos, vp_width, vp_height, width, height);

    SDL_GL_SwapWindow(window);
}

void cad_destroy_child_window()
{
 
}

void cad_set_window_size(int w, int h)
{
    width = w;
    height = h;

    #ifdef __EMSCRIPTEN__

    EM_ASM({
        var canvas = document.getElementById('glcanvas');
        if (canvas) {
            canvas.style.position = 'absolute';
            canvas.width = $0;
            canvas.height = $1;
            canvas.style.width = $0 + 'px';
            canvas.style.height = $1 + 'px';
            canvas.style.zIndex = '1000'; // Ensure it's on top
            canvas.style.pointerEvents = 'auto';
        }
    }, width, height);

    #endif
}

void cad_set_window_pos(int x, int y)
{
    #ifdef __EMSCRIPTEN__

    EM_ASM({
        var canvas = document.getElementById('glcanvas');
        if (canvas) {
            canvas.style.position = 'absolute';
            canvas.style.left = $0 + 'px';
            canvas.style.top = $1 + 'px';
                canvas.style.zIndex = '1000'; // Ensure it's on top
            canvas.style.pointerEvents = 'auto';
        }
    }, x, y);

    #endif
}

void cad_set_cursor_pos(int x, int y)
{
    //printf("cursor pos %d %d\n", x, y);
    cursorPos[0] = x;
    cursorPos[1] = y;

    if (mouse3Active)
    {
        vec2 delta;
        glm_vec2_sub(cursorPos, mouse3StartPos, delta);

        glm_vec2_add(offset, delta, offset);
        set_offset(offset);
        
        glm_vec2_copy(cursorPos, mouse3StartPos);
    }
}

void cad_cursor_lost()
{

}

void cad_set_cursor_button_state(int button, bool pressed)
{
    //printf("cursor state %d %d\n", button, pressed);

    switch (button)
    {
        case 2:
        {
            if (pressed)
            {
                glm_vec2_copy(cursorPos, mouse3StartPos);
                mouse3Active = true;
            }
            else
            {
                mouse3Active = false;

            }

        } break;

        default:
        {
            /* do nothing */
        } break;
    }
}

void cad_set_viewport(int x, int y, int vpw, int vph, int w, int h)
{
    vp_width = vpw;
    vp_height = vph;
    width = w;
    height = h;
    x_pos = x;
    y_pos = y;

    glEnable(GL_SCISSOR_TEST);
    glViewport(x, y, vp_width, vp_height);
    glScissor(x, y, vp_width, vp_height);
    glClearColor(1.0f, 0.15f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    printf("Set viewport %d %d %d %d %d %d\n", x, y, vpw, vph, w, h);
}

void cad_render_viewport()
{
    render(x_pos, y_pos, vp_width, vp_height, width, height);
}

void cad_init_viewport()
{

    if (!gladLoadGL() && !gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        printf("Failed to initialize GLAD\n");
        return false;
    }
    
    make_context();


}

#if __EMSCRIPTEN__
int main() { }
#endif