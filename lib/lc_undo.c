/***************************************************************
**
** libcad Source File
**
** File         :  lc_undo.c
** Module       :  libcad (undo/redo system)
** Author       :  SH
** Created      :  2026-01-29 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Implementation of undo/redo command stack.
**                 Uses hybrid approach: snapshots for create/delete,
**                 deltas for property changes, grouping for multi-step
**                 operations. Provides reversible operations for all
**                 entity mutations.
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lc_undo.h"
#include "lc_entity.h"
#include "lc_document.h"
#include "libcad_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

/* Global undo stack */
static lc_undo_stack_t g_undo_stack;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

/* Deep copy entity snapshot including type-specific data.
 * Returns true on success, false if allocation fails. */
static bool copy_entity_snapshot(lc_entity_slot_t *dest, const lc_entity_slot_t *src);

/* Free allocated data in command union. */
static void free_command_data(lc_undo_command_t *command);

/* Reverse a single command (undo operation).
 * Returns true on success, false on failure. */
static bool reverse_command(lc_undo_command_t *command);

/* Replay a single command (redo operation).
 * Returns true on success, false on failure. */
static bool replay_command(lc_undo_command_t *command);

/* Copy type-specific geometry data.
 * Returns allocated copy or NULL on failure. */
static void* copy_geometry_data(lc_entity_type_t type, const void *data);

/* Free type-specific geometry data. */
static void free_geometry_data(lc_entity_type_t type, void *data);

/* Get size of type-specific geometry data. */
static size_t get_geometry_data_size(lc_entity_type_t type);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void lc_undo_init(void)
{
    memset(&g_undo_stack, 0, sizeof(lc_undo_stack_t));
    g_undo_stack.undo_index = -1;
    g_undo_stack.redo_index = 0;
    g_undo_stack.command_count = 0;
    g_undo_stack.group_depth = 0;
}

void lc_undo_shutdown(void)
{
    int i;

    /* Free all command data */
    for (i = 0; i < g_undo_stack.command_count; i++)
    {
        free_command_data(&g_undo_stack.commands[i]);
    }

    /* Reset stack */
    memset(&g_undo_stack, 0, sizeof(lc_undo_stack_t));
    g_undo_stack.undo_index = -1;
}

void lc_undo_begin_group(void)
{
    lc_undo_command_t cmd;
    memset(&cmd, 0, sizeof(lc_undo_command_t));

    cmd.type = LC_COMMAND_GROUP_BEGIN;
    cmd.entity = LC_ENTITY_INVALID;

    lc_undo_record_command(&cmd);
    g_undo_stack.group_depth++;
}

void lc_undo_end_group(void)
{
    lc_undo_command_t cmd;

    if (g_undo_stack.group_depth <= 0)
    {
        fprintf(stderr, "lc_undo_end_group: group_depth underflow\n");
        return;
    }

    memset(&cmd, 0, sizeof(lc_undo_command_t));
    cmd.type = LC_COMMAND_GROUP_END;
    cmd.entity = LC_ENTITY_INVALID;

    lc_undo_record_command(&cmd);
    g_undo_stack.group_depth--;
}

