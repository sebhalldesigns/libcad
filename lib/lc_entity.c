/***************************************************************
**
** libcad Implementation File
**
** File         :  lc_entity.c
** Module       :  libcad (entity system)
** Author       :  SH
** Created      :  2026-01-29 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Entity system implementation. Provides handle-based
**                 storage with generational indices, free list allocation,
**                 tree manipulation, and type-specific data access.
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lc_entity.h"
#include "libcad_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/* Sentinel value for end of free list */
#define LC_FREE_LIST_END 0xFFFF

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/* Global entity registry.
 * Static storage in lc_entity.c; not exposed to other modules. */
typedef struct lc_entity_registry_t
{
    lc_entity_slot_t slots[LC_ENTITY_MAX_SLOTS];
    uint16_t free_list_head;  /* Index of first free slot */
    uint16_t slot_count;      /* Number of allocated slots (including free) */
    uint16_t generation[LC_ENTITY_MAX_SLOTS]; /* Generation counter per slot */
} lc_entity_registry_t;

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static lc_entity_registry_t g_registry;
static bool g_initialized = false;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static lc_entity_slot_t* lc_entity_get_slot(lc_entity_handle_t handle);
static void lc_entity_free_data(lc_entity_slot_t *slot);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void lc_entity_init(void)
{
    if (g_initialized)
    {
        return;
    }

    /* Clear registry */
    memset(&g_registry, 0, sizeof(g_registry));

    /* Initialize free list: all slots are free initially */
    g_registry.free_list_head = 0;
    g_registry.slot_count = 0;

    /* Link all slots into free list */
    uint16_t i;
    for (i = 0; i < LC_ENTITY_MAX_SLOTS - 1; i++)
    {
        /* Use handle field as next pointer in free list */
        g_registry.slots[i].handle = i + 1;
        g_registry.generation[i] = 1; /* Start generation at 1 (0 = invalid) */
    }
    g_registry.slots[LC_ENTITY_MAX_SLOTS - 1].handle = LC_FREE_LIST_END;
    g_registry.generation[LC_ENTITY_MAX_SLOTS - 1] = 1;

    g_initialized = true;

    printf("[lc_entity] Entity system initialized (max %d slots)\n", LC_ENTITY_MAX_SLOTS);
}

void lc_entity_shutdown(void)
{
    if (!g_initialized)
    {
        return;
    }

    /* Free all allocated entity data */
    uint16_t i;
    for (i = 0; i < LC_ENTITY_MAX_SLOTS; i++)
    {
        lc_entity_slot_t *slot = &g_registry.slots[i];
        if (slot->type != LC_ENTITY_TYPE_INVALID)
        {
            lc_entity_free_data(slot);
        }
    }

    /* Clear registry */
    memset(&g_registry, 0, sizeof(g_registry));
    g_initialized = false;

    printf("[lc_entity] Entity system shutdown\n");
}

lc_entity_handle_t lc_entity_create(lc_entity_type_t type)
{
    if (!g_initialized)
    {
        printf("[lc_entity] ERROR: Entity system not initialized\n");
        return LC_ENTITY_INVALID;
    }

    if (type == LC_ENTITY_TYPE_INVALID || type >= LC_ENTITY_TYPE_COUNT)
    {
        printf("[lc_entity] ERROR: Invalid entity type %d\n", type);
        return LC_ENTITY_INVALID;
    }

    /* Check if free list is empty */
    if (g_registry.free_list_head == LC_FREE_LIST_END)
    {
        printf("[lc_entity] ERROR: Entity registry full (max %d slots)\n", LC_ENTITY_MAX_SLOTS);
        return LC_ENTITY_INVALID;
    }

    /* Allocate from free list */
    uint16_t index = g_registry.free_list_head;
    lc_entity_slot_t *slot = &g_registry.slots[index];

    /* Update free list head to next free slot */
    g_registry.free_list_head = (uint16_t)slot->handle;

    /* Increment generation counter */
    uint16_t generation = g_registry.generation[index];

    /* Construct handle */
    lc_entity_handle_t handle = LC_ENTITY_MAKE_HANDLE(index, generation);

    /* Initialize slot */
    memset(slot, 0, sizeof(lc_entity_slot_t));
    slot->handle = handle;
    slot->type = type;
    slot->flags = LC_ENTITY_FLAG_VISIBLE; /* Visible by default */
    slot->parent = LC_ENTITY_INVALID;
    slot->first_child = LC_ENTITY_INVALID;
    slot->next_sibling = LC_ENTITY_INVALID;
    slot->prev_sibling = LC_ENTITY_INVALID;

    g_registry.slot_count++;

    return handle;
}

