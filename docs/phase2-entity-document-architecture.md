---
title: Phase 2 - Entity, Document, and Undo/Redo Architecture
description: Architectural specification for libcad's document model, entity storage, and undo/redo system
phase: 2
status: design
related-files:
  - include/libcad/libcad.h
  - lib/lc_entity.c (new)
  - lib/lc_entity.h (new)
  - lib/lc_document.c (new)
  - lib/lc_document.h (new)
  - lib/lc_undo.c (new)
  - lib/lc_undo.h (new)
  - lib/libcad.c
---

# Phase 2: Entity, Document, and Undo/Redo Architecture

## Overview

This document specifies the design for libcad's Phase 2 deliverables: entity storage, document model, and undo/redo system. These form the foundational data structures for all parametric CAD features including constraints, B-Rep topology, and generative design workflows.

The entity system provides handle-based storage with persistent IDs. The document model provides hierarchical organization (assemblies, bodies, sketches). The undo/redo system enables reversible operations using an immutable snapshot approach combined with delta commands.

## Requirements

### Functional Requirements

1. **Entity System**
   - Create, read, update, delete entities with persistent handles
   - Support multiple entity types: Sketch, Body, Face, Edge, Vertex, Constraint, etc.
   - Attach arbitrary user data to entities
   - Invalidate handles when entities are deleted (prevent use-after-free)
   - Efficient lookup by handle (O(1) or O(log n))
   - Enumerate all entities of a given type

2. **Document Model**
   - Tree structure with parent-child relationships
   - Root assembly that owns all top-level bodies and sketches
   - Metadata per entity: name, visibility, locked state, layer/group
   - Reference counting for geometry sharing between bodies
   - Serialization to JSON for save/load operations
   - Bulk operations (hide all, lock all children, etc.)

3. **Undo/Redo System**
   - Reversible operations for all entity mutations
   - Stack depth limit (configurable, default 100 operations)
   - Command grouping (multiple operations as single undo step)
   - Compatibility with entity creation/deletion
   - Memory efficient for large documents

### Non-Functional Requirements

1. **Performance**
   - Entity lookup: O(1) average case
   - Document tree traversal: O(n) where n = number of children
   - Undo/redo: O(1) to O(n) depending on snapshot size
   - Maximum entities per document: 100,000+

2. **Memory**
   - Entity storage overhead: ~64 bytes per entity
   - Document tree overhead: ~32 bytes per node
   - Undo stack: configurable limit, typically 10-50 MB for large documents

3. **Platform Support**
   - Strict C99 compliance (no C++ features)
   - Desktop: Windows (MSVC), macOS (Clang), Linux (GCC)
   - Web: Emscripten (WebAssembly)
   - Single-threaded design (thread safety is NOT required; document this explicitly)

## Design

### Module Structure

Three new modules form Phase 2:

```
lib/
  lc_entity.h       /* Entity handle system */
  lc_entity.c       /* Entity storage, lookup, type registry */
  lc_document.h     /* Document tree API */
  lc_document.c     /* Tree traversal, metadata, serialization */
  lc_undo.h         /* Undo/redo command API */
  lc_undo.c         /* Command stack, snapshot management */
  libcad_internal.h /* Shared types (add entity/doc types here) */
```

### Data Structures

#### Entity System (lc_entity.h)

**Handle Design: Generational Indices**

We use generational indices to prevent use-after-free and enable efficient dense storage.

```c
/* Entity handle is a 32-bit value:
 *   - Upper 16 bits: generation counter (prevents stale handles)
 *   - Lower 16 bits: slot index (max 65535 entities)
 */
typedef uint32_t lc_entity_handle_t;

#define LC_ENTITY_INVALID ((lc_entity_handle_t)0)

/* Extract index from handle */
#define LC_ENTITY_INDEX(h)      ((uint16_t)((h) & 0xFFFF))

/* Extract generation from handle */
#define LC_ENTITY_GENERATION(h) ((uint16_t)(((h) >> 16) & 0xFFFF))

/* Construct handle from index and generation */
#define LC_ENTITY_MAKE_HANDLE(idx, gen) \
    (((uint32_t)(gen) << 16) | (uint32_t)(idx))
```

**Entity Slot**

Each slot in the entity registry stores:

