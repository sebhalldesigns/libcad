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

void save_file_returned(void *userdata, const char * const *filelist, int filter);
void open_file_returned(void *userdata, const char * const *filelist, int filter);

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
    style->TabRounding = 2.0f;
    style->FrameRounding = 2.0f;
    style->GrabRounding = 2.0f;
    style->WindowMenuButtonPosition = ImGuiDir_None;
    style->WindowBorderSize = 1.0f;
    style->FrameBorderSize = 0.0f;
    style->PopupBorderSize = 1.0f;
    style->WindowPadding = (ImVec2){8.0f, 8.0f};
    style->FramePadding = (ImVec2){4.0f, 3.0f};
    style->ItemSpacing = (ImVec2){8.0f, 4.0f};

    /* darker color scheme */
    ImVec4* colors = style->Colors;
    colors[ImGuiCol_WindowBg] = (ImVec4){0.08f, 0.08f, 0.10f, 1.0f};
    colors[ImGuiCol_TitleBg] = (ImVec4){0.06f, 0.06f, 0.08f, 1.0f};
    colors[ImGuiCol_TitleBgActive] = (ImVec4){0.10f, 0.10f, 0.12f, 1.0f};
    colors[ImGuiCol_Tab] = (ImVec4){0.10f, 0.10f, 0.12f, 1.0f};
    colors[ImGuiCol_TabSelected] = (ImVec4){0.18f, 0.18f, 0.22f, 1.0f};
    colors[ImGuiCol_TabHovered] = (ImVec4){0.24f, 0.24f, 0.28f, 1.0f};
    colors[ImGuiCol_MenuBarBg] = (ImVec4){0.10f, 0.10f, 0.12f, 1.0f};
    colors[ImGuiCol_Header] = (ImVec4){0.18f, 0.18f, 0.22f, 1.0f};
    colors[ImGuiCol_HeaderHovered] = (ImVec4){0.28f, 0.28f, 0.32f, 1.0f};
    colors[ImGuiCol_HeaderActive] = (ImVec4){0.24f, 0.24f, 0.28f, 1.0f};
    colors[ImGuiCol_Button] = (ImVec4){0.18f, 0.18f, 0.22f, 1.0f};
    colors[ImGuiCol_ButtonHovered] = (ImVec4){0.28f, 0.28f, 0.32f, 1.0f};
    colors[ImGuiCol_ButtonActive] = (ImVec4){0.22f, 0.22f, 0.26f, 1.0f};
    colors[ImGuiCol_FrameBg] = (ImVec4){0.12f, 0.12f, 0.14f, 1.0f};
    colors[ImGuiCol_FrameBgHovered] = (ImVec4){0.18f, 0.18f, 0.20f, 1.0f};
    colors[ImGuiCol_FrameBgActive] = (ImVec4){0.16f, 0.16f, 0.18f, 1.0f};

    SDL_ShowWindow(window);

    SDL_Event event;
    int counter = 0;

    SDL_AddEventWatch(event_watch, NULL);

    cad_create_context();

    while (!quit)
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

        case SDL_EVENT_KEY_DOWN:
        {
            if (event->key.key == SDLK_LSHIFT || event->key.key == SDLK_RSHIFT)
            {
                cad_set_modifier_state(MODIFIER_SHIFT, true);
            }
            else if (event->key.key == SDLK_LCTRL || event->key.key == SDLK_RCTRL)
            {
                cad_set_modifier_state(MODIFIER_CONTROL, true);
            }
            else if (event->key.key == SDLK_LALT || event->key.key == SDLK_RALT)
            {
                cad_set_modifier_state(MODIFIER_ALT, true);
            }
        } break;

        case SDL_EVENT_KEY_UP:
        {
            if (event->key.key == SDLK_LSHIFT || event->key.key == SDLK_RSHIFT)
            {
                cad_set_modifier_state(MODIFIER_SHIFT, false);
            }
            else if (event->key.key == SDLK_LCTRL || event->key.key == SDLK_RCTRL)
            {
                cad_set_modifier_state(MODIFIER_CONTROL, false);
            }
            else if (event->key.key == SDLK_LALT || event->key.key == SDLK_RALT)
            {
                cad_set_modifier_state(MODIFIER_ALT, false);
            }
        } break;

        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        case SDL_EVENT_MOUSE_REMOVED:
        {
            cad_cursor_lost();
        } break;
    }
    return true;
}

