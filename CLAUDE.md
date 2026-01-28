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
    ├── lc_gpu.c     (GPU resource management - handle-based abstraction)
    └── libcad_internal.h (Shared internal types: lc_render_context_t)
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

**Brace style:** Allman style — opening curly braces go on their own new line, never on the same line as the statement. This applies to functions, structs, enums, if/else, for, while, switch, etc.

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

## Development Planning

**IMPORTANT: Before starting any work, read these two documents:**
- **`docs/action-plan.md`** - The master roadmap with phases, tasks, and acceptance criteria
- **`docs/progress.md`** - Tracks what has been completed, what is in progress, and what is next

Update `docs/progress.md` after completing significant work.

## Session Notes (2026-01-28)

Summary of work completed in the initial implementation session:

### What was built
- **`lc_gpu.c/h`** is now a real module (~535 lines) with handle-based GPU resource management (buffers, shaders, programs, VAOs, textures, FBOs). `lc_draw.c` and `lc_scene.c` have NOT yet been refactored to use it — they still call raw GL. That migration is next.
- **Render context pipeline**: `lc_render_context_t` (defined in `lib/libcad_internal.h`) flows from `libcad.c` -> `lc_scene_compute_context()` -> `lc_draw_render_ctx()` / `lc_canvas_render_ctx()`. The old `set_view_matrix()` functions still exist for backward compat but the new context path is wired up.
- **GL state stack**: `lc_gl_state_t` with `lc_gl_save_state()` / `lc_gl_restore_state()` in `lc_draw.c`. The GPU pick pass now properly saves/restores viewport, blend, depth, scissor, and FBO state.
- **Public API stubs**: `libcad.h` now has `cad_entity_t`, `cad_sketch_t`, `cad_body_t` handle types plus stub functions for sketch creation, entity metadata, selection, and undo/redo. All stubs are in `libcad.c` and print debug messages.
- **`lib/libcad_internal.h`**: shared internal header included by all modules.

### Pending refactors
- **Brace style cleanup**: All `.c` and `.h` files written in this session use K&R / same-line braces. These need to be refactored to Allman style (opening brace on its own line). Affected files: `lc_gpu.c`, `lc_gpu.h`, `lc_draw.c`, `lc_draw.h`, `lc_canvas.c`, `lc_canvas.h`, `lc_scene.c`, `lc_scene.h`, `libcad.c`, `libcad.h`, `libcad_internal.h`. Do this refactor at the start of the next coding session.

### What is NOT done yet
- **ImGui DrawList migration** (tasks 3.2-3.4): `lc_draw_line()`, `lc_draw_circle()`, `lc_draw_grid()`, etc. still call `ImDrawList_Add*` functions. These need to be rewritten to push `vector_instance_t` entries into the SDF instancing pipeline instead. This is the biggest remaining Phase 1 item.
- **lc_gpu adoption**: `lc_draw.c` and `lc_scene.c` still call raw `glGen*`/`glBind*`/`glDelete*`. Need to refactor them to use `lc_gpu_create_buffer()` etc.
- **Phase 2 entity system**: The public API stubs exist but `lc_entity.c`, `lc_document.c`, and `lc_undo.c` don't exist yet.
- **Canvas-as-plane concept** (task 2.4): Not documented yet.

### Key architectural details to remember
- `lc_draw.c` has TWO rendering paths: (1) instanced SDF via `vector_instance_t` array + shaders, and (2) ImGui `ig_drawlist` calls. Path 2 needs to be eliminated.
- The SDF pipeline supports types: circle, rounded rect, line segment, arc, triangle, polygon, ellipse, rectangle. Shaders are in `resources/shaders/vector/`.
- `lc_scene.c` owns the 3D camera and computes both the 3D view-projection and the 2D canvas transform. It calls `lc_draw_set_view_matrix()` and `lc_canvas_set_view_matrix()` to propagate these.
- `MAX_INSTANCES` is 64 in `lc_draw.c` — will need to grow for real usage.
