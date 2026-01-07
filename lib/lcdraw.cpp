#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkSurface.h>
#include <skia/include/core/SkPaint.h>
#include <skia/include/core/SkColor.h>
#include <skia/include/core/SkColorSpace.h>
#include <skia/include/gpu/ganesh/GrDirectContext.h>
#include <skia/include/gpu/ganesh/GrBackendSurface.h>
#include <skia/include/gpu/ganesh/SkSurfaceGanesh.h>

#include <skia/include/gpu/ganesh/gl/GrGLDirectContext.h>
#include <skia/include/gpu/ganesh/gl/GrGLBackendSurface.h>
#include <skia/include/gpu/ganesh/gl/GrGLInterface.h>
#include <skia/include/gpu/ganesh/gl/GrGLAssembleInterface.h>

#include <skia/include/gpu/ganesh/gl/egl/GrGLMakeEGLInterface.h>

#define X(x) (x[0])
#define Y(y) (y[1])

#include <glad/glad.h>

#include <stdio.h>

#include "lcdraw.h"

static sk_sp<GrDirectContext> context = NULL;

static float zoom = 1.0f;

static float offsetX = 0.0f;
static float offsetY = 0.0f;
static float dragOffsetX = 0.0f;
static float dragOffsetY = 0.0f;

static int surfaceWidth = 0;
static int surfaceHeight = 0;
static sk_sp<SkSurface> surface = NULL;
static SkCanvas* canvas = NULL;

static vec2 origin = {0.0f, 0.0f};

static void draw_circle(vec2 center, float radius);
static void draw_grid(vec2 start, vec2 end, float spacing, SkColor color);

static void draw_rect(vec2 center, vec2 size);
static void draw_ellipse(vec2 center, vec2 size);

void make_context()
{
    sk_sp<const GrGLInterface> interface = GrGLMakeNativeInterface();

    if (!interface.get())
    {   
        #if defined(__ANDROID__) || defined(__linux__)
            interface = GrGLMakeEGLInterface();
        #endif
    }

    if (!interface.get())
    {
        return;
    }

    printf("interface %p\n", interface.get());

    context = GrDirectContexts::MakeGL(interface);
    printf("context %p\n", context.get());
}

void render(int x, int y, int vpw, int vph, int w, int h)
{

    if (context) {
        context->resetContext(); 
    }

    if (w != surfaceWidth || h != surfaceHeight || surface == NULL)
    {
        
    }

    GrGLFramebufferInfo fbInfo;
        fbInfo.fFBOID = 0;
        fbInfo.fFormat = (GrGLenum)GL_RGBA8;

        GrBackendRenderTarget renderTarget = GrBackendRenderTargets::MakeGL(w, h, 0, 8, fbInfo);

        surface = SkSurfaces::WrapBackendRenderTarget(context.get(), renderTarget, GrSurfaceOrigin::kBottomLeft_GrSurfaceOrigin, SkColorType::kRGBA_8888_SkColorType, SkColorSpace::MakeSRGB(), nullptr);
        surfaceWidth = w;
        surfaceHeight = h;
   

    canvas = surface->getCanvas();
    canvas->setMatrix(SkMatrix::I());
    canvas->translate(x, y);
    canvas->clipRect(SkRect::MakeWH(vpw, vph));


    origin[0] = (vpw / 2.0f) + offsetX + dragOffsetX;
    origin[1] = (vph / 2.0f) + offsetY + dragOffsetY;

    canvas->clear(SkColorSetARGB(255, 25, 38, 51));

    SkPaint paint;
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SkColorSetARGB(255, 255, 255, 255));
    paint.setStrokeWidth(1);

    canvas->drawRect(SkRect::MakeXYWH(10, 10, 10, 10), paint);

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1);
    


    /* DRAW MINOR GRID */

    /* minor grid spacing from 5 to 50 px*/

    const float major_minor_ratio = 5.0f;
    
    const float minor_min = 10.0f;
    const float minor_max = 50.0f;
    const float base_spacing = 25.0f;

    float minor_spacing = zoom * base_spacing;

    while (minor_spacing < minor_min)
    {
        minor_spacing *= major_minor_ratio;
    }

    while (minor_spacing > minor_max)
    {
        minor_spacing /= major_minor_ratio;
    }
    
    vec2 minor_start = {fmodf(origin[0], minor_spacing) - minor_spacing, fmodf(origin[1], minor_spacing) - minor_spacing};
    vec2 grid_end = {(float)vpw, (float)vph};

    draw_grid(minor_start, grid_end, minor_spacing, SkColorSetARGB(13, 255, 255, 255));

    /* DRAW MAJOR GRID */
    float major_spacing = minor_spacing * major_minor_ratio;
    vec2 major_start = {fmodf(origin[0], major_spacing) - major_spacing, fmodf(origin[1], major_spacing) - major_spacing};
    draw_grid(major_start, grid_end, major_spacing, SkColorSetARGB(25, 255, 255, 255));

    /* draw axes */
    paint.setColor(SkColorSetARGB(150, 255, 255, 255));
    canvas->drawLine(0, origin[1], w, origin[1], paint);
    canvas->drawLine(origin[0], 0, origin[0], h, paint);

    draw_circle(origin, 4.0f);

    vec2 rect_origin = {150.0f, 100.0f};
    vec2 rect_size = {200.0f, 100.0f};

    rect_origin[0] *= zoom;
    rect_origin[1] *= zoom;

    rect_origin[0] += origin[0];
    rect_origin[1] += origin[1];

    rect_size[0] *= zoom;
    rect_size[1] *= zoom;

    draw_rect(rect_origin, rect_size);

    vec2 oval_origin = {-150.0f, 100.0f};
    vec2 oval_size = {100.0f, 200.0f};

    oval_origin[0] *= zoom;
    oval_origin[1] *= zoom;

    oval_origin[0] += origin[0];
    oval_origin[1] += origin[1];

    oval_size[0] *= zoom;
    oval_size[1] *= zoom;

    draw_ellipse(oval_origin, oval_size);
  
    context->flushAndSubmit();
}

