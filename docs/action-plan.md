---
title: libcad Development Action Plan
description: High-level roadmap and immediate tasks for libcad architecture and development
related-files:
  - lib/libcad.c
  - lib/lc_canvas.c
  - lib/lc_draw.c
  - lib/lc_scene.c
  - lib/lc_gpu.c
  - include/libcad/libcad.h
---

# libcad Development Action Plan

This document outlines both the long-term architectural vision for libcad and the immediate, concrete tasks that need to be completed. It is written for both human developers and AI agents that will work on the codebase.

## Overview

libcad is an embeddable C99 CAD library, not a standalone application. It is designed to be integrated into other applications (desktop UIs, web apps via Emscripten, generative/agentic design systems, simulation tools, etc.). The core philosophy is:

- **Embedded design**: Static modules with no OOP, similar to OpenGL/Vulkan architecture
- **GPU-first**: Offload calculations and rendering to OpenGL 3.3 (desktop) and WebGL2 (web)
- **Parallelizable**: Threadpool support for CPU-side work
- **Multi-domain**: 2D sketches embedded as planes within a 3D scene; eventually supports B-Rep and simulation

---

# PART I: HIGH-LEVEL ARCHITECTURE ROADMAP

The roadmap is organized into phases. Each phase builds on previous ones and delivers a complete, coherent subsystem.

## Phase 1: Viewport & Coordinate System Foundation (Current - Immediate)

**Goal**: Establish clean separation between 2D canvas, 3D scene, and rendering layers. Fix viewport state corruption bugs.

### Current State
- `lc_canvas.c`: 2D sketching with pan/zoom and modal tools
- `lc_draw.c`: Instanced SDF vector rendering (buggy - viewport state corruption during GPU pick pass)
- `lc_scene.c`: 3D scene with orbit/pan camera and basic cube rendering
- `lc_gpu.c`: Empty stub
- No unified coordinate system between 2D and 3D spaces

### Key Issues
1. **Viewport state corruption**: During GPU picking in `lc_draw`, viewport state is not properly restored, affecting subsequent 3D rendering
2. **Coordinate system fragmentation**: 2D canvas operates in local screen space, 3D scene in world space; no unified transformation pipeline
3. **Module boundary confusion**: `lc_canvas` and `lc_draw` both manage drawing; unclear separation of concerns
4. **ImGui DrawList remnants**: `lc_canvas` uses ImGui DrawList for some primitives instead of unified SDF pipeline

### Deliverables
1. **Coordinate system unification**
   - Define a single NDC (normalized device coordinates) pipeline
   - Canvas (2D) lives as a plane/sketch within the 3D world
   - All transforms go through a unified view-projection matrix
   - Related files: `lib/libcad.c`, `lib/lc_canvas.c`, `lib/lc_scene.c`, `lib/lc_draw.c`

2. **Fix viewport restoration bug**
   - After GPU pick pass in `lc_draw_render()`, explicitly restore viewport state
   - Add assertions to detect state corruption early
   - Related files: `lib/lc_draw.c`

3. **Consolidate 2D drawing**
   - Move all 2D shape rendering (lines, circles, rects, etc.) from `lc_canvas` ImGui DrawList to `lc_draw` SDF instancing
   - Update `lc_canvas_render()` to call `lc_draw_*` functions instead of ImGui primitives
   - Related files: `lib/lc_canvas.c`, `lib/lc_draw.c`, `lib/lc_draw.h`

4. **Define internal rendering protocol**
   - Establish how `lc_canvas`, `lc_draw`, and `lc_scene` communicate transform state
   - Create a shared context struct (e.g., `lc_render_context_t`) passed between modules
   - Related files: `lib/libcad.c`, `lib/lc_draw.h`, `lib/lc_canvas.h`, `lib/lc_scene.h`

---

## Phase 2: Document Model & Entity Storage (Next)

**Goal**: Build a foundation for persistent geometry, undo/redo, and multi-viewport support.

