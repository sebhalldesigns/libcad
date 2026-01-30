/***************************************************************
**
** libcad Header File
**
** File         :  lc_undo.h
** Module       :  libcad (undo/redo system)
** Author       :  SH
** Created      :  2026-01-29 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Undo/redo command stack with hybrid approach:
**                 immutable snapshots for entity creation/deletion,
**                 delta commands for property changes, and command
**                 grouping for multi-step operations.
**
***************************************************************/

#ifndef LC_UNDO_H
#define LC_UNDO_H

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

/* Maximum undo/redo stack depth */
#define LC_UNDO_MAX_STACK_DEPTH 100

/* Maximum size for property value serialization */
#define LC_PROPERTY_VALUE_MAX_SIZE 256

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

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
    LC_PROPERTY_CONSTRAINT_VALUE,  /* For constraint distance/angle parameters */
} lc_property_id_t;

/* Undo command stores enough information to reverse an operation.
 * Commands are immutable once recorded. */
typedef struct lc_undo_command_t
{
    lc_command_type_t type;
    lc_entity_handle_t entity;  /* Affected entity */

    /* Command-specific data */
    union
    {
        /* LC_COMMAND_CREATE_ENTITY: save full entity snapshot */
        struct
        {
            lc_entity_slot_t snapshot;  /* Copy of slot at creation time */
        } create;

        /* LC_COMMAND_DELETE_ENTITY: save full entity snapshot */
        struct
        {
            lc_entity_slot_t snapshot;  /* Copy of slot before deletion */
        } delete_entity;

        /* LC_COMMAND_MODIFY_PROPERTY: save old and new values */
        struct
        {
            lc_property_id_t property_id;
            char old_value[LC_PROPERTY_VALUE_MAX_SIZE];  /* Serialized old value */
            char new_value[LC_PROPERTY_VALUE_MAX_SIZE];  /* Serialized new value */
        } modify_property;

        /* LC_COMMAND_MODIFY_GEOMETRY: save old and new geometry data */
        struct
        {
            void *old_data;  /* Deep copy of old geometry (malloc'd) */
            void *new_data;  /* Deep copy of new geometry (malloc'd) */
            size_t data_size;
        } modify_geometry;

    } data;

} lc_undo_command_t;

/* Undo/redo stack.
 * Single global instance in lc_undo.c. */
typedef struct lc_undo_stack_t
{
    lc_undo_command_t commands[LC_UNDO_MAX_STACK_DEPTH];
    int undo_index;  /* Index of next command to undo (-1 if none) */
    int redo_index;  /* Index of next command to redo (command_count if none) */
    int command_count;  /* Total commands in stack */

    /* Group nesting depth (for LC_COMMAND_GROUP_BEGIN/END) */
    int group_depth;

} lc_undo_stack_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

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

/* Get number of undo steps available. */
int lc_undo_get_undo_count(void);

/* Get number of redo steps available. */
int lc_undo_get_redo_count(void);

/* Clear undo/redo stacks (e.g., after loading a new document). */
void lc_undo_clear(void);

/* Set maximum stack depth (default LC_UNDO_MAX_STACK_DEPTH).
 * Note: Not implemented in Phase 2 (stack size is fixed). */
void lc_undo_set_limit(int max_depth);

#ifdef __cplusplus
}
#endif

#endif /* LC_UNDO_H */
