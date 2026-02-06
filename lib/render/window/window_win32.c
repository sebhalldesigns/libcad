/***************************************************************
**
** libcad Source File
**
** File         :  window_win32.c
** Module       :  render/window
** Author       :  SH
** Created      :  2026-02-05 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad Window API
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "window.h"

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <windowsx.h>
#include <gl/gl.h>

#include <util/log/log.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/* See https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt for all values */
#define WGL_CONTEXT_MAJOR_VERSION_ARB             (0x2091U)
#define WGL_CONTEXT_MINOR_VERSION_ARB             (0x2092U)
#define WGL_CONTEXT_PROFILE_MASK_ARB              (0x9126U)
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          (0x00000001U)

/* See https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt for all values */
#define WGL_DRAW_TO_WINDOW_ARB                    (0x2001U)
#define WGL_ACCELERATION_ARB                      (0x2003U)
#define WGL_SUPPORT_OPENGL_ARB                    (0x2010U)
#define WGL_DOUBLE_BUFFER_ARB                     (0x2011U)
#define WGL_PIXEL_TYPE_ARB                        (0x2013U)
#define WGL_COLOR_BITS_ARB                        (0x2014U)
#define WGL_DEPTH_BITS_ARB                        (0x2022U)
#define WGL_STENCIL_BITS_ARB                      (0x2023U)
#define WGL_FULL_ACCELERATION_ARB                 (0x2027U)
#define WGL_TYPE_RGBA_ARB                         (0x202BU)

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef HGLRC WINAPI wgl_create_context_attribs_arb_t(HDC hdc, HGLRC hglrc,
        const int *attribs);

typedef BOOL WINAPI wgl_choose_pixel_format_arb_t(HDC hdc, const int *attribs,
        const FLOAT *attribs_float, UINT max_formts, int *formats, UINT *num_formats);

typedef struct
{
    HWND window;
    HDC gl_dc;
    HGLRC gl_rc;
    int width;
    int height;
} window_win32_t;

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static wgl_create_context_attribs_arb_t *wgl_create_context_attribs_arb;
static wgl_choose_pixel_format_arb_t *wgl_choose_pixel_format_arb;

static HINSTANCE instance_handle = NULL;
static WNDCLASSW window_class = {0};
static WNDCLASSW gl_window_class = {0};

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