### Rationale
Without a document model, all shapes exist only as transient GPU instances. CAD requires:
- Named, persistent entities (sketches, solid bodies, components)
- Hierarchical organization (assemblies, groups, layers)
- Undo/redo stack
- Selection state decoupled from rendering

### Key Modules
- **`lc_document.c`**: Core document/scene graph (new)
  - Tree structure: assembly → body → face/sketch → geometry
  - Each node has persistent ID, metadata, visibility, locked state
  - Reference counting for geometry sharing

- **`lc_entity.c`**: Generic entity system (new)
  - Entity storage with handle-based access (ECS-lite pattern)
  - Support for arbitrary user data attachment
  - Change notification callbacks (observers)

- **`lc_undo.c`**: Undo/redo manager (new)
  - Immutable snapshots of document state
  - Reversible command pattern
  - Interaction with `lc_entity` for bulk operations

### Deliverables
1. **Entity handle system**
   - Define opaque handle type: `typedef uint32_t cad_entity_t;`
   - Sparse array or generational indices for storage
   - Related files: `lib/lc_entity.h`, `lib/lc_entity.c` (new)

2. **Document tree**
   - Root assembly node
   - Sketch, body, component nodes with parent pointers
   - Lazy loading support for large assemblies
   - Related files: `lib/lc_document.h`, `lib/lc_document.c` (new)

3. **Undo/redo stack**
   - Record command objects on each shape creation/modification
   - Replay on undo/redo
   - Related files: `lib/lc_undo.h`, `lib/lc_undo.c` (new)

4. **Public API extensions**
   - `cad_create_sketch()` → returns `cad_entity_t`
   - `cad_create_line(sketch_id, point1, point2)` → returns `cad_entity_t`
   - `cad_set_entity_name(entity_id, name)`
   - `cad_delete_entity(entity_id)` → records undo entry
   - Related files: `include/libcad/libcad.h`

---

## Phase 3: 2D Sketch System & Constraint Solver

**Goal**: Full constraint-driven 2D sketching on planes within the 3D world.

### Rationale
Parametric design starts with constrained sketches. This is foundational for:
- Feature-driven modeling (pad, pocket, revolve)
- Associative geometry updates
- Generative design (constraint satisfaction)

### Key Modules
- **`lc_sketch.c`**: Sketch plane management (new)
  - Sketch as plane in 3D space (origin, normal, x-axis)
  - Geometry storage (vertices, edges, constraints)
  - Screen-to-sketch coordinate transforms

- **`lc_constraint.c`**: Constraint system (new)
  - Constraint definitions: coincident, perpendicular, parallel, tangent, distance, angle, etc.
  - Degree-of-freedom tracking
  - Constraint conflict detection

- **`lc_solver.c`**: Constraint solver (new)
  - Iterative solver (e.g., Levenberg-Marquardt)
  - Newton-Raphson for algebraic constraints
  - Fallback to numeric approaches for unsolvable systems
  - Can run on GPU via compute shaders for large systems

### Deliverables
1. **Sketch plane abstraction**
   - Define sketch space with origin, basis vectors
   - Maintain active sketch context
   - Related files: `lib/lc_sketch.h`, `lib/lc_sketch.c` (new)

2. **Constraint types & database**
   - Enum for all constraint types
   - Storage: array of constraints, each referencing geometry entities
   - Related files: `lib/lc_constraint.h`, `lib/lc_constraint.c` (new)

3. **Solver kernel**
   - Gradient computation for constraint residuals
   - Newton iteration with line search
   - Convergence criteria
   - Related files: `lib/lc_solver.h`, `lib/lc_solver.c` (new)

4. **Sketch UI integration**
   - Update `lc_canvas` to operate on sketch plane coordinates
   - Constrain drawing tools (lines snap to endpoints, etc.)
   - Related files: `lib/lc_canvas.c` (modify), `lib/lc_sketch.h`

---

