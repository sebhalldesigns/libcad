# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

libcad is an open-source, cross-platform CAD library written in C99, designed like OpenGL/Vulkan as an embeddable library. The goal is a mixed 2D/3D CAD system with instanced GPU rendering for vector primitives.

## Build Commands

```bash
# Native build (Windows/macOS/Linux)
mkdir build && cd build
cmake ..
cmake --build .

# WebAssembly/Emscripten build
emcmake cmake ..
cmake --build .
```

Dependencies are git submodules in `extern/`. Clone with `--recursive` or run `git submodule update --init --recursive`.

Note: democad is outdated. A Python/PyQt test harness is planned as replacement.

## Architecture

```
libcad.h (Public API - cad_* functions)
    ↓
libcad.c (Core dispatcher)
    ├── lc_canvas.c  (2D canvas with pan/zoom, hit testing, modal tools)
    ├── lc_draw.c    (Drawing primitives, instanced vector rendering, ImGui integration)
    ├── lc_scene.c   (3D scene management)
    └── lc_gpu.c     (GPU resource management - stub)
```

**Key directories:**
- `include/libcad/` - Public API header
- `lib/` - Core library implementation
- `resources/shaders/` - GLSL shaders (embedded at compile time via `embed_resources()`)

**Rendering approach:** Moving from ImGui DrawList (inefficient for matrix transforms) to instanced OpenGL rendering with SDF-based vector primitives. This allows screen-space line thicknesses with 3D scene perspective.

## Platform Support

- **Desktop:** OpenGL 3.3 Core with GLAD loader
- **Web:** WebGL2/GLES3 via Emscripten
- Conditional compilation uses `#ifdef EMSCRIPTEN`, `WIN32`, `APPLE`

## Code Conventions

**Language:** C99 standard with `extern "C"` guards for C++ interop.

**File headers:** Use the standard block comment with File, Module, Author, Created, License, Description fields.

**Naming:**
- Public API: `cad_*` prefix
- Internal modules: `lc_*` prefix (e.g., `lc_canvas_init`, `lc_draw_line`)
- Types: snake_case with `_t` suffix (e.g., `lc_canvas_item_t`)
- All variables: snake_case

**File organization:** Sections marked with `/* MARK: */` in order: INCLUDES, CONSTANTS & MACROS, TYPEDEFS, STATIC VARIABLES, STATIC FUNCTION DEFS, PUBLIC FUNCTIONS, STATIC FUNCTIONS.

**Comments:** Always use `/* */` block comments, never `//` line comments.

## Key Dependencies

| Library | Purpose |
|---------|---------|
| cimgui | C bindings for ImGui |
| cglm | Math library (vec2, vec4, mat4) |
| jansson | JSON serialization |
| glad | OpenGL loader (desktop only) |

## Shader Embedding

Shaders in `resources/shaders/` are converted to C byte arrays at build time using the `embed_resources()` CMake function. Desktop uses `*_core_*.glsl`, Emscripten uses `*_es_*.glsl`.