window_t window_create(const char *title, int width, int height)
{   
    
    if (!instance_handle)
    {
        /* first call */
        instance_handle = GetModuleHandle(NULL);

        window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        window_class.lpfnWndProc = window_proc;
        window_class.hInstance = instance_handle; 
        window_class.hCursor = LoadCursorW(NULL, IDC_ARROW);
        window_class.hbrBackground = NULL;
        window_class.lpszClassName = L"libcad";

        if (!RegisterClassW(&window_class))
        {
            #ifdef DEBUG
                log_error("Failed to create Win32 window class");
            #endif
            return 0;
        }

        gl_window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        gl_window_class.lpfnWndProc = DefWindowProcW;
        gl_window_class.hInstance = instance_handle;
        gl_window_class.lpszClassName = L"libcad_gl";
        
        if (!RegisterClassW(&gl_window_class))
        {
            #ifdef DEBUG
                log_error("Failed to create Win32 window class");
            #endif
            return 0;
        }
    }

    window_win32_t *window_win32 = malloc(sizeof(window_win32_t));
    if (!window_win32)
    {
        #ifdef DEBUG
            log_error("Failed to allocate win32 window.");
        #endif
        return 0;
    }

    HWND window  = CreateWindowExW(
        0,
        window_class.lpszClassName,
        L"Window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        0,
        0,
        instance_handle,
        (void*)window_win32
    );

    if (!window)
    {
        #ifdef DEBUG
            log_error("Failed to create Win32 window");
        #endif
        free(window_win32);
        return 0;
    }

    HDC gl_dc = GetDC(window);

    /*
    ** The standard procedure for enabling a "modern" OpenGL context
    ** on Windows is to create a dummy window with a 
    */

    HWND temp_window = CreateWindowExW(
        0,
        gl_window_class.lpszClassName,
        L"OpenGL",
        0,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        0,
        0,
        instance_handle,
        0
    );

    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;

    HDC temp_dc = GetDC(temp_window);

    int pixel_format = ChoosePixelFormat(temp_dc, &pfd);
    if (!pixel_format) 
    {
        #ifdef DEBUG
            log_error("Failed to find a suitable pixel format.");
        #endif
        free(window_win32);
        return 0;
    }

    if (!SetPixelFormat(temp_dc, pixel_format, &pfd)) 
    {
        #ifdef DEBUG
            log_error("Failed to set the pixel format.");
        #endif
        free(window_win32);
        return 0;
    }

    HGLRC temp_context = wglCreateContext(temp_dc);
    if (!temp_context) 
    {
        #ifdef DEBUG
            log_error("Failed to create a dummy OpenGL rendering context.");
        #endif
        free(window_win32);
        return 0;
    }

    if (!wglMakeCurrent(temp_dc, temp_context)) 
    {
        #ifdef DEBUG
            log_error("Failed to activate dummy OpenGL rendering context.");
        #endif
        free(window_win32);
        return 0;
    }

    wgl_create_context_attribs_arb = 
        (wgl_create_context_attribs_arb_t*)wglGetProcAddress("wglCreateContextAttribsARB");

    wgl_choose_pixel_format_arb = 
        (wgl_choose_pixel_format_arb_t*)wglGetProcAddress("wglChoosePixelFormatARB");

    wglMakeCurrent(temp_dc, 0);
    wglDeleteContext(temp_context);
    ReleaseDC(temp_window, temp_dc);
    DestroyWindow(temp_window);

    int pixel_format_attribs[] = 
    {
        WGL_DRAW_TO_WINDOW_ARB,     GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB,     GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,      GL_TRUE,
        WGL_ACCELERATION_ARB,       WGL_FULL_ACCELERATION_ARB,
        WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB,         32,
        WGL_DEPTH_BITS_ARB,         24,
        WGL_STENCIL_BITS_ARB,       8,
        0
    };

    UINT num_formats;
    wgl_choose_pixel_format_arb(gl_dc, pixel_format_attribs, 0, 1, &pixel_format, &num_formats);
    if (!num_formats) 
    {
        #ifdef DEBUG
            log_error("Failed to set the OpenGL 3.3 pixel format.");
        #endif
        free(window_win32);
        return 0;
    }


    DescribePixelFormat(gl_dc, pixel_format, sizeof(pfd), &pfd);
    if (!SetPixelFormat(gl_dc, pixel_format, &pfd)) 
    {
        #ifdef DEBUG
            log_error("Failed to set the OpenGL 3.3 pixel format.");
        #endif
        free(window_win32);
        return 0;
    }

    int gl33_attribs[] = 
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0,
    };

    HGLRC gl33_context = wgl_create_context_attribs_arb(gl_dc, 0, gl33_attribs);
    if (!gl33_context) 
    {
        #ifdef DEBUG
            log_error("Failed to create OpenGL 3.3 context.");
        #endif
        free(window_win32);
        return 0;
    }

    if (!wglMakeCurrent(gl_dc, gl33_context)) 
    {
        #ifdef DEBUG
            log_error("Failed to activate OpenGL 3.3 rendering context.");
        #endif
        free(window_win32);
        return 0;
    }

  
    window_win32->gl_dc = gl_dc;
    window_win32->gl_rc = gl33_context;
    window_win32->width = width;
    window_win32->height = height;

    UpdateWindow(window);
    ShowWindow(window, SW_SHOW);

    return (uintptr_t)window_win32;
}

void window_destroy(window_t window)
{
    DestroyWindow(((window_win32_t*)window)->window);
    free((window_win32_t*)window);
}

bool window_update()
{
    static MSG msg;
    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            return false;
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Sleep(0);
    YieldProcessor();

    return true;
}

void window_get_size(window_t window, vec2 *size)
{
    (*size)[0] = (float)((window_win32_t*)window)->width;
    (*size)[1] = (float)((window_win32_t*)window)->height;
}

void window_swap_buffers(window_t window)
{
    SwapBuffers(((window_win32_t*)window)->gl_dc);
}


/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    window_win32_t *window_data = NULL;

    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT *create_struct = (CREATESTRUCT*)lparam;
        window_data = (window_win32_t*)create_struct->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window_data);
        if (window_data) window_data->window = hwnd;
    }
    else
    {
        window_data = (window_win32_t*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    switch (msg)
    {
        case WM_PAINT:
        {
            ValidateRect(hwnd, NULL);
            return 0;
        }

        case WM_SIZE:
        {
            UINT width = LOWORD(lparam);
            UINT height = HIWORD(lparam);

            if (window_data)
            {
                window_data->width = width;
                window_data->height = height;
            }

            // Trigger a repaint to update during resize
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        case WM_CLOSE:
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
            
        default:
        {
            return DefWindowProcW(hwnd, msg, wparam, lparam);
        } break;
    }
}
