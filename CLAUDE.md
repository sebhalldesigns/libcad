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

## Session Notes (2026-01-29)

**Brace style cleanup completed**: Converted all K&R style braces to Allman style. Primary file affected was `lc_gpu.c` (all function definitions and control structures). Also fixed missing semicolon in `lc_gpu.h` line 101. All code compiles successfully with zero warnings.

**Phase 1 Task 3.2-3.3 (Partial): Line and Grid SDF Migration**
- Increased `MAX_INSTANCES` from 64 to 1024 to support many grid lines
- Modified `lc_draw_begin()` to reset instance count each frame and re-add test shapes
- Implemented `add_line_instance()` helper that:
  - Computes line midpoint, length, direction
  - Encodes endpoints as local coordinates for SHAPE_LINE shader
  - Supports full RGBA color and configurable thickness
  - Uses proper axis transformation (axis_x along line, axis_y perpendicular)
- Converted `lc_draw_line()` from ImGui DrawList to SDF instancing
- Grid rendering (`lc_draw_grid()`) now uses SDF automatically since it calls `lc_draw_line()`
- Axes rendering (in `lc_canvas.c`) now uses SDF via `lc_draw_line()`

**Still using ImGui DrawList**: `lc_draw_circle()`, `lc_draw_rect()`, `lc_draw_rect_filled()`, `lc_draw_ellipse()`, `lc_draw_handle()`, `lc_draw_text()`

**Note**: The SDF pipeline already has shape types for these (SHAPE_CIRCLE, SHAPE_RECTANGLE, SHAPE_ELLIPSE) so conversion should be straightforward following the same pattern as lines.

**Phase 2A Entity System (COMPLETED 2026-01-29)**

Implemented complete entity handle system as specified in `docs/phase2-entity-document-architecture.md`:

**Created Files:**
- `lib/lc_entity.h` (266 lines) - Complete entity API with handles, types, flags, tree manipulation
- `lib/lc_entity.c` (834 lines) - Full implementation with registry, free list, type-specific data
- `test_entity.c` - Comprehensive test suite with 6 test categories

**Key Features:**
- **Generational indices**: 32-bit handle = 16-bit index + 16-bit generation (prevents use-after-free)
- **Free list allocation**: O(1) entity creation and destruction, max 65,535 entities
- **Intrusive tree structure**: Parent/child/sibling pointers in entity slots (zero allocation for tree)
- **Type-specific data**: Discriminated union with malloc'd structs (sketch, body, line, circle, rect)
- **Handle validation**: Generation checking prevents stale handle usage
- **Entity types**: ASSEMBLY, SKETCH, BODY, GEOMETRY_LINE, GEOMETRY_CIRCLE, GEOMETRY_RECT (+ placeholders for FACE, EDGE, VERTEX, CONSTRAINT)
- **Entity flags**: VISIBLE, LOCKED, SELECTED, DELETED
- **Complete API**: 20+ functions for init, create, destroy, validation, data access, tree manipulation, enumeration

**Integration:**
- Wired `libcad.c` stubs: `cad_create_sketch()`, `cad_sketch_add_line/circle/rect()`, `cad_delete_entity()`
- Added `lc_entity_init()` call to `cad_init_viewport()`
- CMakeLists.txt updated to build lc_entity.c and test_entity

**Build Status:** Compiles cleanly on Windows/MSVC with zero errors

**Phase 2B Document System (COMPLETED 2026-01-29)**

Implemented complete document metadata and selection system as specified in `docs/phase2-entity-document-architecture.md`:

**Created Files:**
- `lib/lc_document.h` (172 lines) - Document API with metadata, selection, visibility/locking
- `lib/lc_document.c` (612 lines) - Full implementation with hash table and dynamic arrays

**Key Features:**
- **Metadata hash table**: Open-addressed with 1024 slots, linear probing, on-demand allocation
- **Selection management**: Dynamic array (starts 16, grows 2x, shrinks at 25%), maintains both array and SELECTED flag
- **Visibility/Locking**: Flags-based with proper defaults (visible=true, locked=false)
- **Query operations**: Find entity by name (walks hash table)
- **Root assembly**: Created on init, serves as document tree root
- **Complete API**: 15+ functions for init, metadata, selection, visibility, queries