```c
/* Entity storage slot.
 * Uses discriminated union pattern for type-specific data. */
typedef struct lc_entity_slot_t
{
    lc_entity_handle_t handle;    /* Current handle (includes generation) */
    lc_entity_type_t type;        /* Entity type discriminator */
    uint32_t flags;               /* Metadata flags (see below) */

    /* Parent/child relationships (used by document tree) */
    lc_entity_handle_t parent;    /* Parent entity (LC_ENTITY_INVALID if root) */
    lc_entity_handle_t first_child;
    lc_entity_handle_t next_sibling;
    lc_entity_handle_t prev_sibling;

    /* Entity-specific data */
    union {
        lc_sketch_data_t *sketch;
        lc_body_data_t *body;
        lc_constraint_data_t *constraint;
        void *generic;            /* For future entity types */
    } data;

    /* User data attachment (optional) */
    void *user_data;
    void (*user_data_destructor)(void *);

} lc_entity_slot_t;
```

**Entity Types**

```c
/* Entity type enumeration.
 * Determines which union field in lc_entity_slot_t is valid. */
typedef enum lc_entity_type_t
{
    LC_ENTITY_TYPE_INVALID = 0,
    LC_ENTITY_TYPE_ASSEMBLY,      /* Root or sub-assembly */
    LC_ENTITY_TYPE_SKETCH,        /* 2D sketch plane */
    LC_ENTITY_TYPE_BODY,          /* 3D solid body */
    LC_ENTITY_TYPE_FACE,          /* B-Rep face (future) */
    LC_ENTITY_TYPE_EDGE,          /* B-Rep edge (future) */
    LC_ENTITY_TYPE_VERTEX,        /* B-Rep vertex (future) */
    LC_ENTITY_TYPE_CONSTRAINT,    /* 2D constraint (future) */
    LC_ENTITY_TYPE_GEOMETRY_LINE, /* 2D line in sketch */
    LC_ENTITY_TYPE_GEOMETRY_CIRCLE, /* 2D circle in sketch */
    LC_ENTITY_TYPE_GEOMETRY_RECT,  /* 2D rectangle in sketch */
    LC_ENTITY_TYPE_COUNT
} lc_entity_type_t;
```

**Entity Metadata Flags**

```c
/* Entity metadata bit flags.
 * Stored in lc_entity_slot_t.flags field. */
#define LC_ENTITY_FLAG_VISIBLE   (1 << 0)  /* Entity is visible */
#define LC_ENTITY_FLAG_LOCKED    (1 << 1)  /* Entity cannot be modified */
#define LC_ENTITY_FLAG_SELECTED  (1 << 2)  /* Entity is selected */
#define LC_ENTITY_FLAG_DELETED   (1 << 3)  /* Entity is soft-deleted (for undo) */
```

**Entity Registry**

The registry uses a sparse array with free list for fast allocation.

```c
/* Global entity registry.
 * Static storage in lc_entity.c; not exposed to other modules. */
#define LC_ENTITY_MAX_SLOTS 65536

typedef struct lc_entity_registry_t
{
    lc_entity_slot_t slots[LC_ENTITY_MAX_SLOTS];
    uint16_t free_list_head;  /* Index of first free slot */
    uint16_t slot_count;      /* Number of allocated slots (including free) */
    uint16_t generation[LC_ENTITY_MAX_SLOTS]; /* Generation counter per slot */
} lc_entity_registry_t;
```

**Type-Specific Data Structures**

```c
/* Sketch data attached to LC_ENTITY_TYPE_SKETCH entities. */
typedef struct lc_sketch_data_t
{
    vec3 origin;          /* World position of sketch plane origin */
    vec3 normal;          /* Sketch plane normal (unit vector) */
    vec3 x_axis;          /* Sketch plane X-axis (unit vector) */

    /* Geometry children are stored in entity tree (first_child list) */
    /* No explicit array needed; walk first_child -> next_sibling chain */

} lc_sketch_data_t;

/* Body data attached to LC_ENTITY_TYPE_BODY entities.
 * For Phase 2, bodies are empty containers; Phase 4 adds B-Rep. */
typedef struct lc_body_data_t
{
    /* B-Rep data will be added in Phase 4 */
    void *brep_data;  /* Placeholder; NULL for now */

} lc_body_data_t;

/* Constraint data (future; Phase 3).
 * Included here for completeness but not implemented in Phase 2. */
typedef struct lc_constraint_data_t
{
    int constraint_type;
    lc_entity_handle_t entities[4];  /* Constraint references (e.g., two lines) */
    float value;                     /* Constraint parameter (distance, angle, etc.) */
} lc_constraint_data_t;

/* 2D line geometry in a sketch. */
typedef struct lc_geometry_line_data_t
{
    vec2 start;  /* Start point in sketch-local coordinates */
    vec2 end;    /* End point in sketch-local coordinates */
    uint32_t color;  /* RGBA packed color */
    float thickness; /* Line thickness in pixels */
} lc_geometry_line_data_t;

/* 2D circle geometry in a sketch. */
typedef struct lc_geometry_circle_data_t
{
    vec2 center;     /* Center in sketch-local coordinates */
    float radius;
    uint32_t color;
} lc_geometry_circle_data_t;

/* 2D rectangle geometry in a sketch. */
typedef struct lc_geometry_rect_data_t
{
    vec2 min;  /* Bottom-left corner in sketch-local coordinates */
    vec2 max;  /* Top-right corner in sketch-local coordinates */
    uint32_t color;
} lc_geometry_rect_data_t;
```