## Phase 4: B-Rep Kernel & Solid Modeling

**Goal**: 3D solid geometry with topology and boolean operations.

### Rationale
Generative and agentic design require:
- Programmatic solid creation (e.g., `create_box()`, `create_sphere()`)
- Topology queries (faces, edges, vertices)
- Boolean operations (union, difference, intersection)
- Face/edge/vertex selection and modification

### Key Modules
- **`lc_brep.c`**: Boundary representation (new)
  - Half-edge data structure or similar
  - Winged-edge or quad-edge for efficient traversal
  - Face, edge, vertex entities with persistent IDs

- **`lc_topology.c`**: Topology operations (new)
  - Face/edge/vertex queries (adjacency, orientation, etc.)
  - Manifold validation

- **`lc_boolean.c`**: Boolean operations (new)
  - Constructive solid geometry (CSG)
  - Can offload to GPU for parallel edge-face intersection
  - Fallback to robust CPU implementation

- **`lc_geometry.c`**: Geometric kernels (new)
  - Curve/surface intersection
  - Point-on-face tests
  - Closest-point queries
  - Tessellation for GPU rendering

### Deliverables
1. **B-Rep data structure**
   - Half-edge or DCEL representation
   - Persistent entity IDs for faces, edges, vertices
   - Ownership relationships (body → faces → edges)
   - Related files: `lib/lc_brep.h`, `lib/lc_brep.c` (new)

2. **Topology queries API**
   - `cad_brep_get_faces(body_id)` → array of face IDs
   - `cad_brep_face_edges(face_id)` → array of edge IDs
   - `cad_brep_edge_vertices(edge_id)` → vertex pair
   - Related files: `include/libcad/libcad.h`, `lib/lc_topology.c` (new)

3. **Boolean operations**
   - Union, difference, intersection
   - Clean up degenerate geometry post-operation
   - Related files: `lib/lc_boolean.h`, `lib/lc_boolean.c` (new)

4. **Feature-based modeling**
   - Pad/extrude: take sketch → create solid body
   - Pocket: take face + sketch → subtract
   - Revolve: take sketch + axis → rotate
   - Related files: `lib/lc_features.h`, `lib/lc_features.c` (new)

---

## Phase 5: GPU Compute Pipeline & Threadpool

**Goal**: Parallel processing for geometry, simulation, and rendering.

### Rationale
Real CAD systems must handle large models:
- Constraint solving for thousands of constraints
- Mesh generation and simplification
- Simulation (FEA, CFD) on derived geometries
- Real-time rendering optimization

### Key Modules
- **`lc_compute.c`**: GPU compute abstraction (new)
  - Wrapper for OpenGL compute shaders and transform feedback
  - Work distribution (work groups, reduce operations)
  - Host-device memory synchronization

- **`lc_threadpool.c`**: CPU threadpool (new)
  - Work queue with job dependencies
  - Work-stealing scheduler
  - Condition variables for synchronization

- **`lc_mesh.c`**: Tessellation & mesh generation (new)
  - Convert B-Rep to triangle meshes
  - Simplification/decimation for LOD
  - GPU marching cubes for implicit surfaces

### Deliverables
1. **GPU compute wrapper**
   - Compile and cache compute shaders
   - Dispatch with parameter binding
   - Readback results with synchronization
   - Related files: `lib/lc_compute.h`, `lib/lc_compute.c` (new)

2. **Threadpool implementation**
   - Job queue with priority
   - Work stealing across threads
   - Dependency tracking (job A waits for job B)
   - Related files: `lib/lc_threadpool.h`, `lib/lc_threadpool.c` (new)

3. **Mesh generation**
   - Tessellate B-Rep faces to GPU-friendly triangle meshes
   - Maintain correspondence (triangle → face mapping)
   - Related files: `lib/lc_mesh.h`, `lib/lc_mesh.c` (new)

---

## Phase 6: Generative & Parametric APIs

**Goal**: Enable agentic and programmatic design workflows.