bool lc_entity_destroy(lc_entity_handle_t handle)
{
    if (!g_initialized)
    {
        return false;
    }

    lc_entity_slot_t *slot = lc_entity_get_slot(handle);
    if (slot == NULL)
    {
        return false;
    }

    /* Call user data destructor if present */
    if (slot->user_data_destructor != NULL && slot->user_data != NULL)
    {
        slot->user_data_destructor(slot->user_data);
    }

    /* Free type-specific data */
    lc_entity_free_data(slot);

    /* Remove from parent's child list */
    if (slot->parent != LC_ENTITY_INVALID)
    {
        lc_entity_remove_child(slot->parent, handle);
    }

    /* Mark as deleted */
    slot->flags |= LC_ENTITY_FLAG_DELETED;

    /* Increment generation to invalidate existing handles */
    uint16_t index = LC_ENTITY_INDEX(handle);
    g_registry.generation[index]++;
    if (g_registry.generation[index] == 0)
    {
        /* Wraparound: skip 0 (reserved for invalid) */
        g_registry.generation[index] = 1;
    }

    /* Reset slot */
    slot->type = LC_ENTITY_TYPE_INVALID;
    slot->flags = 0;
    slot->parent = LC_ENTITY_INVALID;
    slot->first_child = LC_ENTITY_INVALID;
    slot->next_sibling = LC_ENTITY_INVALID;
    slot->prev_sibling = LC_ENTITY_INVALID;
    slot->user_data = NULL;
    slot->user_data_destructor = NULL;

    /* Add to free list */
    slot->handle = g_registry.free_list_head;
    g_registry.free_list_head = index;

    g_registry.slot_count--;

    return true;
}

bool lc_entity_is_valid(lc_entity_handle_t handle)
{
    if (!g_initialized)
    {
        return false;
    }

    if (handle == LC_ENTITY_INVALID)
    {
        return false;
    }

    uint16_t index = LC_ENTITY_INDEX(handle);
    uint16_t generation = LC_ENTITY_GENERATION(handle);

    /* Check bounds */
    if (index >= LC_ENTITY_MAX_SLOTS)
    {
        return false;
    }

    /* Check generation matches */
    if (generation != g_registry.generation[index])
    {
        return false;
    }

    /* Check type is valid */
    lc_entity_slot_t *slot = &g_registry.slots[index];
    if (slot->type == LC_ENTITY_TYPE_INVALID)
    {
        return false;
    }

    /* Check not deleted */
    if (slot->flags & LC_ENTITY_FLAG_DELETED)
    {
        return false;
    }

    return true;
}

lc_entity_type_t lc_entity_get_type(lc_entity_handle_t handle)
{
    lc_entity_slot_t *slot = lc_entity_get_slot(handle);
    if (slot == NULL)
    {
        return LC_ENTITY_TYPE_INVALID;
    }
    return slot->type;
}

uint32_t lc_entity_get_flags(lc_entity_handle_t handle)
{
    lc_entity_slot_t *slot = lc_entity_get_slot(handle);
    if (slot == NULL)
    {
        return 0;
    }
    return slot->flags;
}

bool lc_entity_set_flags(lc_entity_handle_t handle, uint32_t flags)
{
    lc_entity_slot_t *slot = lc_entity_get_slot(handle);
    if (slot == NULL)
    {
        return false;
    }
    slot->flags = flags;
    return true;
}

