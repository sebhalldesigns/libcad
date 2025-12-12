
#include <libcad/libcad.h>
#include <stdio.h>

#include <glad/glad.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <cglm/cglm.h>

#include <Windows.h>

#include "lcdraw.h"

static SDL_Window* window = NULL;
static SDL_GLContext gl_context = NULL;
static int width = 800;
static int height = 600;
static HWND hwnd_child = NULL;

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

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);  // Add explicit double buffering


    return (cad_ctx_t)1;
}

bool cad_create_child_window(void* parent_handle, int x, int y, int width, int height)
{

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
    HWND child = CreateWindowEx(
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

    //window = SDL_CreateWindow("Child Window", width, height, SDL_WINDOW_OPENGL);

    if (!window) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return false;
    }

    gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); /* Enable vsync */

    if (!gladLoadGL())
    {
        printf("Failed to initialize GLAD");
        return false;
    }
    
    make_context();

    printf("Created child window with handle: %p\n", child);

    ShowWindow(child, SW_SHOW);

    BOOL isVisible = IsWindowVisible(child);
    printf("Child window visible: %d\n", isVisible);

    hwnd_child = child;

    return true;
}

uintptr_t cad_get_child_window_handle()
{
    printf("Returning child window handle: %p\n", hwnd_child);
    return hwnd_child;
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

    render(width, height);

    SDL_GL_SwapWindow(window);
}

void cad_destroy_child_window()
{
 
}

void cad_set_window_size(int w, int h)
{
    width = w;
    height = h;
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