### Rationale
libcad is not just a CAD kernel; it's a platform for:
- Generative design (AI generates constraints/geometry)
- Parametric modeling (scripts drive design)
- Embedded design in simulation tools
- Cloud-based design services

### Key Modules
- **`lc_api.c`**: High-level builder API (new)
  - Fluent interface for common operations
  - Example: `sketch(plane) → line(p1, p2) → constrain_distance(p1, p2, 10.0) → pad(sketch, 5.0)`
  - Minimize boilerplate for simple designs

- **`lc_script.c`**: Script/expression binding (new, optional)
  - Lua or similar for parametric dimensions
  - Parameter table for design variants

### Deliverables
1. **Builder pattern API**
   - Chainable function calls for model construction
   - Implicit active sketch/body context
   - Example signature: `cad_ctx_t ctx; sketch(ctx, plane); line(ctx, p1, p2); constrain_distance(ctx, ...);`
   - Related files: `lib/lc_api.h`, `lib/lc_api.c` (new), `include/libcad/libcad.h`

2. **Parameter export/import**
   - JSON schema for design parameters
   - Example: `{ "pocket_depth": 5.0, "num_holes": 4, ... }`
   - Regenerate design from JSON
   - Related files: `lib/lc_parameters.h`, `lib/lc_parameters.c` (new)

---

## Phase 7: Simulation Integration (Future)

**Goal**: Bidirectional integration with FEA/CFD solvers.

### Rationale
Design is iterative; simulation feedback drives design decisions. libcad should:
- Export geometry in standard formats (STEP, STL, etc.)
- Receive back analysis results (stress, deformation, flow)
- Visualize results as 3D field data overlaid on geometry

### Key Modules
- **`lc_export.c`**: File format export (new)
  - STEP (CAD standard)
  - STL (meshes)
  - IGES (legacy)
  - Parasolid (proprietary, license permitting)

- **`lc_analysis.c`**: Analysis data binding (new)
  - Store FEA results on mesh vertices/elements
  - Visualization shaders for scalar/vector fields

### Deliverables
1. **STEP export**
   - B-Rep → STEP file
   - Metadata preservation (names, parameters)
   - Related files: `lib/lc_export.c` (new)

2. **Result visualization**
   - Scalar field rendering (stress heatmap)
   - Vector field glyphs (displacements)
   - Related files: `lib/lc_analysis.c` (new), shader updates

---

# PART II: IMMEDIATE TASKS (Priority Order)

These are concrete, actionable tasks to complete in the next few sprints. They focus on Phase 1 deliverables and unblocking further development.

## Critical: Fix Viewport State Corruption Bug

**Priority**: BLOCKER - rendering is currently unreliable

**Context**: During GPU picking in `lc_draw_render()`, the viewport and other GL state is modified to render to the pick FBO. If this state is not properly restored, subsequent 3D scene rendering renders to the wrong viewport.

**Tasks**:

### 1.1 Audit GL state changes in lc_draw.c
- **File**: `lib/lc_draw.c`
- **Details**:
  - Find all `glViewport()`, `glBindFramebuffer()`, `glScissor()`, `glEnable()/glDisable()` calls
  - Document the state before and after
  - Identify which calls are inside the pick pass and which are in the main render pass
- **Acceptance**: Numbered list of state changes with line numbers and context

### 1.2 Implement state stack
- **File**: `lib/lc_draw.c` or `lib/lc_gpu.c`
- **Details**:
  - Create a simple GL state saver:
    ```c
    typedef struct {
        GLint viewport[4];
        GLint scissor[4];
        GLboolean scissor_enabled;
        GLuint bound_fbo;
        /* add more as needed */
    } lc_gl_state_t;

    static void lc_gl_push_state(lc_gl_state_t *state);
    static void lc_gl_pop_state(const lc_gl_state_t *state);
    ```
  - Call before pick FBO bind, call after unbind
