# libcad Development Progress

Tracks progress against `docs/action-plan.md`.

## Phase 1: Viewport & Coordinate System Foundation

### 1. Fix Viewport State Corruption (BLOCKER)
- [x] **1.1** Audit GL state changes in lc_draw.c
- [x] **1.2** Implement GL state stack (`lc_gl_state_t` + save/restore in `lc_draw.c`)
- [x] **1.3** Update pick pass to use state stack (pick_object_at now saves/restores all GL state)
- [ ] **1.4** Test with visual inspection

### 2. Unified Coordinate System (HIGH)
- [x] **2.1** Define `lc_render_context_t` struct (in `lib/libcad_internal.h`)
- [x] **2.2** Refactor `lc_scene_render()` — added `lc_scene_compute_context()` that populates `lc_render_context_t`
- [x] **2.3** Update `lc_draw_render()` — added `lc_draw_render_ctx(const lc_render_context_t *ctx)`
- [x] **2.3b** Update `lc_canvas_render()` — added `lc_canvas_render_ctx(const lc_render_context_t *ctx)`
- [x] **2.3c** Wire up in `libcad.c` — `cad_render_viewport()` now builds context via `lc_scene_compute_context()` and passes to `lc_draw_render_ctx()`
- [ ] **2.4** Define canvas-as-plane concept (comments/docs)

### 3. Consolidate 2D Drawing (HIGH)
- [x] **3.1** Identify ImGui DrawList calls — found 10 in `lc_draw.c` wrapper functions
- [x] **3.2** Add missing SDF shape functions (axes, grid) to instanced pipeline — `lc_draw_line()` and `lc_draw_grid()` now use SDF instancing
- [~] **3.3** Replace ImGui DrawList calls in `lc_draw.c` wrappers with SDF instancing — PARTIAL: line and grid done; still TODO: circle, rect, ellipse, handle, text
- [ ] **3.4** Remove cimgui includes from `lc_draw.h`

### 4. Module Boundary Cleanup (HIGH)
- [x] **4.1** Document module responsibilities (added to all 4 headers)
- [x] **4.2** Partially done — render context eliminates `lc_draw_set_view_matrix()` / `lc_canvas_set_view_matrix()` cross-module calls (old functions kept for backward compat)
- [x] **4.3** Implement GPU resource management in `lc_gpu.c` — full handle-based API for buffers, shaders, programs, VAOs, textures, FBOs (535 lines)

### 5. Shared Internal Types (MEDIUM)
- [x] **5.1** Create `lib/libcad_internal.h` with `lc_render_context_t`
- [x] **5.2** All modules now include `libcad_internal.h`

### 6. Public API Extensions (MEDIUM)
- [x] **6.1** Extend `libcad.h` with entity handle types (`cad_entity_t`, `cad_sketch_t`, `cad_body_t`) and sketch/line/circle/rect creation stubs
- [x] **6.2** Add entity selection API stubs (`cad_select_entity`, `cad_deselect_all`, `cad_get_selection_count`)
- [x] **6.3** Add undo/redo API stubs (`cad_undo`, `cad_redo`, `cad_can_undo`, `cad_can_redo`)
- [x] **6.4** Add entity metadata API (`cad_set_entity_name`, `cad_get_entity_name`)

## What's Next

### Remaining Phase 1 work:
- [~] **3.3-3.4** Complete migration of remaining shapes (circle, rect, ellipse, handle, text) to SDF instancing
- [ ] **4.3b** Refactor `lc_draw.c` to actually USE `lc_gpu` API instead of raw GL calls
- [ ] **2.4** Canvas-as-plane documentation

### Completed 2026-01-29:
- ✓ Increased MAX_INSTANCES from 64 to 1024 to accommodate grid rendering
- ✓ Added frame reset logic to `lc_draw_begin()` to enable dynamic instance accumulation
- ✓ Implemented `add_line_instance()` helper function for SDF line rendering
- ✓ Converted `lc_draw_line()` from ImGui DrawList to SDF instancing (SHAPE_LINE)
- ✓ Grid rendering now uses SDF pipeline (calls `lc_draw_line()` in loops)
- ✓ Axes rendering now uses SDF pipeline (in `lc_canvas.c`, calls `lc_draw_line()`)

### Phase 2: Document Model & Entity Storage

**Completed 2026-01-29:**
- [x] **Architecture Design** — Created comprehensive `docs/phase2-entity-document-architecture.md` with full specification for entity system, document tree, and undo/redo (generational indices, intrusive linked lists, hybrid undo approach)