#### Document Model (lc_document.h)

The document module provides high-level operations on the entity tree.

```c
/* Document metadata attached to each entity.
 * Stored in a parallel hash table keyed by entity handle.
 * Reason for separation: Not all entities need metadata; saves memory. */
typedef struct lc_entity_metadata_t
{
    char name[64];        /* User-visible name */
    char layer[32];       /* Layer/group name */
    int reference_count;  /* For geometry sharing (future) */
} lc_entity_metadata_t;

/* Document state.
 * Single global instance in lc_document.c. */
typedef struct lc_document_t
{
    lc_entity_handle_t root_assembly;  /* Root of document tree */

    /* Metadata table: hash map entity_handle -> lc_entity_metadata_t
     * Simple open-addressed hash table; max 65536 entries. */
    lc_entity_metadata_t *metadata_table;
    size_t metadata_capacity;
    size_t metadata_count;

    /* Selection state */
    lc_entity_handle_t *selection;  /* Dynamic array of selected handles */
    size_t selection_count;
    size_t selection_capacity;

    /* Document dirty flag (for save prompts) */
    bool dirty;

} lc_document_t;
```

**Tree Traversal**

The entity tree uses intrusive linked lists (first_child, next_sibling, prev_sibling). This avoids separate allocation for tree structure and keeps it cache-friendly.

Traversal patterns:

```c
/* Iterate over all children of an entity */
lc_entity_handle_t child = lc_entity_get_first_child(parent);
while (child != LC_ENTITY_INVALID)
{
    /* Process child */
    child = lc_entity_get_next_sibling(child);
}

/* Recursive traversal (pre-order depth-first) */
void traverse(lc_entity_handle_t entity)
{
    /* Process entity */

    lc_entity_handle_t child = lc_entity_get_first_child(entity);
    while (child != LC_ENTITY_INVALID)
    {
        traverse(child);  /* Recurse */
        child = lc_entity_get_next_sibling(child);
    }
}
```

#### Undo/Redo System (lc_undo.h)

We use a hybrid approach:
- **Immutable snapshots** for entity creation/deletion (saves full entity slot state)
- **Delta commands** for property changes (saves old value + new value)
- **Command grouping** for multi-step operations (e.g., extrude = create body + create faces)

**Command Structure**

```c
/* Command type enumeration.
 * Determines which union field in lc_undo_command_t is valid. */
typedef enum lc_command_type_t
{
    LC_COMMAND_CREATE_ENTITY,   /* Entity was created */
    LC_COMMAND_DELETE_ENTITY,   /* Entity was deleted */
    LC_COMMAND_MODIFY_PROPERTY, /* Property changed (name, position, etc.) */
    LC_COMMAND_MODIFY_GEOMETRY, /* Geometry data changed (line endpoints, etc.) */
    LC_COMMAND_GROUP_BEGIN,     /* Begin grouped commands */
    LC_COMMAND_GROUP_END,       /* End grouped commands */
} lc_command_type_t;

/* Property identifier for LC_COMMAND_MODIFY_PROPERTY. */
typedef enum lc_property_id_t
{
    LC_PROPERTY_NAME,
    LC_PROPERTY_VISIBILITY,
    LC_PROPERTY_LOCKED,
    LC_PROPERTY_LAYER,
    LC_PROPERTY_POSITION,  /* For sketch origin, body transform, etc. */
    LC_PROPERTY_COLOR,
} lc_property_id_t;

/* Undo command stores enough information to reverse an operation.
 * Commands are immutable once recorded. */
typedef struct lc_undo_command_t
{
    lc_command_type_t type;
    lc_entity_handle_t entity;  /* Affected entity */

    /* Command-specific data */
    union {
        /* LC_COMMAND_CREATE_ENTITY: save full entity snapshot */
        struct {
            lc_entity_slot_t snapshot;  /* Copy of slot at creation time */
        } create;

        /* LC_COMMAND_DELETE_ENTITY: save full entity snapshot */
        struct {
            lc_entity_slot_t snapshot;  /* Copy of slot before deletion */
        } delete;

        /* LC_COMMAND_MODIFY_PROPERTY: save old and new values */
        struct {
            lc_property_id_t property_id;
            char old_value[256];  /* Serialized old value */
            char new_value[256];  /* Serialized new value */
        } modify_property;

        /* LC_COMMAND_MODIFY_GEOMETRY: save old and new geometry data */
        struct {
            void *old_data;  /* Deep copy of old geometry (malloc'd) */
            void *new_data;  /* Deep copy of new geometry (malloc'd) */
            size_t data_size;
        } modify_geometry;

    } data;

} lc_undo_command_t;
```