- **Acceptance**: Compiles, no warnings; both functions implemented

### 1.3 Update pick rendering pass
- **File**: `lib/lc_draw.c`, function `lc_draw_render()`
- **Details**:
  - Before `glBindFramebuffer(GL_FRAMEBUFFER, pick_fbo)`: call `lc_gl_push_state(&saved_state)`
  - After all pick rendering and readback: call `lc_gl_pop_state(&saved_state)`
  - Explicitly restore viewport to the input dimensions
  - Add debug assertions: `assert(glGetError() == GL_NO_ERROR)` after state transitions
- **Acceptance**: Viewport state correctly restored; no GL errors logged

### 1.4 Test with visual inspection
- **Details**: Build and run existing test (democad or placeholder test harness)
- **Acceptance**: 3D scene and 2D overlays both render correctly without corruption

---

## High: Establish Unified Coordinate System

**Priority**: HIGH - required for all further 2D/3D integration

**Context**: Currently:
- `lc_canvas`: operates in 2D screen space with local pan/zoom transform
- `lc_scene`: operates in 3D world space
- `lc_draw`: renders 2D shapes but needs to know both 2D canvas space and 3D view-projection

### 2.1 Define rendering context struct
- **Files**: `lib/lc_draw.h`, `lib/libcad.c`
- **Details**:
  ```c
  typedef struct {
      mat4 view_projection;  /* world → clip space */
      mat4 view_projection_inv; /* clip space → world */
      int viewport_width, viewport_height;
      /* canvas-specific */
      vec2 canvas_origin;  /* world position of 2D plane origin */
      vec3 canvas_normal;  /* plane normal (typically z-axis) */
      float canvas_zoom;
  } lc_render_context_t;
  ```
  - This is passed to all render functions
  - Allows decoupling module transforms from global state
- **Acceptance**: Struct defined and used in at least one render function signature

### 2.2 Refactor lc_scene_render() to compute and pass view-projection
- **File**: `lib/lc_scene.c`
- **Details**:
  - Extract view-projection matrix computation into a separate function
  - Compute in `lc_scene_render()` and pass via `lc_render_context_t`
  - Related files: `lib/libcad.c` (update call site)
- **Acceptance**: View-projection matrix correctly computed and passed through context

### 2.3 Update lc_draw_render() signature
- **File**: `lib/lc_draw.c`, `lib/lc_draw.h`
- **Details**:
  ```c
  /* OLD */
  void lc_draw_render(float viewport_width, float viewport_height);

  /* NEW */
  void lc_draw_render(const lc_render_context_t *ctx);
  ```
  - Use view-projection from context instead of computing locally
  - Related files: `lib/libcad.c` (update call site)
- **Acceptance**: Function signature updated and all call sites refactored

### 2.4 Define canvas-as-plane concept
- **Files**: `lib/lc_canvas.h`, `lib/lc_canvas.c` (comment/docs only, no code yet)
- **Details**:
  - Add comment explaining that canvas is a Z=0 plane in world space
  - Canvas pan/zoom is a 2D transform within that plane
  - Document planned integration point with 3D scene
- **Acceptance**: Clear comments in code explaining coordinate systems

---

## High: Consolidate 2D Drawing Implementation

**Priority**: HIGH - eliminates ImGui DrawList usage, simplifies architecture

**Context**: Currently `lc_canvas` uses ImGui DrawList for axes/grid and some shapes; this is inefficient and causes tight coupling to ImGui.

### 3.1 Identify all ImGui DrawList calls in lc_canvas
- **File**: `lib/lc_canvas.c`
- **Details**:
  - Search for `ImDrawList_*` calls
  - Search for `igGetWindowDrawList()` calls
  - List all shapes drawn this way (axes, grid, items, etc.)
- **Acceptance**: Documented list with function names and approximate line numbers

