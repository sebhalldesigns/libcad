/***************************************************************
**
** libcad Implementation File
**
** File         :  lc_document.c
** Module       :  libcad (document system)
** Author       :  SH
** Created      :  2026-01-29 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Document tree management, metadata storage, and
**                 selection state. Hash table for metadata, dynamic
**                 array for selection. Default values: visible=true,
**                 locked=false, name="", layer="".
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lc_document.h"
#include "lc_entity.h"
#include "libcad_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/* Initial capacity for selection array */
#define SELECTION_INITIAL_CAPACITY 16

/* Hash function for entity handles */
#define HASH_ENTITY(handle) ((handle) % LC_DOCUMENT_METADATA_TABLE_SIZE)

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

/* Global document instance */
static lc_document_t g_document;

/* Default empty strings */
static const char *EMPTY_STRING = "";

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

/* Hash table helper: find metadata slot for entity handle.
 * Returns pointer to slot (occupied or first free slot for insertion).
 * Returns NULL if table is full (should never happen with proper sizing). */
static lc_entity_metadata_t* find_metadata_slot(lc_entity_handle_t entity, bool for_insert);

/* Selection array helper: find index of entity in selection array.
 * Returns index if found, or selection_count if not found. */
static size_t find_selection_index(lc_entity_handle_t entity);

/* Selection array helper: grow capacity if needed. */
static void ensure_selection_capacity(void);

/* Selection array helper: shrink capacity if too large. */
static void shrink_selection_if_needed(void);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void lc_document_init(void)
{
    /* Zero out document state */
    memset(&g_document, 0, sizeof(g_document));

    /* Create root assembly */
    g_document.root_assembly = lc_entity_create(LC_ENTITY_TYPE_ASSEMBLY);
    if (g_document.root_assembly == LC_ENTITY_INVALID)
    {
        fprintf(stderr, "[lc_document] ERROR: Failed to create root assembly\n");
        return;
    }

    /* Initialize metadata table */
    memset(g_document.metadata_table, 0, sizeof(g_document.metadata_table));
    g_document.metadata_count = 0;

    /* Initialize selection array */
    g_document.selection_capacity = SELECTION_INITIAL_CAPACITY;
    g_document.selection = (lc_entity_handle_t*)malloc(
        sizeof(lc_entity_handle_t) * g_document.selection_capacity
    );
    if (!g_document.selection)
    {
        fprintf(stderr, "[lc_document] ERROR: Failed to allocate selection array\n");
        g_document.selection_capacity = 0;
        return;
    }
    g_document.selection_count = 0;

    /* Document starts clean */
    g_document.dirty = false;

    printf("[lc_document] Initialized (root assembly handle=%u)\n", g_document.root_assembly);
}

void lc_document_shutdown(void)
{
    /* Free selection array */
    if (g_document.selection)
    {
        free(g_document.selection);
        g_document.selection = NULL;
    }

    /* Metadata table is statically allocated; no need to free entries */
    /* (Entity data cleanup is handled by lc_entity_shutdown) */

    /* Destroy root assembly (will cascade to children via entity system) */
    if (g_document.root_assembly != LC_ENTITY_INVALID)
    {
        lc_entity_destroy(g_document.root_assembly);
        g_document.root_assembly = LC_ENTITY_INVALID;
    }

    /* Zero out document state */
    memset(&g_document, 0, sizeof(g_document));

    printf("[lc_document] Shutdown complete\n");
}

lc_entity_handle_t lc_document_get_root(void)
{
    return g_document.root_assembly;
}

bool lc_document_set_entity_name(lc_entity_handle_t entity, const char *name)
{
    if (!lc_entity_is_valid(entity))
    {
        return false;
    }

    if (!name)
    {
        name = EMPTY_STRING;
    }

    /* Find or allocate metadata slot */
    lc_entity_metadata_t *meta = find_metadata_slot(entity, true);
    if (!meta)
    {
        fprintf(stderr, "[lc_document] ERROR: Metadata table full\n");
        return false;
    }

    /* If slot was newly allocated, initialize it */
    if (!meta->occupied)
    {
        meta->occupied = true;
        meta->key = entity;
        meta->name[0] = '\0';
        meta->layer[0] = '\0';
        meta->reference_count = 0;
        g_document.metadata_count++;
    }

    /* Copy name (truncate if too long) */
    strncpy(meta->name, name, sizeof(meta->name) - 1);
    meta->name[sizeof(meta->name) - 1] = '\0';

    g_document.dirty = true;
    return true;
}

const char* lc_document_get_entity_name(lc_entity_handle_t entity)
{
    if (!lc_entity_is_valid(entity))
    {
        return EMPTY_STRING;
    }

    lc_entity_metadata_t *meta = find_metadata_slot(entity, false);
    if (meta && meta->occupied)
    {
        return meta->name;
    }

    return EMPTY_STRING;
}