void* lc_entity_get_data(lc_entity_handle_t handle)
{
    lc_entity_slot_t *slot = lc_entity_get_slot(handle);
    if (slot == NULL)
    {
        return NULL;
    }

    /* Return appropriate data pointer based on type */
    switch (slot->type)
    {
        case LC_ENTITY_TYPE_SKETCH:
            return slot->data.sketch;
        case LC_ENTITY_TYPE_BODY:
            return slot->data.body;
        case LC_ENTITY_TYPE_CONSTRAINT:
            return slot->data.constraint;
        case LC_ENTITY_TYPE_GEOMETRY_LINE:
            return slot->data.geometry_line;
        case LC_ENTITY_TYPE_GEOMETRY_CIRCLE:
            return slot->data.geometry_circle;
        case LC_ENTITY_TYPE_GEOMETRY_RECT:
            return slot->data.geometry_rect;
        default:
            return slot->data.generic;
    }
}

bool lc_entity_set_data(lc_entity_handle_t handle, void *data)
{
    lc_entity_slot_t *slot = lc_entity_get_slot(handle);
    if (slot == NULL)
    {
        return false;
    }

    /* Set appropriate data pointer based on type */
    switch (slot->type)
    {
        case LC_ENTITY_TYPE_SKETCH:
            slot->data.sketch = (lc_sketch_data_t*)data;
            break;
        case LC_ENTITY_TYPE_BODY:
            slot->data.body = (lc_body_data_t*)data;
            break;
        case LC_ENTITY_TYPE_CONSTRAINT:
            slot->data.constraint = (lc_constraint_data_t*)data;
            break;
        case LC_ENTITY_TYPE_GEOMETRY_LINE:
            slot->data.geometry_line = (lc_geometry_line_data_t*)data;
            break;
        case LC_ENTITY_TYPE_GEOMETRY_CIRCLE:
            slot->data.geometry_circle = (lc_geometry_circle_data_t*)data;
            break;
        case LC_ENTITY_TYPE_GEOMETRY_RECT:
            slot->data.geometry_rect = (lc_geometry_rect_data_t*)data;
            break;
        default:
            slot->data.generic = data;
            break;
    }

    return true;
}

bool lc_entity_set_user_data(lc_entity_handle_t handle, void *user_data,
                              void (*destructor)(void*))
{
    lc_entity_slot_t *slot = lc_entity_get_slot(handle);
    if (slot == NULL)
    {
        return false;
    }

    /* Call old destructor if present */
    if (slot->user_data_destructor != NULL && slot->user_data != NULL)
    {
        slot->user_data_destructor(slot->user_data);
    }

    slot->user_data = user_data;
    slot->user_data_destructor = destructor;
    return true;
}

void* lc_entity_get_user_data(lc_entity_handle_t handle)
{
    lc_entity_slot_t *slot = lc_entity_get_slot(handle);
    if (slot == NULL)
    {
        return NULL;
    }
    return slot->user_data;
}

bool lc_entity_add_child(lc_entity_handle_t parent, lc_entity_handle_t child)
{
    if (!g_initialized)
    {
        return false;
    }

    lc_entity_slot_t *parent_slot = lc_entity_get_slot(parent);
    lc_entity_slot_t *child_slot = lc_entity_get_slot(child);

    if (parent_slot == NULL || child_slot == NULL)
    {
        return false;
    }

    /* Remove child from previous parent if present */
    if (child_slot->parent != LC_ENTITY_INVALID)
    {
        lc_entity_remove_child(child_slot->parent, child);
    }

    /* Set parent */
    child_slot->parent = parent;

    /* Add to parent's child list (prepend to front) */
    child_slot->next_sibling = parent_slot->first_child;
    child_slot->prev_sibling = LC_ENTITY_INVALID;

    if (parent_slot->first_child != LC_ENTITY_INVALID)
    {
        lc_entity_slot_t *first_child_slot = lc_entity_get_slot(parent_slot->first_child);
        if (first_child_slot != NULL)
        {
            first_child_slot->prev_sibling = child;
        }
    }

    parent_slot->first_child = child;

    return true;
}