### 3.2 Add missing SDF shape functions to lc_draw
- **Files**: `lib/lc_draw.c`, `lib/lc_draw.h`
- **Details**: Currently have circle, rect, ellipse; need:
  - `lc_draw_axes()` - draw X/Y axes with labels
  - `lc_draw_grid()` - draw background grid
  - These should use the same SDF instancing pipeline as other shapes
  - Update `vector_instance_t` struct if needed to support axis/grid rendering
- **Acceptance**: Both functions implemented and callable

### 3.3 Replace ImGui DrawList calls in lc_canvas
- **File**: `lib/lc_canvas.c`
- **Details**:
  - Replace `igGetWindowDrawList()` calls with corresponding `lc_draw_*()` calls
  - Remove ImGui DrawList dependency from `lc_canvas`
  - Update function signatures as needed (may need render context)
- **Acceptance**: No ImGui DrawList calls remain in lc_canvas; compiles and renders correctly

### 3.4 Remove ImGui integration from lc_draw header (optional cleanup)
- **Files**: `lib/lc_draw.h`, `lib/lc_draw.c`
- **Details**:
  - ImGui should not be imported in public module headers
  - Keep ImGui bindings internal if needed for debug UI
  - Create separate module for ImGui UI if debug visualizations are needed
- **Acceptance**: `#include <cimgui/...>` removed from `lc_draw.h`

---

## High: Module Boundary Cleanup

**Priority**: HIGH - improves code organization and testability

**Context**: Currently module responsibilities are fuzzy:
- What belongs in canvas vs. draw vs. scene?
- What state is shared across modules vs. encapsulated?

### 4.1 Document module responsibilities
- **Files**: Comments at top of each module
- **Details**:
  ```c
  /* Module: lc_canvas
   * Responsibility: 2D sketch plane management, tool state, undo/redo
   * Owns: pan/zoom state, active sketch, selection
   * Uses: lc_draw (for rendering), lc_scene (for world context)
   * Does NOT own: 3D geometry, GPU resources
   */
  ```
  - Update headers for all modules: `lc_canvas.h`, `lc_draw.h`, `lc_scene.h`, `lc_gpu.h`
- **Acceptance**: Clear, concise responsibility statement for each module

### 4.2 Eliminate cross-module static state dependencies
- **Files**: `lib/libcad.c`, `lib/lc_canvas.c`, `lib/lc_draw.c`, `lib/lc_scene.c`
- **Details**:
  - Currently modules access each other's state indirectly (e.g., canvas sets state, draw reads it)
  - Pass state explicitly via function parameters or context structs
  - Example: instead of `lc_draw_set_view_matrix()`, pass matrix in render context
  - Search for `lc_draw_set_view_matrix()`, `lc_canvas_set_view_matrix()` calls
  - Trace dependencies and refactor to use context
- **Acceptance**: All cross-module data flows through explicit parameters; no hidden global state

### 4.3 Move GPU resource management to lc_gpu
- **Files**: `lib/lc_gpu.c`, `lib/lc_draw.c`
- **Details**:
  - Currently `lc_draw.c` manages VAO, VBO, pick FBO, shaders
  - Move to `lc_gpu.c` with a handle-based API
  - Example:
    ```c
    typedef uintptr_t lc_gpu_buffer_t;
    lc_gpu_buffer_t lc_gpu_create_buffer(size_t size, const void *data);
    void lc_gpu_update_buffer(lc_gpu_buffer_t buf, size_t offset, size_t size, const void *data);
    void lc_gpu_destroy_buffer(lc_gpu_buffer_t buf);
    ```
  - `lc_draw.c` calls `lc_gpu_*` functions instead of calling OpenGL directly
- **Acceptance**: Key GPU resource management functions moved to `lc_gpu.h`

---

## Medium: Create Internal Header for Shared Types

**Priority**: MEDIUM - improves maintainability

**Context**: Currently some types are duplicated across modules (e.g., `vector_instance_t` in lc_draw).