bool lc_document_set_entity_layer(lc_entity_handle_t entity, const char *layer)
{
    if (!lc_entity_is_valid(entity))
    {
        return false;
    }

    if (!layer)
    {
        layer = EMPTY_STRING;
    }

    /* Find or allocate metadata slot */
    lc_entity_metadata_t *meta = find_metadata_slot(entity, true);
    if (!meta)
    {
        fprintf(stderr, "[lc_document] ERROR: Metadata table full\n");
        return false;
    }

    /* If slot was newly allocated, initialize it */
    if (!meta->occupied)
    {
        meta->occupied = true;
        meta->key = entity;
        meta->name[0] = '\0';
        meta->layer[0] = '\0';
        meta->reference_count = 0;
        g_document.metadata_count++;
    }

    /* Copy layer (truncate if too long) */
    strncpy(meta->layer, layer, sizeof(meta->layer) - 1);
    meta->layer[sizeof(meta->layer) - 1] = '\0';

    g_document.dirty = true;
    return true;
}

const char* lc_document_get_entity_layer(lc_entity_handle_t entity)
{
    if (!lc_entity_is_valid(entity))
    {
        return EMPTY_STRING;
    }

    lc_entity_metadata_t *meta = find_metadata_slot(entity, false);
    if (meta && meta->occupied)
    {
        return meta->layer;
    }

    return EMPTY_STRING;
}

void lc_document_select_entity(lc_entity_handle_t entity)
{
    if (!lc_entity_is_valid(entity))
    {
        return;
    }

    /* Check if already selected */
    if (lc_entity_get_flags(entity) & LC_ENTITY_FLAG_SELECTED)
    {
        return;
    }

    /* Set SELECTED flag */
    uint32_t flags = lc_entity_get_flags(entity);
    lc_entity_set_flags(entity, flags | LC_ENTITY_FLAG_SELECTED);

    /* Add to selection array */
    ensure_selection_capacity();
    if (g_document.selection_count < g_document.selection_capacity)
    {
        g_document.selection[g_document.selection_count] = entity;
        g_document.selection_count++;
    }
}

void lc_document_deselect_entity(lc_entity_handle_t entity)
{
    if (!lc_entity_is_valid(entity))
    {
        return;
    }

    /* Check if selected */
    if (!(lc_entity_get_flags(entity) & LC_ENTITY_FLAG_SELECTED))
    {
        return;
    }

    /* Clear SELECTED flag */
    uint32_t flags = lc_entity_get_flags(entity);
    lc_entity_set_flags(entity, flags & ~LC_ENTITY_FLAG_SELECTED);

    /* Remove from selection array */
    size_t index = find_selection_index(entity);
    if (index < g_document.selection_count)
    {
        /* Shift remaining elements */
        for (size_t i = index; i < g_document.selection_count - 1; i++)
        {
            g_document.selection[i] = g_document.selection[i + 1];
        }
        g_document.selection_count--;

        /* Shrink if needed */
        shrink_selection_if_needed();
    }
}

void lc_document_deselect_all(void)
{
    /* Clear SELECTED flag on all selected entities */
    for (size_t i = 0; i < g_document.selection_count; i++)
    {
        lc_entity_handle_t entity = g_document.selection[i];
        if (lc_entity_is_valid(entity))
        {
            uint32_t flags = lc_entity_get_flags(entity);
            lc_entity_set_flags(entity, flags & ~LC_ENTITY_FLAG_SELECTED);
        }
    }

    /* Clear selection array */
    g_document.selection_count = 0;

    /* Shrink if needed */
    shrink_selection_if_needed();
}

bool lc_document_is_selected(lc_entity_handle_t entity)
{
    if (!lc_entity_is_valid(entity))
    {
        return false;
    }

    return (lc_entity_get_flags(entity) & LC_ENTITY_FLAG_SELECTED) != 0;
}

size_t lc_document_get_selection_count(void)
{
    return g_document.selection_count;
}

size_t lc_document_get_selected_entities(lc_entity_handle_t *out_entities,
                                         size_t max_count)
{
    if (!out_entities || max_count == 0)
    {
        return g_document.selection_count;
    }

    size_t copy_count = g_document.selection_count;
    if (copy_count > max_count)
    {
        copy_count = max_count;
    }

    memcpy(out_entities, g_document.selection,
           sizeof(lc_entity_handle_t) * copy_count);

    return g_document.selection_count;
}

void lc_document_set_visible(lc_entity_handle_t entity, bool visible)
{
    if (!lc_entity_is_valid(entity))
    {
        return;
    }

    uint32_t flags = lc_entity_get_flags(entity);
    if (visible)
    {
        flags |= LC_ENTITY_FLAG_VISIBLE;
    }
    else
    {
        flags &= ~LC_ENTITY_FLAG_VISIBLE;
    }
    lc_entity_set_flags(entity, flags);

    g_document.dirty = true;
}

bool lc_document_get_visible(lc_entity_handle_t entity)
{
    if (!lc_entity_is_valid(entity))
    {
        return true; /* Default visible */
    }

    uint32_t flags = lc_entity_get_flags(entity);

    /* Default is visible if flag not explicitly cleared */
    /* Check if entity has been initialized with flags */
    if (flags == 0)
    {
        return true;
    }

    return (flags & LC_ENTITY_FLAG_VISIBLE) != 0;
}