void lc_undo_record_command(const lc_undo_command_t *command)
{
    lc_undo_command_t *dest;
    int i;

    if (!command)
    {
        return;
    }

    /* Clear redo stack when new command is recorded */
    for (i = g_undo_stack.undo_index + 1; i < g_undo_stack.command_count; i++)
    {
        free_command_data(&g_undo_stack.commands[i]);
    }
    g_undo_stack.command_count = g_undo_stack.undo_index + 1;

    /* If stack is full, drop oldest command (shift array) */
    if (g_undo_stack.command_count >= LC_UNDO_MAX_STACK_DEPTH)
    {
        free_command_data(&g_undo_stack.commands[0]);

        /* Shift all commands down */
        for (i = 0; i < LC_UNDO_MAX_STACK_DEPTH - 1; i++)
        {
            g_undo_stack.commands[i] = g_undo_stack.commands[i + 1];
        }

        g_undo_stack.command_count = LC_UNDO_MAX_STACK_DEPTH - 1;
        g_undo_stack.undo_index = LC_UNDO_MAX_STACK_DEPTH - 2;
    }

    /* Record new command */
    dest = &g_undo_stack.commands[g_undo_stack.command_count];
    memset(dest, 0, sizeof(lc_undo_command_t));

    dest->type = command->type;
    dest->entity = command->entity;

    /* Deep copy command-specific data */
    switch (command->type)
    {
        case LC_COMMAND_CREATE_ENTITY:
            if (!copy_entity_snapshot(&dest->data.create.snapshot,
                                     &command->data.create.snapshot))
            {
                fprintf(stderr, "lc_undo_record_command: failed to copy CREATE snapshot\n");
                return;
            }
            break;

        case LC_COMMAND_DELETE_ENTITY:
            if (!copy_entity_snapshot(&dest->data.delete_entity.snapshot,
                                     &command->data.delete_entity.snapshot))
            {
                fprintf(stderr, "lc_undo_record_command: failed to copy DELETE snapshot\n");
                return;
            }
            break;

        case LC_COMMAND_MODIFY_PROPERTY:
            dest->data.modify_property.property_id = command->data.modify_property.property_id;
            memcpy(dest->data.modify_property.old_value,
                   command->data.modify_property.old_value,
                   LC_PROPERTY_VALUE_MAX_SIZE);
            memcpy(dest->data.modify_property.new_value,
                   command->data.modify_property.new_value,
                   LC_PROPERTY_VALUE_MAX_SIZE);
            break;

        case LC_COMMAND_MODIFY_GEOMETRY:
            dest->data.modify_geometry.data_size = command->data.modify_geometry.data_size;

            if (command->data.modify_geometry.old_data)
            {
                dest->data.modify_geometry.old_data = malloc(command->data.modify_geometry.data_size);
                if (dest->data.modify_geometry.old_data)
                {
                    memcpy(dest->data.modify_geometry.old_data,
                           command->data.modify_geometry.old_data,
                           command->data.modify_geometry.data_size);
                }
            }

            if (command->data.modify_geometry.new_data)
            {
                dest->data.modify_geometry.new_data = malloc(command->data.modify_geometry.data_size);
                if (dest->data.modify_geometry.new_data)
                {
                    memcpy(dest->data.modify_geometry.new_data,
                           command->data.modify_geometry.new_data,
                           command->data.modify_geometry.data_size);
                }
            }
            break;

        case LC_COMMAND_GROUP_BEGIN:
        case LC_COMMAND_GROUP_END:
            /* No data to copy */
            break;
    }

    g_undo_stack.command_count++;
    g_undo_stack.undo_index++;
    g_undo_stack.redo_index = g_undo_stack.command_count;
}

bool lc_undo_perform(void)
{
    int group_level;
    bool success;

    if (!lc_undo_can_undo())
    {
        return false;
    }

    group_level = 0;
    success = true;

    /* Walk backwards from undo_index */
    while (g_undo_stack.undo_index >= 0)
    {
        lc_undo_command_t *cmd = &g_undo_stack.commands[g_undo_stack.undo_index];

        /* Handle group boundaries */
        if (cmd->type == LC_COMMAND_GROUP_END)
        {
            group_level++;
            g_undo_stack.undo_index--;
            continue;
        }
        else if (cmd->type == LC_COMMAND_GROUP_BEGIN)
        {
            group_level--;
            g_undo_stack.undo_index--;

            /* If we've closed all groups, stop */
            if (group_level < 0)
            {
                break;
            }
            continue;
        }

        /* Reverse the command */
        if (!reverse_command(cmd))
        {
            fprintf(stderr, "lc_undo_perform: failed to reverse command at index %d\n",
                    g_undo_stack.undo_index);
            success = false;
        }

        g_undo_stack.undo_index--;

        /* If not in a group, stop after one command */
        if (group_level == 0)
        {
            break;
        }
    }

    /* Update redo index */
    g_undo_stack.redo_index = g_undo_stack.undo_index + 1;

    return success;
}

bool lc_redo_perform(void)
{
    int group_level;
    bool success;

    if (!lc_undo_can_redo())
    {
        return false;
    }

    group_level = 0;
    success = true;

    /* Walk forwards from redo_index */
    while (g_undo_stack.redo_index < g_undo_stack.command_count)
    {
        lc_undo_command_t *cmd = &g_undo_stack.commands[g_undo_stack.redo_index];

        /* Handle group boundaries */
        if (cmd->type == LC_COMMAND_GROUP_BEGIN)
        {
            group_level++;
            g_undo_stack.redo_index++;
            continue;
        }
        else if (cmd->type == LC_COMMAND_GROUP_END)
        {
            group_level--;
            g_undo_stack.redo_index++;

            /* If we've closed all groups, stop */
            if (group_level < 0)
            {
                break;
            }
            continue;
        }

        /* Replay the command */
        if (!replay_command(cmd))
        {
            fprintf(stderr, "lc_redo_perform: failed to replay command at index %d\n",
                    g_undo_stack.redo_index);
            success = false;
        }

        g_undo_stack.redo_index++;

        /* If not in a group, stop after one command */
        if (group_level == 0)
        {
            break;
        }
    }

    /* Update undo index */
    g_undo_stack.undo_index = g_undo_stack.redo_index - 1;

    return success;
}