**Undo Stack**

```c
/* Undo/redo stack.
 * Single global instance in lc_undo.c. */
#define LC_UNDO_MAX_STACK_DEPTH 100

typedef struct lc_undo_stack_t
{
    lc_undo_command_t commands[LC_UNDO_MAX_STACK_DEPTH];
    int undo_index;  /* Index of next command to undo (-1 if none) */
    int redo_index;  /* Index of next command to redo (-1 if none) */
    int command_count;  /* Total commands in stack */

    /* Group nesting depth (for LC_COMMAND_GROUP_BEGIN/END) */
    int group_depth;

} lc_undo_stack_t;
```

### API Surface

#### Entity System (lc_entity.h)

```c
/* Initialize entity system.
 * Call once at startup before any entity operations. */
void lc_entity_init(void);

/* Shutdown entity system and free all resources.
 * Call once at shutdown; invalidates all entity handles. */
void lc_entity_shutdown(void);

/* Create a new entity of the specified type.
 * Returns valid handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_entity_create(lc_entity_type_t type);

/* Destroy an entity and invalidate its handle.
 * Returns true if entity existed and was destroyed, false otherwise. */
bool lc_entity_destroy(lc_entity_handle_t handle);

/* Check if a handle is valid (entity exists and generation matches).
 * Returns true if valid, false if handle is stale or entity was deleted. */
bool lc_entity_is_valid(lc_entity_handle_t handle);

/* Get entity type.
 * Returns LC_ENTITY_TYPE_INVALID if handle is invalid. */
lc_entity_type_t lc_entity_get_type(lc_entity_handle_t handle);

/* Get entity flags (visibility, locked, selected, etc.).
 * Returns 0 if handle is invalid. */
uint32_t lc_entity_get_flags(lc_entity_handle_t handle);

/* Set entity flags.
 * Returns true if successful, false if handle is invalid. */
bool lc_entity_set_flags(lc_entity_handle_t handle, uint32_t flags);

/* Get type-specific data pointer.
 * Returns NULL if handle is invalid or type mismatch.
 * Caller must cast to appropriate type based on entity type. */
void* lc_entity_get_data(lc_entity_handle_t handle);

/* Attach user data to an entity.
 * If destructor is non-NULL, it will be called when entity is destroyed.
 * Returns true if successful, false if handle is invalid. */
bool lc_entity_set_user_data(lc_entity_handle_t handle, void *user_data,
                              void (*destructor)(void*));

/* Get user data attached to an entity.
 * Returns NULL if no user data or handle is invalid. */
void* lc_entity_get_user_data(lc_entity_handle_t handle);

/* Parent/child tree manipulation */
bool lc_entity_add_child(lc_entity_handle_t parent, lc_entity_handle_t child);
bool lc_entity_remove_child(lc_entity_handle_t parent, lc_entity_handle_t child);
lc_entity_handle_t lc_entity_get_parent(lc_entity_handle_t entity);
lc_entity_handle_t lc_entity_get_first_child(lc_entity_handle_t entity);
lc_entity_handle_t lc_entity_get_next_sibling(lc_entity_handle_t entity);
lc_entity_handle_t lc_entity_get_prev_sibling(lc_entity_handle_t entity);

/* Enumerate all entities of a given type.
 * Fills out_handles array with up to max_count handles.
 * Returns actual number of entities found (may exceed max_count). */
size_t lc_entity_enumerate_type(lc_entity_type_t type,
                                 lc_entity_handle_t *out_handles,
                                 size_t max_count);
```

#### Document System (lc_document.h)