**Implementation Tasks:**
- [x] **Phase 2A** (COMPLETED): Created `lc_entity.c/h` (834 + 266 lines)
  - Generational index handles (16-bit index + 16-bit generation)
  - Free list allocation with O(1) create/destroy
  - Intrusive tree structure (parent, first_child, next/prev sibling)
  - Type-specific data structures (sketch, body, line, circle, rect)
  - Complete API: init, shutdown, create, destroy, validation, tree manipulation, enumeration
  - Integrated into libcad.c (entity_init, create_sketch, add_line/circle/rect)
  - Test suite created (test_entity.c with 6 test categories)
  - Build verified: compiles successfully on Windows/MSVC
- [x] **Phase 2B** (COMPLETED): Created `lc_document.c/h` (612 + 172 lines)
  - Open-addressed metadata hash table (1024 slots, linear probing)
  - Dynamic selection array (starts at 16, grows 2x, shrinks at 25% usage)
  - Complete API: metadata (name, layer), selection, visibility/locking, query by name
  - Default values: visible=true, locked=false, name="", layer=""
  - Integrated into CMakeLists.txt for both native and Emscripten builds
  - Build verified: libcad.lib now 500KB (was 414KB)
- [x] **Phase 2C** (COMPLETED): Created `lc_undo.c/h` (669 + 178 lines)
  - Hybrid approach: immutable snapshots for create/delete, delta commands for property changes
  - Command stack with 100 entry capacity, overflow drops oldest
  - Command grouping for multi-step operations (GROUP_BEGIN/GROUP_END markers)
  - Complete undo/redo logic with forward/backward traversal
  - Deep copying of entity snapshots including type-specific data
  - Complete API: init, shutdown, record, begin/end_group, undo/redo, can_undo/redo, clear
  - Integrated into CMakeLists.txt
  - Build verified: libcad.lib now 537KB (was 500KB)
- [ ] **Phase 2D** (PARTIAL): Wire entity stubs in `libcad.c` — sketch/geometry functions done, metadata/selection/undo stubs remain
- [ ] **Phase 2E** (1-2 days): Modify `lc_canvas.c` to query entity tree instead of internal `items` array
- [ ] **Phase 2F** (1 day): Implement JSON serialization using jansson

**Estimated Total Effort: 7-10 days**

## Phase 3: Constraint System

**Architecture Design (COMPLETED 2026-01-29):**
- [x] Created comprehensive architecture document `docs/phase3-constraint-system-architecture.md`
- 15 constraint types with error functions
- Constraint graph + DOF analysis
- Integration with Phase 2 entity system
- 8-12 day implementation plan (phases 3A-3G)

**Implementation Tasks:**
- [x] **Phase 3A** (COMPLETED): Extended entity system with 16 constraint types, full constraint data structure
- [x] **Phase 3B** (COMPLETED): Created lc_constraint.c/h (459 + 124 lines) with 15 creation functions, management API
- [x] **Phase 3C** (COMPLETED 2026-01-30): Implemented error functions for all 16 constraint types
  - Added geometry query helpers: get_point_position, get_line_endpoints, get_circle_params
  - Implemented 15 error computation functions (distance, angle, coincident, parallel, perpendicular, horizontal, vertical, tangent, equal, fix)
  - Updated lc_constraint_evaluate() to dispatch to appropriate error function
  - Added comprehensive error tolerance checking (1e-6)
  - File grew from 459 to 1050 lines (+591 lines)
  - Library size: 606KB (was 579KB, +27KB)
  - Test suite extended with 10 comprehensive error evaluation tests
  - Math validation: All error functions verified with standalone tests
- [x] **Phase 3D** (COMPLETED 2026-01-30): Constraint graph & DOF analysis
  - Added graph data structures to libcad_internal.h (lc_constraint_graph_t, node/edge types)
  - Implemented graph caching system (up to 16 sketches)
  - Implemented lc_constraint_build_graph() - builds graph from sketch entities
  - Implemented DOF computation for entity types (line=4, circle=3, rect=4)
  - Implemented DOF removed computation for constraint types (1 or 2 DOF per constraint)
  - Implemented graph analysis with node flags (under/fully/over-constrained)
  - Implemented lc_constraint_get_dof() - returns net DOF for sketch
  - Implemented lc_constraint_is_fully_constrained() and lc_constraint_is_over_constrained()
  - Implemented lc_constraint_invalidate_graph() for cache management
  - File grew from 1050 to 1470 lines (+420 lines)
  - Header grew from 151 to 178 lines (+27 lines)
  - Library size: 619KB (was 606KB, +13KB)
  - Test suite: test_graph.c with 8 DOF analysis scenarios