bool lc_entity_remove_child(lc_entity_handle_t parent, lc_entity_handle_t child)
{
    if (!g_initialized)
    {
        return false;
    }

    lc_entity_slot_t *parent_slot = lc_entity_get_slot(parent);
    lc_entity_slot_t *child_slot = lc_entity_get_slot(child);

    if (parent_slot == NULL || child_slot == NULL)
    {
        return false;
    }

    /* Verify child is actually a child of parent */
    if (child_slot->parent != parent)
    {
        return false;
    }

    /* Unlink from sibling list */
    if (child_slot->prev_sibling != LC_ENTITY_INVALID)
    {
        lc_entity_slot_t *prev_slot = lc_entity_get_slot(child_slot->prev_sibling);
        if (prev_slot != NULL)
        {
            prev_slot->next_sibling = child_slot->next_sibling;
        }
    }
    else
    {
        /* Child is first child of parent */
        parent_slot->first_child = child_slot->next_sibling;
    }

    if (child_slot->next_sibling != LC_ENTITY_INVALID)
    {
        lc_entity_slot_t *next_slot = lc_entity_get_slot(child_slot->next_sibling);
        if (next_slot != NULL)
        {
            next_slot->prev_sibling = child_slot->prev_sibling;
        }
    }

    /* Clear parent and sibling pointers */
    child_slot->parent = LC_ENTITY_INVALID;
    child_slot->prev_sibling = LC_ENTITY_INVALID;
    child_slot->next_sibling = LC_ENTITY_INVALID;

    return true;
}

lc_entity_handle_t lc_entity_get_parent(lc_entity_handle_t entity)
{
    lc_entity_slot_t *slot = lc_entity_get_slot(entity);
    if (slot == NULL)
    {
        return LC_ENTITY_INVALID;
    }
    return slot->parent;
}

lc_entity_handle_t lc_entity_get_first_child(lc_entity_handle_t entity)
{
    lc_entity_slot_t *slot = lc_entity_get_slot(entity);
    if (slot == NULL)
    {
        return LC_ENTITY_INVALID;
    }
    return slot->first_child;
}

lc_entity_handle_t lc_entity_get_next_sibling(lc_entity_handle_t entity)
{
    lc_entity_slot_t *slot = lc_entity_get_slot(entity);
    if (slot == NULL)
    {
        return LC_ENTITY_INVALID;
    }
    return slot->next_sibling;
}

lc_entity_handle_t lc_entity_get_prev_sibling(lc_entity_handle_t entity)
{
    lc_entity_slot_t *slot = lc_entity_get_slot(entity);
    if (slot == NULL)
    {
        return LC_ENTITY_INVALID;
    }
    return slot->prev_sibling;
}

size_t lc_entity_enumerate_type(lc_entity_type_t type,
                                 lc_entity_handle_t *out_handles,
                                 size_t max_count)
{
    if (!g_initialized)
    {
        return 0;
    }

    if (type == LC_ENTITY_TYPE_INVALID || type >= LC_ENTITY_TYPE_COUNT)
    {
        return 0;
    }

    size_t count = 0;
    uint16_t i;

    for (i = 0; i < LC_ENTITY_MAX_SLOTS; i++)
    {
        lc_entity_slot_t *slot = &g_registry.slots[i];

        /* Skip invalid or deleted slots */
        if (slot->type == LC_ENTITY_TYPE_INVALID)
        {
            continue;
        }
        if (slot->flags & LC_ENTITY_FLAG_DELETED)
        {
            continue;
        }

        /* Check type matches */
        if (slot->type == type)
        {
            /* Add to output array if space available */
            if (out_handles != NULL && count < max_count)
            {
                out_handles[count] = slot->handle;
            }
            count++;
        }
    }

    return count;
}