void update(SDL_Event* event)
{
    /* user requests quit */
    if (event->type == SDL_EVENT_QUIT)
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

            SDL_DialogFileFilter filters[3];
            filters[0].name = "libcad models";
            filters[0].pattern = "lc";
            filters[1].name = "STL files";
            filters[1].pattern = "stl";
            filters[2].name = "Wavefront (OBJ) files";
            filters[2].pattern = "stl";

            if (igMenuItem_Bool("New", "Ctrl+N", false, true))
            {
                SDL_ShowSaveFileDialog(save_file_returned, NULL, window, filters, 1, "untitled.lc");
            }

            if (igMenuItem_Bool("Open", "Ctrl+O", false, true))
            {
                SDL_ShowOpenFileDialog(open_file_returned, NULL, window, filters, 1, NULL, false);
            }

            if (igMenuItem_Bool("Save", "Ctrl+S", false, true))
            {
               
            }

            igSeparator();

            if (igMenuItem_Bool("Exit", "Ctrl+Q", false, true))
            {
                SDL_Event quitEvent;
                quitEvent.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quitEvent);
            }

            igEndMenu();
        }

        if (igBeginMenu("Model", true))
        {
            if (igBeginMenu("New...", true))
            {
                if (igMenuItem_Bool("Sketch", NULL, false, true))
                {
                    printf("New Sketch clicked\n");
                }

                igSeparator();

                if (igMenuItem_Bool("Model Datum", NULL, false, true))
                {
                    
                }

                if (igMenuItem_Bool("Model Axis", NULL, false, true))
                {
                    
                }

                if (igMenuItem_Bool("Model Plane", NULL, false, true))
                {
                    
                }

                igSeparator();

                if (igMenuItem_Bool("Mesh", NULL, false, true))
                {
                    
                }



                igEndMenu();
            }


            


            igEndMenu();
        }

        if (igBeginMenu("View", true))
        {
            if (igBeginMenu("Projection", true))
            {

                if (igMenuItem_Bool("Orthographic", NULL, false, true))
                {
                    /* do nothing */
                }

                if (igMenuItem_Bool("Perspective", NULL, false, true))
                {
                    /* do nothing */
                }

                igEndMenu();
            }

            if (igMenuItem_Bool("Reset", NULL, false, true))
            {
                /* do nothing */
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
        
        left_id = igDockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.15f, NULL, &center_id);
        right_id = igDockBuilderSplitNode(center_id, ImGuiDir_Right, 0.18f, NULL, &center_id);
    
        igDockBuilderDockWindow("Explorer", left_id);
        igDockBuilderDockWindow("Terminal", left_id);
        igDockBuilderDockWindow("Help", right_id);
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

    igSeparatorText("Hover");

    const char* hovered_name = cad_get_hovered_name();
    int hovered_id = cad_get_hovered_id();

    if (hovered_id > 0)
    {
        igTextColored((ImVec4){0.4f, 0.8f, 1.0f, 1.0f}, "%s", hovered_name);
        igText("ID: %d", hovered_id);
    }
    else
    {
        igTextDisabled("(none)");
    }

    igSeparatorText("Transform");

    static float position[3] = {0.0f, 0.0f, 0.0f};
    static float rotation[3] = {0.0f, 0.0f, 0.0f};
    static float scale[3] = {1.0f, 1.0f, 1.0f};

    igDragFloat3("Position", position, 0.1f, -100.0f, 100.0f, "%.2f", 0);
    igDragFloat3("Rotation", rotation, 1.0f, -360.0f, 360.0f, "%.1f", 0);
    igDragFloat3("Scale", scale, 0.01f, 0.01f, 10.0f, "%.2f", 0);

    igSeparatorText("Material");

    static float color[4] = {0.5f, 0.5f, 0.5f, 1.0f};
    igColorEdit4("Color", color, 0);

    igEnd();

    igBegin("Help", NULL, 0);

    igSeparatorText("Navigation");
    igBulletText("Middle Mouse: Pan");
    igBulletText("Shift + Middle Mouse: Orbit");
    igBulletText("Scroll Wheel: Zoom");

    igSeparatorText("Selection");
    igBulletText("Left Click: Select");
    igBulletText("Escape: Deselect");

    igEnd();

    igBegin("Terminal", NULL, 0);

    igTextDisabled("libcad v0.1-dev");
    igSeparator();
    igTextWrapped("Ready.");

    igEnd();

    ImGuiDockNode *node = igDockBuilderGetNode(left_id);
    node->HasWindowMenuButton = false;

    ImGuiDockNode *right_node = igDockBuilderGetNode(right_id);

    int left = (int)(node->Pos.x + node->Size.x + viewport->Pos.x);
    int right = (int)(right_node->Pos.x + viewport->Pos.x);
    int top = (int)(node->Pos.y + viewport->Pos.y + menuSize.y);
    int bottom = (int)(node->Pos.y + node->Size.y + viewport->Pos.y + menuSize.y);
    
    int width = (int)(right - left);
    int height = (int)(bottom - top);


    glViewport(0, 0, windowWidth, windowHeight);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    cad_set_viewport((int)left, (int)(top - menuSize.y), width, height, windowWidth, windowHeight);
    cad_render_viewport();

    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, windowWidth, windowHeight);

    igRender();
    ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());

    
    SDL_GL_SwapWindow(window);
}

void save_file_returned(void *userdata, const char * const *filelist, int filter)
{
    printf("Save file returned\n");

    for (int i = 0; filelist[i] != NULL; i++)
    {
        printf("File: %s\n", filelist[i]);
    }
}

void open_file_returned(void *userdata, const char * const *filelist, int filter)
{
    printf("Open file returned\n");

    for (int i = 0; filelist[i] != NULL; i++)
    {
        printf("File: %s\n", filelist[i]);
    }
}