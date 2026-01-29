/***************************************************************
**
** libcad Header File
**
** File         :  lc_document.h
** Module       :  libcad (document system)
** Author       :  SH
** Created      :  2026-01-29 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Document tree management, metadata storage, and
**                 selection state. Provides high-level operations
**                 on the entity system including visibility, locking,
**                 and hierarchical organization.
**
***************************************************************/

#ifndef LC_DOCUMENT_H
#define LC_DOCUMENT_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lc_entity.h"
#include <stddef.h>
#include <stdbool.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/* Metadata hash table capacity */
#define LC_DOCUMENT_METADATA_TABLE_SIZE 1024

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/* Document metadata attached to each entity.
 * Stored in a hash table keyed by entity handle.
 * Separation from entity slots saves memory for entities without metadata. */
typedef struct lc_entity_metadata_t
{
    char name[64];        /* User-visible name */
    char layer[32];       /* Layer/group name */
    int reference_count;  /* For geometry sharing (future) */
    lc_entity_handle_t key; /* Entity handle (for hash table) */
    bool occupied;        /* True if this slot is in use */
} lc_entity_metadata_t;

/* Document state.
 * Single global instance in lc_document.c. */
typedef struct lc_document_t
{
    lc_entity_handle_t root_assembly;  /* Root of document tree */

    /* Metadata table: open-addressed hash table */
    lc_entity_metadata_t metadata_table[LC_DOCUMENT_METADATA_TABLE_SIZE];
    size_t metadata_count;

    /* Selection state */
    lc_entity_handle_t *selection;  /* Dynamic array of selected handles */
    size_t selection_count;
    size_t selection_capacity;

    /* Document dirty flag (for save prompts) */
    bool dirty;

} lc_document_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

/* Initialize document system.
 * Creates root assembly entity. */
void lc_document_init(void);

/* Shutdown document system and free all resources. */
void lc_document_shutdown(void);

/* Get root assembly entity.
 * All top-level bodies and sketches are children of this entity. */
lc_entity_handle_t lc_document_get_root(void);

/* Metadata operations */

/* Set entity name. Allocates metadata entry if needed.
 * Returns true on success, false if handle is invalid. */
bool lc_document_set_entity_name(lc_entity_handle_t entity, const char *name);

/* Get entity name. Returns "" if no name set or handle invalid.
 * Returned pointer is valid until next set_entity_name call. */
const char* lc_document_get_entity_name(lc_entity_handle_t entity);

/* Set entity layer. Allocates metadata entry if needed.
 * Returns true on success, false if handle is invalid. */
bool lc_document_set_entity_layer(lc_entity_handle_t entity, const char *layer);

/* Get entity layer. Returns "" if no layer set or handle invalid.
 * Returned pointer is valid until next set_entity_layer call. */
const char* lc_document_get_entity_layer(lc_entity_handle_t entity);

/* Selection operations */

/* Add entity to selection. Sets SELECTED flag.
 * No-op if already selected. */
void lc_document_select_entity(lc_entity_handle_t entity);

/* Remove entity from selection. Clears SELECTED flag.
 * No-op if not selected. */
void lc_document_deselect_entity(lc_entity_handle_t entity);

/* Clear all selections. Clears SELECTED flag on all entities. */
void lc_document_deselect_all(void);

/* Check if entity is selected. */
bool lc_document_is_selected(lc_entity_handle_t entity);

/* Get number of selected entities. */
size_t lc_document_get_selection_count(void);

/* Get selected entities array.
 * Copies selection array to out_entities (up to max_count).
 * Returns actual selection count (may exceed max_count).
 * Returns NULL and sets *out_count = 0 if no selection. */
size_t lc_document_get_selected_entities(lc_entity_handle_t *out_entities,
                                         size_t max_count);

/* Visibility and locking */

/* Set entity visible flag. */
void lc_document_set_visible(lc_entity_handle_t entity, bool visible);

/* Get entity visible flag. Returns true by default. */
bool lc_document_get_visible(lc_entity_handle_t entity);

/* Set entity locked flag. */
void lc_document_set_locked(lc_entity_handle_t entity, bool locked);

/* Get entity locked flag. Returns false by default. */
bool lc_document_get_locked(lc_entity_handle_t entity);

/* Query operations */

/* Find entity by name. Returns first match or LC_ENTITY_INVALID if not found.
 * Linear search through metadata table. */
lc_entity_handle_t lc_document_find_entity_by_name(const char *name);

/* Dirty flag management */

/* Mark document as modified. */
void lc_document_mark_dirty(void);

/* Clear document dirty flag. */
void lc_document_clear_dirty(void);

/* Check if document has unsaved changes. */
bool lc_document_is_dirty(void);

/* Internal function: delete entity (called by libcad.c) */
bool lc_document_delete_entity(lc_entity_handle_t entity);

#ifdef __cplusplus
}
#endif

#endif /* LC_DOCUMENT_H */