lc_entity_handle_t lc_entity_create_sketch(vec3 origin, vec3 normal, vec3 x_axis)
{
    lc_entity_handle_t handle = lc_entity_create(LC_ENTITY_TYPE_SKETCH);
    if (handle == LC_ENTITY_INVALID)
    {
        return LC_ENTITY_INVALID;
    }

    /* Allocate sketch data */
    lc_sketch_data_t *sketch_data = (lc_sketch_data_t*)malloc(sizeof(lc_sketch_data_t));
    if (sketch_data == NULL)
    {
        lc_entity_destroy(handle);
        printf("[lc_entity] ERROR: Failed to allocate sketch data\n");
        return LC_ENTITY_INVALID;
    }

    /* Initialize sketch data */
    glm_vec3_copy(origin, sketch_data->origin);
    glm_vec3_copy(normal, sketch_data->normal);
    glm_vec3_copy(x_axis, sketch_data->x_axis);

    /* Attach to entity */
    lc_entity_slot_t *slot = lc_entity_get_slot(handle);
    if (slot != NULL)
    {
        slot->data.sketch = sketch_data;
    }

    return handle;
}

lc_entity_handle_t lc_entity_create_line(lc_entity_handle_t sketch,
                                          vec2 start, vec2 end,
                                          uint32_t color, float thickness)
{
    /* Verify sketch is valid */
    if (!lc_entity_is_valid(sketch))
    {
        printf("[lc_entity] ERROR: Invalid sketch handle for line creation\n");
        return LC_ENTITY_INVALID;
    }

    if (lc_entity_get_type(sketch) != LC_ENTITY_TYPE_SKETCH)
    {
        printf("[lc_entity] ERROR: Parent is not a sketch\n");
        return LC_ENTITY_INVALID;
    }

    lc_entity_handle_t handle = lc_entity_create(LC_ENTITY_TYPE_GEOMETRY_LINE);
    if (handle == LC_ENTITY_INVALID)
    {
        return LC_ENTITY_INVALID;
    }

    /* Allocate line data */
    lc_geometry_line_data_t *line_data = (lc_geometry_line_data_t*)malloc(sizeof(lc_geometry_line_data_t));
    if (line_data == NULL)
    {
        lc_entity_destroy(handle);
        printf("[lc_entity] ERROR: Failed to allocate line data\n");
        return LC_ENTITY_INVALID;
    }

    /* Initialize line data */
    glm_vec2_copy(start, line_data->start);
    glm_vec2_copy(end, line_data->end);
    line_data->color = color;
    line_data->thickness = thickness;

    /* Attach to entity */
    lc_entity_slot_t *slot = lc_entity_get_slot(handle);
    if (slot != NULL)
    {
        slot->data.geometry_line = line_data;
    }

    /* Add as child of sketch */
    lc_entity_add_child(sketch, handle);

    return handle;
}

lc_entity_handle_t lc_entity_create_circle(lc_entity_handle_t sketch,
                                            vec2 center, float radius,
                                            uint32_t color)
{
    /* Verify sketch is valid */
    if (!lc_entity_is_valid(sketch))
    {
        printf("[lc_entity] ERROR: Invalid sketch handle for circle creation\n");
        return LC_ENTITY_INVALID;
    }

    if (lc_entity_get_type(sketch) != LC_ENTITY_TYPE_SKETCH)
    {
        printf("[lc_entity] ERROR: Parent is not a sketch\n");
        return LC_ENTITY_INVALID;
    }

    lc_entity_handle_t handle = lc_entity_create(LC_ENTITY_TYPE_GEOMETRY_CIRCLE);
    if (handle == LC_ENTITY_INVALID)
    {
        return LC_ENTITY_INVALID;
    }

    /* Allocate circle data */
    lc_geometry_circle_data_t *circle_data = (lc_geometry_circle_data_t*)malloc(sizeof(lc_geometry_circle_data_t));
    if (circle_data == NULL)
    {
        lc_entity_destroy(handle);
        printf("[lc_entity] ERROR: Failed to allocate circle data\n");
        return LC_ENTITY_INVALID;
    }

    /* Initialize circle data */
    glm_vec2_copy(center, circle_data->center);
    circle_data->radius = radius;
    circle_data->color = color;

    /* Attach to entity */
    lc_entity_slot_t *slot = lc_entity_get_slot(handle);
    if (slot != NULL)
    {
        slot->data.geometry_circle = circle_data;
    }

    /* Add as child of sketch */
    lc_entity_add_child(sketch, handle);

    return handle;
}

