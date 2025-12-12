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

#include <glad/glad.h>

#include <stdio.h>

#include "lcdraw.h"

static sk_sp<GrDirectContext> context = NULL;

static float offsetX = 0.0f;
static float offsetY = 0.0f;
static float dragOffsetX = 0.0f;
static float dragOffsetY = 0.0f;

void make_context()
{
    sk_sp<const GrGLInterface> interface = GrGLMakeNativeInterface();

    printf("interface %p\n", interface);

    context = GrDirectContexts::MakeGL(interface);
    printf("context %p\n", context);
}

void render(int w, int h)
{
    GrGLFramebufferInfo fbInfo;
    fbInfo.fFBOID = 0;
    fbInfo.fFormat = (GrGLenum)GL_RGBA8;

    GrBackendRenderTarget renderTarget = GrBackendRenderTargets::MakeGL(w, h, 0, 8, fbInfo);

    sk_sp<SkSurface> surface = SkSurfaces::WrapBackendRenderTarget(context.get(), renderTarget, GrSurfaceOrigin::kBottomLeft_GrSurfaceOrigin, SkColorType::kRGBA_8888_SkColorType, SkColorSpace::MakeSRGB(), nullptr);

    SkCanvas* canvas = surface->getCanvas();

    float originX = (w / 2.0f) + offsetX + dragOffsetX;
    float originY = (h / 2.0f) + offsetY + dragOffsetY;

    canvas->clear(SkColorSetARGB(255, 25, 38, 51));

    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1);
    
    /* draw axes */
    paint.setColor(SkColorSetARGB(255, 150, 150, 150));
    canvas->drawLine(0, originY, w, originY, paint);
    canvas->drawLine(originX, 0, originX, h, paint);

    paint.setColor(SK_ColorRED);

    canvas->drawCircle(originX, originY, 10, paint);
    context->flushAndSubmit();
}

void set_offset(vec2 vec)
{

    printf("offset %.3f %.3f\n", offsetX, offsetY);
    offsetX = vec[0];
    offsetY = vec[1];
}