```c
/* Initialize document system.
 * Creates root assembly entity. */
void lc_document_init(void);

/* Shutdown document system and free all resources. */
void lc_document_shutdown(void);

/* Get root assembly entity.
 * All top-level bodies and sketches are children of this entity. */
lc_entity_handle_t lc_document_get_root(void);

/* Metadata operations */
bool lc_document_set_entity_name(lc_entity_handle_t entity, const char *name);
const char* lc_document_get_entity_name(lc_entity_handle_t entity);

bool lc_document_set_entity_layer(lc_entity_handle_t entity, const char *layer);
const char* lc_document_get_entity_layer(lc_entity_handle_t entity);

/* Selection operations */
void lc_document_select_entity(lc_entity_handle_t entity);
void lc_document_deselect_entity(lc_entity_handle_t entity);
void lc_document_deselect_all(void);
bool lc_document_is_selected(lc_entity_handle_t entity);
size_t lc_document_get_selection_count(void);
const lc_entity_handle_t* lc_document_get_selection(size_t *out_count);

/* Visibility and locking */
void lc_document_set_visible(lc_entity_handle_t entity, bool visible);
bool lc_document_is_visible(lc_entity_handle_t entity);
void lc_document_set_locked(lc_entity_handle_t entity, bool locked);
bool lc_document_is_locked(lc_entity_handle_t entity);

/* Bulk operations (recursive on children) */
void lc_document_hide_all_children(lc_entity_handle_t parent);
void lc_document_show_all_children(lc_entity_handle_t parent);
void lc_document_lock_all_children(lc_entity_handle_t parent);
void lc_document_unlock_all_children(lc_entity_handle_t parent);

/* Dirty flag management */
void lc_document_mark_dirty(void);
void lc_document_clear_dirty(void);
bool lc_document_is_dirty(void);

/* Serialization (JSON format) */
bool lc_document_save_json(const char *filepath);
bool lc_document_load_json(const char *filepath);
```

#### Undo/Redo System (lc_undo.h)

```c
/* Initialize undo system. */
void lc_undo_init(void);

/* Shutdown undo system and free all resources. */
void lc_undo_shutdown(void);

/* Begin a command group.
 * All commands until lc_undo_end_group() will be undone/redone together.
 * Groups can be nested. */
void lc_undo_begin_group(void);

/* End a command group. */
void lc_undo_end_group(void);

/* Record a command (internal use; called by entity/document modules).
 * Clears redo stack when a new command is recorded. */
void lc_undo_record_command(const lc_undo_command_t *command);

/* Undo the last command or command group.
 * Returns true if undo was performed, false if nothing to undo. */
bool lc_undo_perform(void);

/* Redo the last undone command or command group.
 * Returns true if redo was performed, false if nothing to redo. */
bool lc_redo_perform(void);

/* Check if undo is available. */
bool lc_undo_can_undo(void);

/* Check if redo is available. */
bool lc_undo_can_redo(void);

/* Clear undo/redo stacks (e.g., after loading a new document). */
void lc_undo_clear(void);
```

### Internal Interfaces

These functions are called by `libcad.c` to wire up the public API stubs.

```c
/* In lc_entity.c - called by cad_create_sketch() */
lc_entity_handle_t lc_entity_create_sketch(vec3 origin, vec3 normal, vec3 x_axis);

/* In lc_entity.c - called by cad_sketch_add_line() */
lc_entity_handle_t lc_entity_create_line(lc_entity_handle_t sketch,
                                          vec2 start, vec2 end,
                                          uint32_t color, float thickness);

/* In lc_entity.c - called by cad_sketch_add_circle() */
lc_entity_handle_t lc_entity_create_circle(lc_entity_handle_t sketch,
                                            vec2 center, float radius,
                                            uint32_t color);

/* In lc_entity.c - called by cad_sketch_add_rect() */
lc_entity_handle_t lc_entity_create_rect(lc_entity_handle_t sketch,
                                          vec2 min, vec2 max,
                                          uint32_t color);

/* In lc_document.c - called by cad_delete_entity() */
bool lc_document_delete_entity(lc_entity_handle_t entity);
```

### Data Flow

**Entity Creation Flow**

```
User calls cad_create_sketch()
  ↓
libcad.c → lc_entity_create_sketch()
  ↓
lc_entity.c:
  1. Allocate slot from free list
  2. Increment generation counter
  3. Construct handle
  4. Allocate lc_sketch_data_t
  5. Initialize sketch data (origin, normal, x_axis)
  6. Add to document tree as child of root assembly
  ↓
lc_undo.c:
  7. Record LC_COMMAND_CREATE_ENTITY with snapshot
  ↓
lc_document.c:
  8. Mark document dirty
  ↓
Return handle to user
```

**Entity Deletion Flow**