### 5.1 Define libcad_internal.h
- **File**: `lib/libcad_internal.h` (new)
- **Details**:
  ```c
  /* Shared internal types and functions across all modules */

  /* Rendering context (used by canvas, draw, scene) */
  typedef struct { ... } lc_render_context_t;

  /* Vector shape instance (used by draw, canvas) */
  typedef struct { ... } lc_vector_instance_t;

  /* Shared functions */
  void lc_gl_push_state(...);
  void lc_gl_pop_state(...);
  ```
  - Document which modules include this
  - Not part of public API
- **Acceptance**: File created and imported by at least 3 modules

### 5.2 Update module includes
- **Files**: All `lib/*.c` files
- **Details**:
  - Remove duplicate type definitions
  - Replace with includes of `libcad_internal.h`
  - Ensures consistency
- **Acceptance**: No duplicate type definitions; all modules agree on shared types

---

## Medium: Update Public API with Document Model Stubs

**Priority**: MEDIUM - unblocks Phase 2 work without breaking current API

**Context**: The public API currently only has viewport/input functions; needs entity/document functions.

### 6.1 Extend libcad.h with entity types
- **File**: `include/libcad/libcad.h`
- **Details**:
  ```c
  /* Opaque handles for document entities */
  typedef uint32_t cad_sketch_t;
  typedef uint32_t cad_body_t;
  typedef uint32_t cad_entity_t;  /* generic */

  /* Shape creation - not yet implemented, but API in place */
  EXPORT cad_sketch_t  cad_create_sketch(cad_ctx_t ctx);
  EXPORT cad_entity_t  cad_sketch_add_line(cad_ctx_t ctx, cad_sketch_t sketch,
                                            float x1, float y1, float x2, float y2);
  EXPORT void          cad_delete_entity(cad_ctx_t ctx, cad_entity_t entity);
  ```
  - Implementation stubs for now (return dummy values, empty functions)
  - Will be filled in by Phase 2
- **Acceptance**: API extended; documentation comments added

### 6.2 Add entity selection API
- **File**: `include/libcad/libcad.h`
- **Details**:
  ```c
  EXPORT void          cad_select_entity(cad_ctx_t ctx, cad_entity_t entity);
  EXPORT void          cad_deselect_all(cad_ctx_t ctx);
  EXPORT cad_entity_t* cad_get_selected_entities(cad_ctx_t ctx, int *p_count);
  ```
  - Stubs for now
  - Required for interactive selection, constraint definition, etc.
- **Acceptance**: API defined with documentation

---

## Small: Add Debug Visualization Helpers

**Priority**: SMALL - improves development visibility

**Context**: Hard to debug rendering issues without visualizing internal state.

### 7.1 Create lc_debug.c module
- **File**: `lib/lc_debug.c` (new), `lib/lc_debug.h` (new)
- **Details**:
  ```c
  void lc_debug_draw_matrix(mat4 m, vec3 pos, float scale);
  void lc_debug_draw_aabb(vec3 min, vec3 max, uint32_t color);
  void lc_debug_draw_plane(vec3 origin, vec3 normal, float size, uint32_t color);
  void lc_debug_print_state(void);  /* dumps all module state for inspection */
  ```
  - Use `lc_draw` functions to visualize
  - Only compiled if `#define LC_DEBUG` or similar
- **Acceptance**: Module compiles; basic functions callable

### 7.2 Add state dump function
- **Files**: `lib/lc_debug.c`, all module headers
- **Details**:
  - Each module exports `void lc_<module>_debug_print_state(void);`
  - Called by `lc_debug_print_state()` to dump all state
  - Useful for inspecting zoom level, transform matrices, instance counts, etc.
- **Acceptance**: Calling `lc_debug_print_state()` outputs module state to stdout

---

## Small: Code Style & Documentation Pass

**Priority**: SMALL - improves maintainability

**Context**: Code follows conventions but could be more consistent.