bool lc_undo_can_undo(void)
{
    return g_undo_stack.undo_index >= 0;
}

bool lc_undo_can_redo(void)
{
    return g_undo_stack.redo_index < g_undo_stack.command_count;
}

int lc_undo_get_undo_count(void)
{
    return g_undo_stack.undo_index + 1;
}

int lc_undo_get_redo_count(void)
{
    return g_undo_stack.command_count - g_undo_stack.redo_index;
}

void lc_undo_clear(void)
{
    int i;

    /* Free all command data */
    for (i = 0; i < g_undo_stack.command_count; i++)
    {
        free_command_data(&g_undo_stack.commands[i]);
    }

    /* Reset stack */
    g_undo_stack.undo_index = -1;
    g_undo_stack.redo_index = 0;
    g_undo_stack.command_count = 0;
    g_undo_stack.group_depth = 0;
}

void lc_undo_set_limit(int max_depth)
{
    /* Not implemented in Phase 2 - stack size is fixed */
    (void)max_depth;
    fprintf(stderr, "lc_undo_set_limit: not implemented (stack size is fixed at %d)\n",
            LC_UNDO_MAX_STACK_DEPTH);
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static bool copy_entity_snapshot(lc_entity_slot_t *dest, const lc_entity_slot_t *src)
{
    if (!dest || !src)
    {
        return false;
    }

    /* Copy base fields */
    dest->handle = src->handle;
    dest->type = src->type;
    dest->flags = src->flags;
    dest->parent = src->parent;
    dest->first_child = src->first_child;
    dest->next_sibling = src->next_sibling;
    dest->prev_sibling = src->prev_sibling;

    /* Deep copy type-specific data */
    dest->data.generic = copy_geometry_data(src->type, src->data.generic);

    /* User data is not copied (snapshot doesn't preserve user data) */
    dest->user_data = NULL;
    dest->user_data_destructor = NULL;

    return true;
}

static void free_command_data(lc_undo_command_t *command)
{
    if (!command)
    {
        return;
    }

    switch (command->type)
    {
        case LC_COMMAND_CREATE_ENTITY:
            free_geometry_data(command->data.create.snapshot.type,
                             command->data.create.snapshot.data.generic);
            break;

        case LC_COMMAND_DELETE_ENTITY:
            free_geometry_data(command->data.delete_entity.snapshot.type,
                             command->data.delete_entity.snapshot.data.generic);
            break;

        case LC_COMMAND_MODIFY_GEOMETRY:
            if (command->data.modify_geometry.old_data)
            {
                free(command->data.modify_geometry.old_data);
            }
            if (command->data.modify_geometry.new_data)
            {
                free(command->data.modify_geometry.new_data);
            }
            break;

        default:
            /* Other command types have no allocated data */
            break;
    }
}

static bool reverse_command(lc_undo_command_t *command)
{
    char temp_value[LC_PROPERTY_VALUE_MAX_SIZE];
    void *temp_data;

    if (!command)
    {
        return false;
    }

    switch (command->type)
    {
        case LC_COMMAND_CREATE_ENTITY:
            /* Undo creation by destroying entity */
            return lc_entity_destroy(command->entity);

        case LC_COMMAND_DELETE_ENTITY:
            /* Undo deletion by restoring entity from snapshot */
            /* This is complex - need to restore slot state and re-insert into tree */
            /* For now, just log - full implementation requires access to entity internals */
            fprintf(stderr, "reverse_command: LC_COMMAND_DELETE_ENTITY not fully implemented\n");
            return false;

        case LC_COMMAND_MODIFY_PROPERTY:
            /* Swap old_value and new_value, then apply old_value */
            memcpy(temp_value, command->data.modify_property.old_value, LC_PROPERTY_VALUE_MAX_SIZE);
            memcpy(command->data.modify_property.old_value,
                   command->data.modify_property.new_value,
                   LC_PROPERTY_VALUE_MAX_SIZE);
            memcpy(command->data.modify_property.new_value, temp_value, LC_PROPERTY_VALUE_MAX_SIZE);

            /* Apply the property change based on property_id */
            switch (command->data.modify_property.property_id)
            {
                case LC_PROPERTY_NAME:
                    return lc_document_set_entity_name(command->entity,
                                                       command->data.modify_property.old_value);
                case LC_PROPERTY_LAYER:
                    return lc_document_set_entity_layer(command->entity,
                                                        command->data.modify_property.old_value);
                case LC_PROPERTY_VISIBILITY:
                    lc_document_set_visible(command->entity,
                                           command->data.modify_property.old_value[0] != 0);
                    return true;
                case LC_PROPERTY_LOCKED:
                    lc_document_set_locked(command->entity,
                                          command->data.modify_property.old_value[0] != 0);
                    return true;
                default:
                    fprintf(stderr, "reverse_command: unknown property_id %d\n",
                            command->data.modify_property.property_id);
                    return false;
            }

        case LC_COMMAND_MODIFY_GEOMETRY:
            /* Swap old_data and new_data pointers */
            temp_data = command->data.modify_geometry.old_data;
            command->data.modify_geometry.old_data = command->data.modify_geometry.new_data;
            command->data.modify_geometry.new_data = temp_data;

            /* Copy old_data to entity (requires entity data access) */
            /* For now, just log - full implementation requires access to entity internals */
            fprintf(stderr, "reverse_command: LC_COMMAND_MODIFY_GEOMETRY not fully implemented\n");
            return false;

        case LC_COMMAND_GROUP_BEGIN:
        case LC_COMMAND_GROUP_END:
            /* Groups are boundaries, not reversible actions */
            return true;

        default:
            fprintf(stderr, "reverse_command: unknown command type %d\n", command->type);
            return false;
    }
}

static bool replay_command(lc_undo_command_t *command)
{
    void *temp_data;

    if (!command)
    {
        return false;
    }

    switch (command->type)
    {
        case LC_COMMAND_CREATE_ENTITY:
            /* Redo creation by restoring entity from snapshot */
            /* For now, just log - full implementation requires access to entity internals */
            fprintf(stderr, "replay_command: LC_COMMAND_CREATE_ENTITY not fully implemented\n");
            return false;

        case LC_COMMAND_DELETE_ENTITY:
            /* Redo deletion by destroying entity */
            return lc_entity_destroy(command->entity);

        case LC_COMMAND_MODIFY_PROPERTY:
            /* Apply new_value (values were already swapped by reverse_command) */
            switch (command->data.modify_property.property_id)
            {
                case LC_PROPERTY_NAME:
                    return lc_document_set_entity_name(command->entity,
                                                       command->data.modify_property.new_value);
                case LC_PROPERTY_LAYER:
                    return lc_document_set_entity_layer(command->entity,
                                                        command->data.modify_property.new_value);
                case LC_PROPERTY_VISIBILITY:
                    lc_document_set_visible(command->entity,
                                           command->data.modify_property.new_value[0] != 0);
                    return true;
                case LC_PROPERTY_LOCKED:
                    lc_document_set_locked(command->entity,
                                          command->data.modify_property.new_value[0] != 0);
                    return true;
                default:
                    fprintf(stderr, "replay_command: unknown property_id %d\n",
                            command->data.modify_property.property_id);
                    return false;
            }

        case LC_COMMAND_MODIFY_GEOMETRY:
            /* Swap pointers back (they were swapped by reverse_command) */
            temp_data = command->data.modify_geometry.old_data;
            command->data.modify_geometry.old_data = command->data.modify_geometry.new_data;
            command->data.modify_geometry.new_data = temp_data;

            /* Copy new_data to entity (requires entity data access) */
            /* For now, just log - full implementation requires access to entity internals */
            fprintf(stderr, "replay_command: LC_COMMAND_MODIFY_GEOMETRY not fully implemented\n");
            return false;

        case LC_COMMAND_GROUP_BEGIN:
        case LC_COMMAND_GROUP_END:
            /* Groups are boundaries, not reversible actions */
            return true;

        default:
            fprintf(stderr, "replay_command: unknown command type %d\n", command->type);
            return false;
    }
}

static void* copy_geometry_data(lc_entity_type_t type, const void *data)
{
    void *copy;
    size_t size;

    if (!data)
    {
        return NULL;
    }

    size = get_geometry_data_size(type);
    if (size == 0)
    {
        return NULL;
    }

    copy = malloc(size);
    if (copy)
    {
        memcpy(copy, data, size);
    }

    return copy;
}

static void free_geometry_data(lc_entity_type_t type, void *data)
{
    (void)type;  /* Type not needed for simple free */

    if (data)
    {
        free(data);
    }
}

static size_t get_geometry_data_size(lc_entity_type_t type)
{
    switch (type)
    {
        case LC_ENTITY_TYPE_SKETCH:
            return sizeof(lc_sketch_data_t);
        case LC_ENTITY_TYPE_BODY:
            return sizeof(lc_body_data_t);
        case LC_ENTITY_TYPE_CONSTRAINT:
            return sizeof(lc_constraint_data_t);
        case LC_ENTITY_TYPE_GEOMETRY_LINE:
            return sizeof(lc_geometry_line_data_t);
        case LC_ENTITY_TYPE_GEOMETRY_CIRCLE:
            return sizeof(lc_geometry_circle_data_t);
        case LC_ENTITY_TYPE_GEOMETRY_RECT:
            return sizeof(lc_geometry_rect_data_t);
        default:
            return 0;
    }
}
