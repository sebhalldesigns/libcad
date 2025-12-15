#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_SDL3
#define CIMGUI_USE_OPENGL3
#include <cimgui/cimgui.h>
#include <cimgui/cimgui_impl.h>

#include <libcad/libcad.h>

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

    cad_init_viewport();

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

    //cad_create_context();

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
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            cad_set_cursor_button_state(event->button.button, true);
        } break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            cad_set_cursor_button_state(event->button.button, false);
        } break;

        case SDL_EVENT_MOUSE_MOTION:
        {
            cad_set_cursor_pos((int)event->motion.x, (int)event->motion.y);    
        } break;

        case SDL_EVENT_MOUSE_WHEEL:
        {
            cad_axis_delta(0, event->wheel.y);
        } break;

        

        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        case SDL_EVENT_MOUSE_REMOVED:
        {
            cad_cursor_lost();
        } break;
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

    ImVec2 menuSize = {0,0};

    if (igBeginMainMenuBar())
    {   

        menuSize = igGetWindowSize();

        if (igBeginMenu("File", true))
        {
            if (igMenuItem_Bool("Exit", NULL, false, true))
            {
                SDL_Event quitEvent;
                quitEvent.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quitEvent);
            }

            igEndMenu();
        }

        if (igBeginMenu("Model", true))
        {
            if (igMenuItem_Bool("New Sketch", NULL, false, true))
            {
                printf("New Sketch clicked\n");
            }


            igEndMenu();
        }

        if (igBeginMenu("Help", true))
        {
            if (igMenuItem_Bool("About", NULL, false, true))
            {
                /* do nothing */
            }

            igEndMenu();
        }

        igEndMainMenuBar();
    }


    ImGuiID dockspace_id = igGetID_Str("Dockspace");
    ImGuiViewport* viewport = igGetMainViewport();

    static bool dockspace_setup = false;
    static ImGuiID center_id = 0;
    static ImGuiID left_id = 0; 
    static ImGuiID right_id = 0; 


    if (!dockspace_setup)
    {
        igDockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        igDockBuilderSetNodeSize(dockspace_id, viewport->Size);
        
        left_id = igDockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.1f, NULL, &center_id);
        right_id = igDockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.11f, NULL, &center_id);
    
        igDockBuilderDockWindow("Explorer", left_id);
        igDockBuilderDockWindow("Help", left_id);
        igDockBuilderDockWindow("Properties", right_id);

        igDockBuilderFinish(dockspace_id);
        dockspace_setup = true;
    }

    igDockSpaceOverViewport(dockspace_id, viewport, ImGuiDockNodeFlags_PassthruCentralNode, NULL);
    
    igBegin("Explorer", NULL, 0);

        if (igTreeNodeEx_Str("Grid", ImGuiTreeNodeFlags_Leaf))
        {
            if (igIsItemClicked(0))
            {
                /* clicked */;
            }
            
            igTreePop();
        }
 
        if (igTreeNode_Str("Origin"))
        {
            if (igTreeNodeEx_Str("Point", ImGuiTreeNodeFlags_Leaf))
            {
                if (igIsItemClicked(0))
                {
                    /* clicked */;
                }
                igTreePop();
            }

            if (igTreeNodeEx_Str("X", ImGuiTreeNodeFlags_Leaf))
            {
                if (igIsItemClicked(0))
                {
                    /* clicked */;
                }
                igTreePop();
            }

            if (igTreeNodeEx_Str("Y", ImGuiTreeNodeFlags_Leaf))
            {
                if (igIsItemClicked(0))
                {
                    /* clicked */;
                }
                igTreePop();
            }

            if (igTreeNodeEx_Str("Z", ImGuiTreeNodeFlags_Leaf))
            {
                if (igIsItemClicked(0))
                {
                    /* clicked */;
                }
                igTreePop();
            }

            if (igTreeNodeEx_Str("XY", ImGuiTreeNodeFlags_Leaf))
            {
                if (igIsItemClicked(0))
                {
                    /* clicked */;
                }
                igTreePop();
            }

            if (igTreeNodeEx_Str("XZ", ImGuiTreeNodeFlags_Leaf))
            {
                if (igIsItemClicked(0))
                {
                    /* clicked */;
                }
                igTreePop();
            }

            if (igTreeNodeEx_Str("YZ", ImGuiTreeNodeFlags_Leaf))
            {
                if (igIsItemClicked(0))
                {
                    /* clicked */;
                }
                igTreePop();
            }

            igTreePop();
        }

    igEnd();    

    igBegin("Properties", NULL, 0);

    igText("Properties content");

    igEnd();

    igBegin("Help", NULL, 0);

    igText("Help content");

    igEnd();

    ImGuiDockNode *node = igDockBuilderGetNode(left_id);
    node->HasWindowMenuButton = false;

    ImGuiDockNode *right_node = igDockBuilderGetNode(right_id);

    igSetNextWindowPos((ImVec2){node->Size.x, node->Pos.y}, ImGuiCond_Always, (ImVec2){0.0f, 0.0f});

    igBegin("Test", NULL,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_AlwaysAutoResize
    );

    static int currentItem = 0;

    if (igCombo_Str("Selection Mode", &currentItem, "Auto\0Vertex\0Line\0Face\0Solid\0Part\0Assembly\0", -1))
    {
        /* do nothing */
    }

    if (igButton("None", (ImVec2){0,0}))
    {
        
    }

    igSameLine(0.0f, 10.0f);

    if (igButton("Line", (ImVec2){0,0}))
    {
        
    }

    igSameLine(0.0f, 10.0f);

    if (igButton("Rectangle", (ImVec2){0,0}))
    {
        
    }

    igSameLine(0.0f, 10.0f);

    if (igButton("Circle", (ImVec2){0,0}))
    {
    }

    igEnd();

    glViewport(0, 0, windowWidth, windowHeight);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



    int left = (int)(node->Pos.x + node->Size.x + viewport->Pos.x);
    int right = (int)(right_node->Pos.x + viewport->Pos.x);
    int top = (int)(node->Pos.y + viewport->Pos.y + menuSize.y);
    int bottom = (int)(node->Pos.y + node->Size.y + viewport->Pos.y + menuSize.y);
    
    int width = (int)(right - left);
    int height = (int)(bottom - top);

    cad_set_viewport((int)left, (int)(top - menuSize.y), width, height, windowWidth, windowHeight);
    cad_render_viewport();

    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, windowWidth, windowHeight);

    igRender();
    ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());

    
    SDL_GL_SwapWindow(window);
}