```
User calls cad_delete_entity(handle)
  ↓
libcad.c → lc_document_delete_entity(handle)
  ↓
lc_undo.c:
  1. Record LC_COMMAND_DELETE_ENTITY with snapshot
  ↓
lc_entity.c:
  2. Soft-delete: set LC_ENTITY_FLAG_DELETED
     (Don't free slot yet; undo needs it)
  3. Remove from document tree (unlink from parent/siblings)
  4. Deselect if selected
  ↓
lc_document.c:
  5. Mark document dirty
  ↓
Return success
```

**Undo Flow**

```
User calls cad_undo()
  ↓
libcad.c → lc_undo_perform()
  ↓
lc_undo.c:
  1. Pop command from undo stack
  2. Switch on command type:

     LC_COMMAND_CREATE_ENTITY:
       - Call lc_entity_destroy() to reverse creation

     LC_COMMAND_DELETE_ENTITY:
       - Restore entity slot from snapshot
       - Clear LC_ENTITY_FLAG_DELETED
       - Re-insert into document tree

     LC_COMMAND_MODIFY_PROPERTY:
       - Restore old_value from command

     LC_COMMAND_MODIFY_GEOMETRY:
       - Swap old_data and new_data pointers

     LC_COMMAND_GROUP_BEGIN/END:
       - Recursively undo all commands in group

  3. Push command to redo stack
  ↓
lc_document.c:
  4. Mark document dirty
  ↓
Return success
```

**Rendering Integration**

Entities do not directly render themselves. Instead, `lc_canvas.c` and `lc_scene.c` query the entity system during their render passes:

```
cad_render_viewport()
  ↓
lc_scene_compute_context() → builds lc_render_context_t
  ↓
lc_canvas_render_ctx(ctx)
  ↓
lc_canvas.c:
  1. Query lc_document_get_root()
  2. Traverse children recursively
  3. For each LC_ENTITY_TYPE_SKETCH:
       a. Get lc_sketch_data_t via lc_entity_get_data()
       b. Transform sketch-local coords to world using sketch origin/normal/x_axis
       c. For each child LC_ENTITY_TYPE_GEOMETRY_LINE:
            - Get lc_geometry_line_data_t
            - Transform start/end to world coords
            - Call lc_draw_line()
       d. Same for CIRCLE, RECT, etc.
  4. Check visibility flag before rendering each entity
  5. Highlight selected entities (different color or outline)
```

### Platform Considerations

**Cross-Platform Compatibility**

1. **No C++ features**: All code is strict C99. Use `/* */` comments, no `//`.
2. **No platform-specific types**: Use stdint.h types (uint32_t, int64_t, etc.).
3. **Endianness**: JSON serialization is text-based; no binary endianness issues.
4. **File I/O**: Use stdio.h (fopen, fread, fwrite); works on all platforms.

**Emscripten Considerations**

1. **No threads**: Document that undo/redo is single-threaded by design.
2. **Memory limits**: 100,000 entity limit ensures reasonable memory usage on web.
3. **File system**: Emscripten's virtual file system supports stdio.h; no changes needed.

**Build System**

No new dependencies. Entity/document/undo modules use only:
- stdlib.h (malloc, free, memcpy)
- stdio.h (file I/O)
- stdint.h (fixed-width types)
- cglm (vec2, vec3, vec4 types already used elsewhere)
- jansson (JSON serialization; already a dependency)

Add to CMakeLists.txt:

```cmake
target_sources(libcad PRIVATE
    lib/lc_entity.c
    lib/lc_document.c
    lib/lc_undo.c
)
```

## Alternatives Considered

### 1. Entity Storage: Sparse Array vs Generational Indices

**Generational Indices (CHOSEN)**

Pros:
- Detects use-after-free (stale handles return invalid)
- Dense storage (no wasted slots for long-lived entities)
- Simple implementation (~200 lines)
- O(1) lookup, O(1) allocation (free list)

Cons:
- 16-bit generation counter can wrap (after 65535 create/delete cycles on same slot)
- Limit of 65535 entities (sufficient for CAD; most models < 10K entities)

**Sparse Array with Tombstones**

Pros:
- Never reuse slots (no generation counter needed)
- Simpler handle: just an index

