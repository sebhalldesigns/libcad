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
- [ ] **3.2** Add missing SDF shape functions (axes, grid) to instanced pipeline
- [ ] **3.3** Replace ImGui DrawList calls in `lc_draw.c` wrappers with SDF instancing
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
- [ ] **3.2-3.4** Migrate ImGui DrawList wrappers in `lc_draw.c` to SDF instancing pipeline
- [ ] **4.3b** Refactor `lc_draw.c` to actually USE `lc_gpu` API instead of raw GL calls
- [ ] **2.4** Canvas-as-plane documentation

### Phase 2: Document Model & Entity Storage
- [ ] Create `lc_entity.c` — entity handle system with sparse storage
- [ ] Create `lc_document.c` — document tree (assembly -> body -> sketch -> geometry)
- [ ] Create `lc_undo.c` — undo/redo command stack
- [ ] Wire entity stubs in `libcad.c` to actual entity system

## Phase 3-7
Not started.

## Build Status
All changes compile cleanly on Windows (MSVC) with zero errors and zero warnings from libcad code.
