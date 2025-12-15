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

static int surfaceWidth = 0;
static int surfaceHeight = 0;
static sk_sp<SkSurface> surface = NULL;

void make_context()
{
    sk_sp<const GrGLInterface> interface = GrGLMakeNativeInterface();

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
   

    SkCanvas* canvas = surface->getCanvas();
    canvas->setMatrix(SkMatrix::I());
    canvas->translate(x, y);
    canvas->clipRect(SkRect::MakeWH(vpw, vph));


    float originX = (vpw / 2.0f) + offsetX + dragOffsetX;
    float originY = (vph / 2.0f) + offsetY + dragOffsetY;

    canvas->clear(SkColorSetARGB(255, 25, 38, 51));

    SkPaint paint;
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SkColorSetARGB(255, 255, 255, 255));
    paint.setStrokeWidth(1);

    canvas->drawRect(SkRect::MakeXYWH(10, 10, 10, 10), paint);

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