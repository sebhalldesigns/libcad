/***************************************************************
**
** libcad Header File
**
** File         :  sketch.h
** Module       :  sketch/sketch
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad sketch type - 2D drawing container
**
***************************************************************/

#ifndef LIBCAD_SKETCH_SKETCH_H
#define LIBCAD_SKETCH_SKETCH_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <types/core/object/object.h>
#include <stdint.h>

/***************************************************************
** MARK: FORWARD DECLARATIONS
***************************************************************/

typedef struct sketch_t sketch_t;
typedef struct sketch_class_t sketch_class_t;
typedef struct plane_t plane_t;

/***************************************************************
** MARK: TYPE DEFINITIONS
***************************************************************/

/*
** sketch_t instance - container for 2D sketch entities
**
** A sketch contains 2D geometric primitives (lines, circles, etc.)
** and is typically attached to a plane.
** Children are managed via inherited object_t tree structure.
**
** IMPORTANT: First field MUST be parent (object_t parent)
*/
typedef struct sketch_t {
    object_t parent;  /* Inherit from object_t (MUST be first!) */

    /* Sketch state */
    bool active;      /* Whether this sketch is currently being edited */
    plane_t* reference_plane; /* Non-owning pointer to sketch reference plane */
} sketch_t;

/*
** sketch_t class - vtable and metadata
*/
typedef struct sketch_class_t {
    object_class_t parent_class;  /* Inherit parent vtable */

    /* Virtual methods specific to sketch_t */
    /* (using inherited add_child/remove_child for entities) */
} sketch_class_t;

/***************************************************************
** MARK: TYPE SYSTEM
***************************************************************/

type_handle_t sketch_get_type(void);
sketch_class_t* sketch_class_get(void);

/***************************************************************
** MARK: CONSTRUCTORS
***************************************************************/

sketch_t* sketch_new(void);
void sketch_free(sketch_t* self);

/***************************************************************
** MARK: PUBLIC API
***************************************************************/

/* Set whether sketch is being actively edited */
void sketch_set_active(sketch_t* self, bool active);

/* Check if sketch is active */
bool sketch_is_active(const sketch_t* self);

/* Set/get the sketch reference plane (non-owning) */
void sketch_set_reference_plane(sketch_t* self, plane_t* plane);
plane_t* sketch_get_reference_plane(const sketch_t* self);
uint32_t sketch_get_reference_plane_entity_id(const sketch_t* self);

/***************************************************************
** MARK: OVERRIDDEN METHODS
***************************************************************/

void sketch_debug_print(sketch_t* self);
json_t* sketch_to_json(sketch_t* self);

/***************************************************************
** MARK: CONVENIENCE MACROS
***************************************************************/

#define SKETCH_TYPE (sketch_get_type())
#define IS_SKETCH(obj) (type_instance_is_a((void*)(obj), SKETCH_TYPE))
#define SKETCH(obj) ((sketch_t*)(obj))
#define SKETCH_AS_OBJECT(sketch) ((object_t*)(sketch))

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_SKETCH_SKETCH_H */