- [x] **Phase 3E** (COMPLETED 2026-01-30): Public API integration
  - Added 24 constraint functions to public API (libcad.h)
  - Implemented wrapper functions in libcad.c
  - Constraint creation: 15 functions (distance, angle, parallel, coincident, tangent, equal, fix)
  - Constraint management: 5 functions (delete, set/get value, get error, is satisfied)
  - Sketch analysis: 4 functions (get DOF, is fully/over constrained, get count)
  - All wrappers convert public handles to internal handles
  - libcad.c grew from 377 to 658 lines (+281 lines)
  - libcad.h grew from 114 to 173 lines (+59 lines)
  - Library size: 638KB (was 619KB, +19KB)
- [x] **Phase 3F** (COMPLETED 2026-01-30): Undo/redo integration
  - Added LC_PROPERTY_CONSTRAINT_VALUE to undo property types
  - Entity creation now records CREATE_ENTITY undo commands
  - Entity deletion now records DELETE_ENTITY undo commands
  - Constraint value modification records MODIFY_PROPERTY commands
  - Added lc_constraint_invalidate_all_graphs() for bulk invalidation
  - Undo/redo automatically invalidate all constraint graphs
  - Graph invalidation on entity/constraint destruction
  - lc_entity.c: 834 → 900 lines (+66 lines)
  - lc_undo.c: 669 → 676 lines (+7 lines)
  - lc_constraint.c: 1470 → 1485 lines (+15 lines)
  - libcad.c: 658 → 678 lines (+20 lines)
  - Library size: 642KB (was 638KB, +4KB)
- [ ] **Phase 3G** (1 day, optional): Constraint visualization

**Completed 2026-01-30:**
- ✓ Phase 4A: Geometry System - Complete curve and surface implementation
- ✓ 3 curve types (line, circle, ellipse) with full parameterization
- ✓ 5 surface types (plane, cylinder, sphere, cone, torus) with full parameterization
- ✓ Evaluation functions for position, tangent, and normals
- ✓ Curve length computation (exact for line/circle, approximate for ellipse)
- ✓ Geometry registry with 65K capacity for curves and surfaces
- ✓ Test suite with 9 comprehensive tests covering all types
- ✓ Added to CMakeLists.txt for both native and Emscripten builds

## Phase 4: B-Rep Topology Kernel

**Architecture Design (COMPLETED 2026-01-29):**
- [x] Created comprehensive architecture document `docs/phase4-brep-kernel-architecture.md` (1077 lines)
- Complete B-Rep hierarchy (Vertex, Edge, Loop, Face, Shell, Solid)
- Edge-use pattern for topology representation
- Geometric primitives (curves, surfaces)
- Euler operators for manifold validity
- 12-18 day implementation plan (phases 4A-4G)

**Implementation Tasks:**
- [x] **Phase 4A** (COMPLETED 2026-01-30): Geometry system - curves, surfaces, evaluation functions
  - Created lc_geometry.h (304 lines) - Type definitions for curves and surfaces
  - Created lc_geometry.c (811 lines) - Full implementation with registry and evaluation
  - Geometry registry: 65,536 curves + 65,536 surfaces capacity
  - Curve types: line, circle, ellipse (with evaluation and tangent computation)
  - Surface types: plane, cylinder, sphere, cone, torus (with evaluation and normal computation)
  - Evaluation functions: eval_curve, eval_curve_tangent, eval_surface, eval_surface_normal
  - Utility functions: curve_length (exact for line/circle, approximate for ellipse)
  - Projection/intersection functions stubbed for future implementation
  - Test suite: test_geometry.c with 9 comprehensive tests (8 geometric types + large scale)
  - Build verified: All tests pass, library size 363KB (Release build)
- [x] **Phase 4B** (COMPLETED 2026-01-30): B-Rep entity types - vertex, edge, loop, face, shell, solid
  - Extended lc_entity.h with B-Rep data structures (vertex, edge, edge_use, loop, face, shell)
  - Edge data includes edge_uses[2] array for O(1) face adjacency lookup
  - Edge-use pattern with forward/reverse orientation and next_in_loop chain
  - Loop, face, and shell data structures with back-references
  - Entity system handles creation/destruction of all B-Rep types
