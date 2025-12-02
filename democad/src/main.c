#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_SDL3
#define CIMGUI_USE_OPENGL3
#include <cimgui/cimgui.h>
#include <cimgui/cimgui_impl.h>

static SDL_Window* window = NULL;
static SDL_GLContext gl_context = 0;

struct ImGuiContext* ctx = NULL;
struct ImGuiIO* io = NULL;

static bool quit = false;

void update(SDL_Event *event);
bool event_watch(void *data, SDL_Event *event);

int main() 
{

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

    window = SDL_CreateWindow(
        "democad", 
        1600, 
        900, 
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN
    );

    if (!window) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return false;
    }

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_MaximizeWindow(window);

    gl_context = SDL_GL_CreateContext(window);

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); /* Enable vsync */

    ctx = igCreateContext(NULL);
    io  = igGetIO_Nil();
    io->IniFilename = NULL;
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    const char* glsl_version = "#version 330 core";
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    igStyleColorsDark(NULL);

    ImGuiStyle *style = igGetStyle();
    style->TabRounding = 0.0f;
    style->WindowMenuButtonPosition = ImGuiDir_None;
    style->WindowBorderSize = 0.0f;

    SDL_ShowWindow(window);

    SDL_Event event;
    int counter = 0;

    SDL_AddEventWatch(event_watch, NULL);

    while( !quit )
    {
        while(SDL_PollEvent( &event) != 0)
        {
            update(&event);
            counter++;
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();

    igDestroyContext(ctx);

    return 0;
}

bool event_watch(void *data, SDL_Event *event)
{
    switch (event->type)
    {
        case SDL_EVENT_WINDOW_RESIZED:
            update(event);
            break;
    }
    //update(event);
    return true;
}

void update(SDL_Event* event)
{
    //User requests quit
    if(event->type == SDL_EVENT_QUIT)
    {
        quit = true;
    }

    ImGui_ImplSDL3_ProcessEvent(event);
    
    int windowWidth;
    int windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();

    igNewFrame();

    igText("Hello, world!", NULL);  

    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    igRender();
    ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());

        
    SDL_GL_SwapWindow(window);
}