void set_zoom(float z)
{
    zoom = z;
}

void set_offset(vec2 vec)
{
    offsetX = vec[0];
    offsetY = vec[1];
}

static void draw_circle(vec2 center, float radius)
{
    static SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SkColorSetARGB(255, 200, 200, 200));

    //canvas->drawCircle(SkPoint::Make(center[0] - 0.5f, center[1] - 0.5f), radius, paint);
    canvas->drawCircle(SkPoint::Make(center[0] - 0.5f, center[1] - 0.5f), radius, paint);

}

static void draw_grid(vec2 start, vec2 end, float spacing, SkColor color)
{
    static SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(color);
    paint.setStrokeWidth(1);

    for (float x = start[0]; x <= end[0]; x += spacing)
    {
        canvas->drawLine(x, start[1], x, end[1], paint);
    }

    for (float y = start[1]; y <= end[1]; y += spacing)
    {
        canvas->drawLine(start[0], y, end[0], y, paint);
    }
}

static void draw_rect(vec2 center, vec2 size)
{
    static SkPaint paint;
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SkColorSetARGB(25, 255, 255, 255));

    canvas->drawRect(SkRect::MakeXYWH(center[0] - size[0] / 2.0f, center[1] - size[1] / 2.0f, size[0], size[1]), paint);

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(SkColorSetARGB(150, 255, 255, 255));
    paint.setStrokeWidth(2);

    canvas->drawRect(SkRect::MakeXYWH(center[0] - size[0] / 2.0f, center[1] - size[1] / 2.0f, size[0], size[1]), paint);

    draw_circle(center, 4.0f);

    vec2 corners[4];
    corners[0][0] = center[0] - size[0] / 2.0f;
    corners[0][1] = center[1] - size[1] / 2.0f;
    corners[1][0] = center[0] + size[0] / 2.0f;
    corners[1][1] = center[1] - size[1] / 2.0f;
    corners[2][0] = center[0] - size[0] / 2.0f;
    corners[2][1] = center[1] + size[1] / 2.0f;
    corners[3][0] = center[0] + size[0] / 2.0f;
    corners[3][1] = center[1] + size[1] / 2.0f;

    draw_circle(corners[0], 4.0f);
    draw_circle(corners[1], 4.0f);
    draw_circle(corners[2], 4.0f);
    draw_circle(corners[3], 4.0f);


}

static void draw_ellipse(vec2 center, vec2 size)
{
    static SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SkColorSetARGB(25, 255, 255, 255));

    canvas->drawOval(SkRect::MakeXYWH(center[0] - size[0] / 2.0f, center[1] - size[1] / 2.0f, size[0], size[1]), paint);

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(SkColorSetARGB(150, 255, 255, 255));
    paint.setStrokeWidth(2);

    canvas->drawOval(SkRect::MakeXYWH(center[0] - size[0] / 2.0f, center[1] - size[1] / 2.0f, size[0], size[1]), paint);
    
    draw_circle(center, 4.0f);

    vec2 corners[4];
    corners[0][0] = center[0];
    corners[0][1] = center[1] - size[1] / 2.0f;
    corners[1][0] = center[0] + size[0] / 2.0f;
    corners[1][1] = center[1];
    corners[2][0] = center[0];
    corners[2][1] = center[1] + size[1] / 2.0f;
    corners[3][0] = center[0] - size[0] / 2.0f;
    corners[3][1] = center[1];

    draw_circle(corners[0], 4.0f);
    draw_circle(corners[1], 4.0f);
    draw_circle(corners[2], 4.0f);
    draw_circle(corners[3], 4.0f);

}