### 8.1 Ensure all functions have header comments
- **Files**: All `lib/*.c` files
- **Details**:
  ```c
  /* Set the active sketch plane.
   * Params:
   *   sketch_id - handle to sketch entity
   * Returns:
   *   true if successful, false if sketch not found
   */
  static bool set_active_sketch(uint32_t sketch_id);
  ```
  - Focus on public/important internal functions (not every tiny helper)
- **Acceptance**: All 50+ significant functions have docstrings

### 8.2 Add MARK sections where missing
- **Files**: Files with over 200 lines should have MARK sections
- **Details**:
  - Follow pattern: `/* MARK: SECTION_NAME */`
  - Standard sections: INCLUDES, CONSTANTS & MACROS, TYPEDEFS, STATIC VARIABLES, STATIC FUNCTION DEFS, PUBLIC FUNCTIONS, STATIC FUNCTIONS
- **Acceptance**: All files follow this structure

---

## Small: Create Build & Test Documentation

**Priority**: SMALL - enables others to contribute

**Context**: CLAUDE.md exists but no detailed build/test guide for developers.

### 9.1 Create build-guide.md
- **File**: `docs/build-guide.md` (new)
- **Details**:
  - Prerequisites (compiler, CMake, SDL3, etc.)
  - Desktop build steps
  - Emscripten web build steps
  - Troubleshooting common issues
  - Related files: `CMakeLists.txt`, build directory
- **Acceptance**: Complete build guide with working examples

### 9.2 Create testing-guide.md
- **File**: `docs/testing-guide.md` (new)
- **Details**:
  - How to run democad or test harness
  - Where to add unit tests
  - Manual test checklist (viewport state, picking, transforms, etc.)
  - Related files: existing test infrastructure
- **Acceptance**: Guide explains how to verify correctness of changes

---

# Summary of Phases & Deliverables

| Phase | Goal | Key Modules | Estimated Effort |
|-------|------|-------------|------------------|
| 1 | Viewport & Coordinate System | lc_draw, lc_canvas, lc_scene, libcad | 1-2 weeks |
| 2 | Document Model | lc_entity, lc_document, lc_undo | 2-3 weeks |
| 3 | Sketch & Constraints | lc_sketch, lc_constraint, lc_solver | 3-4 weeks |
| 4 | B-Rep & Solids | lc_brep, lc_boolean, lc_topology | 4-6 weeks |
| 5 | GPU & Threadpool | lc_compute, lc_threadpool, lc_mesh | 2-3 weeks |
| 6 | Generative APIs | lc_api, lc_parameters | 1-2 weeks |
| 7 | Simulation | lc_export, lc_analysis | 2-3 weeks (optional) |

---

# Success Criteria

A task is complete when:

1. **Code changes are minimal and focused**: One responsibility per commit
2. **All function signatures match documentation**: No guessing
3. **Code compiles without warnings** on Windows (MSVC), macOS (Clang), Linux (GCC), Emscripten
4. **Visual tests pass**: Rendering is correct; no viewport corruption
5. **Module boundaries are clear**: Each module owns its state
6. **Comments explain the "why"**: Not just what the code does
7. **Related files are updated**: Headers, CMakeLists.txt, docs, etc.
8. **Tests or reproduction steps provided**: How to verify the change works

---

# Next Steps

1. **Immediate** (this sprint):
   - Tasks 1.1 - 1.3 (Fix viewport corruption)
   - Tasks 2.1 - 2.2 (Establish coordinate system)

2. **Short-term** (next 1-2 sprints):
   - Tasks 3.1 - 3.3 (Consolidate 2D drawing)
   - Tasks 4.1 - 4.3 (Module boundary cleanup)
   - Tasks 5.1 - 5.2 (Internal headers)

3. **Mid-term** (1 month):
   - Tasks 6.1 - 6.2 (Extend public API)
   - Begin Phase 2 (document model)

4. **Documentation**:
   - Tasks 8.1 - 8.2 (Code style pass)
   - Tasks 9.1 - 9.2 (Build & test guides)