Cons:
- Use-after-free bugs harder to detect
- Memory grows indefinitely (can't reclaim deleted slots)
- Not suitable for long-lived applications

**Verdict**: Generational indices strike the best balance for CAD workloads.

### 2. Undo Strategy: Immutable Snapshots vs Command Deltas

**Hybrid Approach (CHOSEN)**

Pros:
- Snapshots are simple and safe for create/delete (just copy entire slot)
- Deltas are memory-efficient for property changes (save only changed fields)
- Handles both small edits and bulk operations efficiently

Cons:
- Two code paths (snapshot and delta)
- Slightly more complex implementation

**Pure Snapshots**

Pros:
- Simplest implementation
- Foolproof correctness (always restore full state)

Cons:
- Memory explosion for large documents (each undo copies entire document)
- Impractical for 10K+ entity models

**Pure Deltas**

Pros:
- Minimal memory usage

Cons:
- Complex to implement (must track all property changes)
- Fragile (missing a field causes incorrect undo)

**Verdict**: Hybrid approach gives best tradeoff.

### 3. Document Tree: Intrusive Lists vs Pointer Arrays

**Intrusive Linked Lists (CHOSEN)**

Pros:
- Zero separate allocation for tree structure
- Cache-friendly (tree pointers live in entity slots)
- Constant memory overhead per entity

Cons:
- Manual traversal required (no random access)

**Pointer Arrays**

Pros:
- Random access to children
- Easy to sort children

Cons:
- Extra allocation per parent
- Dynamic resizing complexity
- Cache-unfriendly (pointer chasing)

**Verdict**: Intrusive lists align with libcad's philosophy (minimal allocation, explicit state).

### 4. Metadata Storage: Embedded vs Hash Table

**Hash Table (CHOSEN)**

Pros:
- Only allocates metadata when needed (most entities don't have custom names)
- Easy to extend with new metadata fields without inflating entity slots

Cons:
- Extra indirection for metadata access (O(1) hash lookup)

**Embedded in Entity Slot**

Pros:
- No indirection; metadata is always in cache with entity data

Cons:
- Wastes memory (every entity slot allocates 64 bytes for name even if unused)
- Harder to extend with new metadata fields

**Verdict**: Hash table saves memory and is more extensible.

## Implementation Notes

### Suggested Implementation Order

**Phase 2A: Entity System (1-2 days)**

1. Create `lib/lc_entity.h` and `lib/lc_entity.c`
2. Implement registry, free list, handle generation
3. Implement `lc_entity_create()`, `lc_entity_destroy()`, `lc_entity_is_valid()`
4. Implement tree manipulation functions (add_child, remove_child, etc.)
5. Add type-specific creation functions (create_sketch, create_line, etc.)
6. Test: Create 1000 entities, destroy half, verify handles invalidate correctly

**Phase 2B: Document System (1 day)**

1. Create `lib/lc_document.h` and `lib/lc_document.c`
2. Implement metadata hash table
3. Implement selection array
4. Implement visibility/locking helpers
5. Test: Create sketch, add lines, select/deselect, hide/show

**Phase 2C: Undo/Redo System (2-3 days)**

1. Create `lib/lc_undo.h` and `lib/lc_undo.c`
2. Implement command stack
3. Implement `lc_undo_record_command()`
4. Implement `lc_undo_perform()` and `lc_redo_perform()`
5. Wire up entity create/delete to record commands
6. Test: Create entity, undo, redo; modify property, undo, redo

**Phase 2D: Public API Integration (1 day)**

1. Modify `lib/libcad.c` to call lc_entity/lc_document functions
2. Replace stub implementations with real calls
3. Test: Call `cad_create_sketch()`, `cad_sketch_add_line()`, `cad_undo()` from democad

**Phase 2E: Rendering Integration (1-2 days)**

1. Modify `lc_canvas.c` to query entity tree instead of using its current `items` array
2. Traverse root assembly → sketches → geometry entities
3. Call `lc_draw_line()`, `lc_draw_circle()`, etc. based on entity type
4. Respect visibility flags
5. Highlight selected entities
6. Test: Create shapes via API, verify they render correctly

**Phase 2F: JSON Serialization (1 day)**

1. Implement `lc_document_save_json()` using jansson
2. Implement `lc_document_load_json()`
3. Serialize entity tree, metadata, geometry data
4. Test: Save document, load in new session, verify all data restored

**Total Estimated Effort: 7-10 days**

### Known Risks and Open Questions

**Risk: Generation Counter Wraparound**

If a single entity slot is created/deleted 65535 times, the generation counter wraps to 0. This could cause a handle created 65535 cycles ago to become valid again, leading to use-after-free.

**Mitigation**: For CAD workflows, this is extremely unlikely (user would need to create/delete the same slot 65K times). If it becomes an issue, increase generation to 24 bits (8 bits for index = max 255 entities; not sufficient). Better: use 64-bit handles (32-bit gen, 32-bit index).

**Recommendation**: Ship with 16-bit generation; monitor in practice; upgrade to 64-bit handles in Phase 3 if needed.

**Question: Lazy Loading for Large Assemblies**

For documents with 100K+ entities, should we load entities on-demand (lazy) or all at once (eager)?

**Recommendation**: Defer to Phase 4. For Phase 2, eager loading is simpler and sufficient for typical models (< 10K entities).

**Question: Thread Safety**

Should entity operations be thread-safe?

**Recommendation**: No. Document this explicitly. CAD document is single-threaded; GPU rendering happens on main thread. Future threadpool work (Phase 5) will be for constraint solving, not entity mutations.

**Question: Reference Counting for Geometry Sharing**

Sketches might reference shared geometry (e.g., two bodies share a face). How to handle?

**Recommendation**: Defer to Phase 4 (B-Rep). For Phase 2, geometry is owned by a single parent (no sharing).

### Dependencies on Existing Code

**Requires from Phase 1**

- `lc_render_context_t` (defined in `libcad_internal.h`) - used to transform sketch coords to world coords during rendering
- `lc_draw_line()`, `lc_draw_circle()`, `lc_draw_rect()` - called by entity rendering code
- cglm (vec2, vec3, mat4 types) - used in lc_sketch_data_t

**Integrates with**

- `lc_canvas.c` - modified to query entity tree instead of internal `items` array
- `libcad.c` - modified to call lc_entity/lc_document functions instead of stubs

**No changes required to**

- `lc_scene.c` - 3D scene is independent; entities will integrate in Phase 4 (B-Rep bodies)
- `lc_draw.c` - rendering API unchanged
- `lc_gpu.c` - GPU resources unchanged

### Migration Path from Current Test Shapes

**Current State (Phase 1)**

`lc_canvas.c` maintains an internal `items` array of `lc_canvas_item_t` structs. These are transient (lost on restart). They render via `lc_draw_*()` calls.

**Phase 2 Migration**

1. Keep `lc_canvas_item_t` structs temporarily for backward compatibility
2. When user creates a shape via `cad_sketch_add_line()`:
   - Create entity via `lc_entity_create_line()`
   - Also create `lc_canvas_item_t` (to keep old rendering code working)
3. Rendering pass queries both entity tree AND `items` array
4. Once Phase 2E is complete (rendering integration), delete `items` array entirely

This allows incremental migration without breaking existing functionality.

### Testing Strategy

**Unit Tests (Manual; democad replacement pending)**

1. **Entity Lifecycle**
   - Create 10K entities, verify all handles valid
   - Destroy 5K entities, verify handles invalid
   - Create new entities, verify slots reused

2. **Tree Operations**
   - Create parent, add 100 children, verify traversal order
   - Remove child, verify tree integrity
   - Delete parent, verify all children also deleted (or orphaned, depending on design choice)

3. **Undo/Redo**
   - Create entity, undo, verify entity gone
   - Create entity, undo, redo, verify entity restored
   - Modify property, undo, verify old value restored
   - Group 10 operations, undo once, verify all reversed

4. **Selection**
   - Select 10 entities, verify count = 10
   - Deselect one, verify count = 9
   - Delete selected entity, verify removed from selection array

5. **Serialization**
   - Create 100 entities with metadata
   - Save to JSON
   - Clear document
   - Load from JSON
   - Verify all entities, metadata, tree structure restored

**Integration Tests**

1. Call public API from Python/PyQt test harness (planned)
2. Create sketch, add shapes, undo/redo, save/load
3. Verify rendering matches expected output

**Performance Tests**

1. Create 100K entities, measure time (should be < 1 second)
2. Traverse 100K entity tree, measure time (should be < 100ms)
3. Undo/redo 1000 commands, measure memory usage (should be < 50MB)

## Summary

This architecture provides a solid foundation for Phase 2 and beyond. Key design decisions:

- **Generational indices** for safe handle-based entity storage
- **Intrusive linked lists** for memory-efficient document tree
- **Hybrid undo** (snapshots + deltas) for correctness and efficiency
- **Hash table metadata** to avoid wasting memory on unused entity names
- **Single-threaded** design for simplicity (CAD is inherently sequential)

The implementation is pure C99, cross-platform, and integrates cleanly with Phase 1's rendering architecture. Estimated effort is 7-10 days for a complete, tested implementation.

After Phase 2, libcad will have persistent documents with undo/redo, enabling constraint-driven parametric modeling (Phase 3) and B-Rep solid geometry (Phase 4).
