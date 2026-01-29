/***************************************************************
**
** libcad Header File
**
** File         :  lc_entity.h
** Module       :  libcad (entity system)
** Author       :  SH
** Created      :  2026-01-29 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Entity handle-based storage system with generational
**                 indices. Provides create, destroy, validation, tree
**                 manipulation, and type-specific data access.
**
***************************************************************/

#ifndef LC_ENTITY_H
#define LC_ENTITY_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <cglm/cglm.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

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

/* Maximum number of entity slots */
#define LC_ENTITY_MAX_SLOTS 65536

/* Entity metadata bit flags */
#define LC_ENTITY_FLAG_VISIBLE   (1 << 0)  /* Entity is visible */
#define LC_ENTITY_FLAG_LOCKED    (1 << 1)  /* Entity cannot be modified */
#define LC_ENTITY_FLAG_SELECTED  (1 << 2)  /* Entity is selected */
#define LC_ENTITY_FLAG_DELETED   (1 << 3)  /* Entity is soft-deleted (for undo) */

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

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

/* Entity storage slot.
 * Uses discriminated union pattern for type-specific data. */
typedef struct lc_entity_slot_t
{
    lc_entity_handle_t handle;    /* Current handle (includes generation) */
    lc_entity_type_t type;        /* Entity type discriminator */
    uint32_t flags;               /* Metadata flags (see above) */

    /* Parent/child relationships (used by document tree) */
    lc_entity_handle_t parent;    /* Parent entity (LC_ENTITY_INVALID if root) */
    lc_entity_handle_t first_child;
    lc_entity_handle_t next_sibling;
    lc_entity_handle_t prev_sibling;

    /* Entity-specific data */
    union
    {
        lc_sketch_data_t *sketch;
        lc_body_data_t *body;
        lc_constraint_data_t *constraint;
        lc_geometry_line_data_t *geometry_line;
        lc_geometry_circle_data_t *geometry_circle;
        lc_geometry_rect_data_t *geometry_rect;
        void *generic;            /* For future entity types */
    } data;

    /* User data attachment (optional) */
    void *user_data;
    void (*user_data_destructor)(void *);

} lc_entity_slot_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

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

/* Type-specific creation functions (called by libcad.c) */

/* Create a sketch entity with specified plane parameters.
 * Returns valid handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_entity_create_sketch(vec3 origin, vec3 normal, vec3 x_axis);

/* Create a line geometry entity within a sketch.
 * Returns valid handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_entity_create_line(lc_entity_handle_t sketch,
                                          vec2 start, vec2 end,
                                          uint32_t color, float thickness);

/* Create a circle geometry entity within a sketch.
 * Returns valid handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_entity_create_circle(lc_entity_handle_t sketch,
                                            vec2 center, float radius,
                                            uint32_t color);

/* Create a rectangle geometry entity within a sketch.
 * Returns valid handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_entity_create_rect(lc_entity_handle_t sketch,
                                          vec2 min, vec2 max,
                                          uint32_t color);

#ifdef __cplusplus
}
#endif

#endif /* LC_ENTITY_H */