lc_entity_handle_t lc_entity_create_rect(lc_entity_handle_t sketch,
                                          vec2 min, vec2 max,
                                          uint32_t color)
{
    /* Verify sketch is valid */
    if (!lc_entity_is_valid(sketch))
    {
        printf("[lc_entity] ERROR: Invalid sketch handle for rect creation\n");
        return LC_ENTITY_INVALID;
    }

    if (lc_entity_get_type(sketch) != LC_ENTITY_TYPE_SKETCH)
    {
        printf("[lc_entity] ERROR: Parent is not a sketch\n");
        return LC_ENTITY_INVALID;
    }

    lc_entity_handle_t handle = lc_entity_create(LC_ENTITY_TYPE_GEOMETRY_RECT);
    if (handle == LC_ENTITY_INVALID)
    {
        return LC_ENTITY_INVALID;
    }

    /* Allocate rect data */
    lc_geometry_rect_data_t *rect_data = (lc_geometry_rect_data_t*)malloc(sizeof(lc_geometry_rect_data_t));
    if (rect_data == NULL)
    {
        lc_entity_destroy(handle);
        printf("[lc_entity] ERROR: Failed to allocate rect data\n");
        return LC_ENTITY_INVALID;
    }

    /* Initialize rect data */
    glm_vec2_copy(min, rect_data->min);
    glm_vec2_copy(max, rect_data->max);
    rect_data->color = color;

    /* Attach to entity */
    lc_entity_slot_t *slot = lc_entity_get_slot(handle);
    if (slot != NULL)
    {
        slot->data.geometry_rect = rect_data;
    }

    /* Add as child of sketch */
    lc_entity_add_child(sketch, handle);

    return handle;
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static lc_entity_slot_t* lc_entity_get_slot(lc_entity_handle_t handle)
{
    if (!g_initialized)
    {
        return NULL;
    }

    if (handle == LC_ENTITY_INVALID)
    {
        return NULL;
    }

    uint16_t index = LC_ENTITY_INDEX(handle);
    uint16_t generation = LC_ENTITY_GENERATION(handle);

    /* Check bounds */
    if (index >= LC_ENTITY_MAX_SLOTS)
    {
        return NULL;
    }

    /* Check generation matches */
    if (generation != g_registry.generation[index])
    {
        return NULL;
    }

    lc_entity_slot_t *slot = &g_registry.slots[index];

    /* Check type is valid */
    if (slot->type == LC_ENTITY_TYPE_INVALID)
    {
        return NULL;
    }

    /* Check not deleted */
    if (slot->flags & LC_ENTITY_FLAG_DELETED)
    {
        return NULL;
    }

    return slot;
}

static void lc_entity_free_data(lc_entity_slot_t *slot)
{
    if (slot == NULL)
    {
        return;
    }

    /* Free type-specific data */
    switch (slot->type)
    {
        case LC_ENTITY_TYPE_SKETCH:
            if (slot->data.sketch != NULL)
            {
                free(slot->data.sketch);
                slot->data.sketch = NULL;
            }
            break;

        case LC_ENTITY_TYPE_BODY:
            if (slot->data.body != NULL)
            {
                free(slot->data.body);
                slot->data.body = NULL;
            }
            break;

        case LC_ENTITY_TYPE_CONSTRAINT:
            if (slot->data.constraint != NULL)
            {
                free(slot->data.constraint);
                slot->data.constraint = NULL;
            }
            break;

        case LC_ENTITY_TYPE_GEOMETRY_LINE:
            if (slot->data.geometry_line != NULL)
            {
                free(slot->data.geometry_line);
                slot->data.geometry_line = NULL;
            }
            break;

        case LC_ENTITY_TYPE_GEOMETRY_CIRCLE:
            if (slot->data.geometry_circle != NULL)
            {
                free(slot->data.geometry_circle);
                slot->data.geometry_circle = NULL;
            }
            break;

        case LC_ENTITY_TYPE_GEOMETRY_RECT:
            if (slot->data.geometry_rect != NULL)
            {
                free(slot->data.geometry_rect);
                slot->data.geometry_rect = NULL;
            }
            break;

        default:
            /* Generic data - caller is responsible for freeing */
            break;
    }
}