- [x] **Phase 4C** (COMPLETED 2026-01-30): Primitive construction - box (cylinder/sphere stubbed)
  - Created lc_brep.h (77 lines) - B-Rep construction API
  - Created lc_brep.c (~590 lines) - Full box constructor implementation
  - Box creates: 8 vertices, 12 shared edges, 24 edge uses, 6 loops, 6 faces, 1 shell, 1 solid
  - Edge sharing via find_or_create_edge() - checks existing edges before creating new ones
  - Edge uses registered in parent edge's edge_uses[] array for fast adjacency
  - Solid destruction recursively collects and destroys all child entities
  - Validation checks Euler characteristic and edge manifoldness
  - Public API wired: cad_create_box, cad_validate_solid, face/edge/vertex counts
- [x] **Phase 4D** (COMPLETED 2026-01-30): Topological queries - navigation, adjacency
  - Created lc_topology.h (93 lines) - Topology query API
  - Created lc_topology.c (~568 lines) - Full query implementation
  - Hierarchy traversal: get_shells, get_faces, get_loops, get_edge_uses, get_edges
  - Vertex access: get_edge_vertices, get_vertex_position
  - Adjacency: get_edge_faces (O(1) via edge_uses[] array)
  - Analysis: count_elements (with deduplication), check_euler (V-E+F=2)
  - Bounding box: compute_bbox (walks all unique vertices)
  - Test suite: test_brep.c with 7 tests (creation, traversal, bbox, validation, destroy, edge sharing, multiple boxes)
  - All 7 tests pass: V=8, E=12, F=6, Euler=2, correct bbox, edge adjacency=2
- [x] **Phase 4E** (COMPLETED 2026-01-30): Tessellation - mesh generation for rendering
  - Created lc_tessellate.h (90 lines) - Tessellation API with mesh types and functions
  - Created lc_tessellate.c (457 lines) - Full tessellation implementation
  - Mesh vertex structure: position[3] + normal[3]
  - Mesh structure: vertices array + indices array
  - Face tessellation: tessellate_face() with mesh caching in face_data->mesh_data
  - Solid tessellation: tessellate_solid() traverses all faces and tessellates each
  - Planar face support: ear-clipping triangulation (fan from first vertex for convex faces)
  - Face normal computation: cross product of first two edges, respects face->forward flag
  - Mesh management: get_mesh(), invalidate(), free_mesh()
  - Mesh caching: stored in lc_face_data_t->mesh_data with dirty flag
  - Test suite: test_tessellate.c (379 lines) with 6 comprehensive tests
  - All 6 tests pass: box faces have 4 vertices + 2 triangles each (12 total)
  - Face normals verified: unit length, axis-aligned for box faces
  - Mesh invalidation and cleanup verified
  - Vertex positions verified within bounding box
  - Added to CMakeLists.txt for both native and Emscripten builds
  - Build verified: libcad.lib now 425KB, all existing tests still pass
- [x] **Phase 4F** (COMPLETED 2026-01-31): Rendering integration - display B-Rep bodies
  - Created 4 mesh shader files with Lambertian lighting: mesh_core_vs.glsl, mesh_core_fs.glsl, mesh_es_vs.glsl, mesh_es_fs.glsl
  - Added shader compilation and linking in lc_scene_init() with error handling
  - Implemented render_solid() function that tessellates solids and renders shaded faces + wireframe edges
  - Implemented render_all_solids() that enumerates all SOLID entities and renders visible ones
  - Wired rendering into lc_scene_render() after demo cube rendering (visible when SHOW_SOLIDS defined)
  - Updated CMakeLists.txt embed_resources() for both desktop and Emscripten to include mesh shaders
  - Updated lc_scene.h with includes for lc_entity.h, lc_topology.h, lc_tessellate.h
  - Build verified: zero errors, all tests pass
- [x] **Phase 4G** (COMPLETED 2026-01-31): Euler operators for topologically-safe B-Rep modification
  - Created lc_euler.h (170 lines) - Public API for 7 Euler operators
  - Created lc_euler.c (~1150 lines) - Full implementation with helper functions
  - Operators: MVEF, MEV, MEF, KEV, KEF, MEKL, KEML
  - Helper functions: vertex/edge/edge-use/loop/face creation, edge use chain manipulation
  - Edge use chain splicing for loop manipulation (MEV spike, MEF split, KEF merge, MEKL/KEML)
  - Proper entity cleanup via lc_entity_destroy (which frees data internally)
  - Tessellation invalidation on all topology changes
  - Test suite: test_euler.c with 8 tests (MVEF, MEV, MEF, KEV round-trip, KEF round-trip, triangle build, MEKL/KEML round-trip, invalid inputs)
  - All 8 tests pass
  - Build verified: zero errors, zero warnings from libcad code

## Phase 5-7
Not started yet.

## Build Status
All changes compile cleanly on Windows (MSVC) with zero errors and zero warnings from libcad code.