void lc_document_set_locked(lc_entity_handle_t entity, bool locked)
{
    if (!lc_entity_is_valid(entity))
    {
        return;
    }

    uint32_t flags = lc_entity_get_flags(entity);
    if (locked)
    {
        flags |= LC_ENTITY_FLAG_LOCKED;
    }
    else
    {
        flags &= ~LC_ENTITY_FLAG_LOCKED;
    }
    lc_entity_set_flags(entity, flags);

    g_document.dirty = true;
}

bool lc_document_get_locked(lc_entity_handle_t entity)
{
    if (!lc_entity_is_valid(entity))
    {
        return false; /* Default not locked */
    }

    uint32_t flags = lc_entity_get_flags(entity);
    return (flags & LC_ENTITY_FLAG_LOCKED) != 0;
}

lc_entity_handle_t lc_document_find_entity_by_name(const char *name)
{
    if (!name || name[0] == '\0')
    {
        return LC_ENTITY_INVALID;
    }

    /* Linear search through metadata table */
    for (size_t i = 0; i < LC_DOCUMENT_METADATA_TABLE_SIZE; i++)
    {
        if (g_document.metadata_table[i].occupied)
        {
            if (strcmp(g_document.metadata_table[i].name, name) == 0)
            {
                return g_document.metadata_table[i].key;
            }
        }
    }

    return LC_ENTITY_INVALID;
}

void lc_document_mark_dirty(void)
{
    g_document.dirty = true;
}

void lc_document_clear_dirty(void)
{
    g_document.dirty = false;
}

bool lc_document_is_dirty(void)
{
    return g_document.dirty;
}

bool lc_document_delete_entity(lc_entity_handle_t entity)
{
    if (!lc_entity_is_valid(entity))
    {
        return false;
    }

    /* Remove from selection if selected */
    if (lc_document_is_selected(entity))
    {
        lc_document_deselect_entity(entity);
    }

    /* Remove from parent (unlink from tree) */
    lc_entity_handle_t parent = lc_entity_get_parent(entity);
    if (parent != LC_ENTITY_INVALID)
    {
        lc_entity_remove_child(parent, entity);
    }

    /* Destroy entity */
    bool result = lc_entity_destroy(entity);

    if (result)
    {
        g_document.dirty = true;
    }

    return result;
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static lc_entity_metadata_t* find_metadata_slot(lc_entity_handle_t entity, bool for_insert)
{
    /* Hash function: simple modulo */
    size_t hash = HASH_ENTITY(entity);
    size_t index = hash;
    size_t probes = 0;

    /* Linear probing */
    while (probes < LC_DOCUMENT_METADATA_TABLE_SIZE)
    {
        lc_entity_metadata_t *slot = &g_document.metadata_table[index];

        /* Found matching occupied slot */
        if (slot->occupied && slot->key == entity)
        {
            return slot;
        }

        /* Found empty slot */
        if (!slot->occupied)
        {
            if (for_insert)
            {
                return slot; /* Return empty slot for insertion */
            }
            else
            {
                return NULL; /* Not found for lookup */
            }
        }

        /* Probe next slot */
        index = (index + 1) % LC_DOCUMENT_METADATA_TABLE_SIZE;
        probes++;
    }

    /* Table full or not found */
    return NULL;
}

static size_t find_selection_index(lc_entity_handle_t entity)
{
    for (size_t i = 0; i < g_document.selection_count; i++)
    {
        if (g_document.selection[i] == entity)
        {
            return i;
        }
    }
    return g_document.selection_count; /* Not found */
}

static void ensure_selection_capacity(void)
{
    if (g_document.selection_count >= g_document.selection_capacity)
    {
        /* Grow by 2x */
        size_t new_capacity = g_document.selection_capacity * 2;
        if (new_capacity == 0)
        {
            new_capacity = SELECTION_INITIAL_CAPACITY;
        }

        lc_entity_handle_t *new_selection = (lc_entity_handle_t*)realloc(
            g_document.selection,
            sizeof(lc_entity_handle_t) * new_capacity
        );

        if (new_selection)
        {
            g_document.selection = new_selection;
            g_document.selection_capacity = new_capacity;
        }
        else
        {
            fprintf(stderr, "[lc_document] ERROR: Failed to grow selection array\n");
        }
    }
}

static void shrink_selection_if_needed(void)
{
    /* Shrink if less than 25% full and capacity > initial */
    if (g_document.selection_capacity > SELECTION_INITIAL_CAPACITY)
    {
        size_t threshold = g_document.selection_capacity / 4;
        if (g_document.selection_count < threshold)
        {
            size_t new_capacity = g_document.selection_capacity / 2;
            if (new_capacity < SELECTION_INITIAL_CAPACITY)
            {
                new_capacity = SELECTION_INITIAL_CAPACITY;
            }

            lc_entity_handle_t *new_selection = (lc_entity_handle_t*)realloc(
                g_document.selection,
                sizeof(lc_entity_handle_t) * new_capacity
            );

            if (new_selection)
            {
                g_document.selection = new_selection;
                g_document.selection_capacity = new_capacity;
            }
            /* Failure to shrink is non-fatal; just keep old capacity */
        }
    }
}