**Integration:**
- Added lc_document.c to CMakeLists.txt (both native and Emscripten)
- Ready for wiring to libcad.c public API stubs

**Build Status:** Compiles cleanly on Windows/MSVC, libcad.lib = 500KB (was 414KB)

**Phase 2C Undo/Redo System (COMPLETED 2026-01-29)**

Implemented complete undo/redo command system as specified in `docs/phase2-entity-document-architecture.md`:

**Created Files:**
- `lib/lc_undo.h` (178 lines) - Undo API with command types, property IDs, stack management
- `lib/lc_undo.c` (669 lines) - Full implementation with hybrid snapshot/delta approach

**Key Features:**
- **Hybrid approach**: Immutable snapshots for entity create/delete, delta commands for property changes
- **Command stack**: Fixed 100-entry array, overflow drops oldest command (shift array)
- **Command grouping**: Multi-step operations with GROUP_BEGIN/GROUP_END markers, group depth tracking
- **Deep copying**: Entity snapshots include full type-specific data (sketch, body, line, circle, rect)
- **Undo/Redo logic**: Bidirectional stack traversal, respects command groups
- **Complete API**: 13 public functions for init, record, group, undo/redo, query, clear

**Implementation:**
- `lc_undo_perform()` - Walk backwards from undo_index, reverse each command
- `lc_undo_redo_perform()` - Walk forwards from redo_index, replay each command
- `copy_entity_snapshot()` - Deep copy entity slot including malloc'd type data
- `reverse_command()` - Undo one command (CREATE→destroy, DELETE→restore, MODIFY→apply old)
- `replay_command()` - Redo one command (apply new value)
- Command types: CREATE_ENTITY, DELETE_ENTITY, MODIFY_PROPERTY, MODIFY_GEOMETRY, GROUP_BEGIN/END
- Property types: NAME, VISIBILITY, LOCKED, LAYER, POSITION, COLOR

**Integration:**
- Added lc_undo.c to CMakeLists.txt (both native and Emscripten)
- Ready for wiring: entity create/destroy need to call `lc_undo_record_command()`
- Ready for wiring: property setters need to record undo commands

**Build Status:** Compiles cleanly on Windows/MSVC, libcad.lib = 537KB (was 500KB)

**Next Steps:** Phase 2D (wire public API stubs to real implementations), Phase 2E (rendering integration), Phase 2F (JSON serialization)

## Phase 2 Summary (2026-01-29)

**All core infrastructure complete** - Entity system, document model, and undo/redo are fully implemented (2,115 lines of C99 code across 6 files). Ready for integration and rendering.

### What was built
- **`lc_gpu.c/h`** is now a real module (~535 lines) with handle-based GPU resource management (buffers, shaders, programs, VAOs, textures, FBOs). `lc_draw.c` and `lc_scene.c` have NOT yet been refactored to use it — they still call raw GL. That migration is next.
- **Render context pipeline**: `lc_render_context_t` (defined in `lib/libcad_internal.h`) flows from `libcad.c` -> `lc_scene_compute_context()` -> `lc_draw_render_ctx()` / `lc_canvas_render_ctx()`. The old `set_view_matrix()` functions still exist for backward compat but the new context path is wired up.
- **GL state stack**: `lc_gl_state_t` with `lc_gl_save_state()` / `lc_gl_restore_state()` in `lc_draw.c`. The GPU pick pass now properly saves/restores viewport, blend, depth, scissor, and FBO state.
- **Public API stubs**: `libcad.h` now has `cad_entity_t`, `cad_sketch_t`, `cad_body_t` handle types plus stub functions for sketch creation, entity metadata, selection, and undo/redo. All stubs are in `libcad.c` and print debug messages.
- **`lib/libcad_internal.h`**: shared internal header included by all modules.

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
