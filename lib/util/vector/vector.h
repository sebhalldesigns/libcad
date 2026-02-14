/***************************************************************
**
** libcad Header File
**
** File         :  vector.h
** Module       :  util/vector
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Generic dynamic array (vector) implementation
**
***************************************************************/

#ifndef LIBCAD_UTIL_VECTOR_H
#define LIBCAD_UTIL_VECTOR_H

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <util/log/log.h>

/***************************************************************
** MARK: CONSTANTS
***************************************************************/

#define VECTOR_INITIAL_CAPACITY 8

/***************************************************************
** MARK: VECTOR MACROS
**
** Generic type-safe vector implementation using macros.
**
** Usage example:
**   object_t** children;
**   size_t children_count;
**   size_t children_capacity;
**
**   VECTOR_INIT(children, children_count, children_capacity, object_t*);
**   VECTOR_PUSH(children, children_count, children_capacity, object_t*, my_object);
**   VECTOR_REMOVE(children, children_count, index);
**   VECTOR_FREE(children);
***************************************************************/

/*
** Initialize a vector with initial capacity
**
** Parameters:
**   vec       - Pointer to array (e.g., self->children)
**   count     - Count variable (e.g., self->children_count)
**   capacity  - Capacity variable (e.g., self->children_capacity)
**   type      - Element type (e.g., object_t*)
*/
#define VECTOR_INIT(vec, count, capacity, type) \
    do { \
        (vec) = calloc(VECTOR_INITIAL_CAPACITY, sizeof(type)); \
        if ((vec)) { \
            (capacity) = VECTOR_INITIAL_CAPACITY; \
            (count) = 0; \
        } else { \
            log_error("VECTOR_INIT: Failed to allocate vector"); \
            (capacity) = 0; \
            (count) = 0; \
        } \
    } while (0)

/*
** Push an element to the end of the vector
** Automatically grows the vector if needed
**
** Parameters:
**   vec       - Pointer to array
**   count     - Count variable
**   capacity  - Capacity variable
**   type      - Element type
**   element   - Element to add
*/
#define VECTOR_PUSH(vec, count, capacity, type, element) \
    do { \
        /* Initialize if needed */ \
        if (!(vec)) { \
            VECTOR_INIT(vec, count, capacity, type); \
        } \
        /* Grow if needed */ \
        if ((vec) && (count) >= (capacity)) { \
            size_t new_capacity = (capacity) * 2; \
            type* new_vec = realloc((vec), new_capacity * sizeof(type)); \
            if (new_vec) { \
                /* Zero out new region */ \
                memset(&new_vec[capacity], 0, (new_capacity - (capacity)) * sizeof(type)); \
                (vec) = new_vec; \
                (capacity) = new_capacity; \
            } else { \
                log_error("VECTOR_PUSH: Failed to grow vector"); \
            } \
        } \
        /* Add element if we have space */ \
        if ((vec) && (count) < (capacity)) { \
            (vec)[(count)++] = (element); \
        } \
    } while (0)

/*
** Remove an element at the specified index
** Shifts remaining elements down
**
** Parameters:
**   vec       - Pointer to array
**   count     - Count variable
**   index     - Index to remove
*/
#define VECTOR_REMOVE(vec, count, index) \
    do { \
        if ((vec) && (index) < (count)) { \
            /* Shift remaining elements */ \
            for (size_t i = (index); i < (count) - 1; i++) { \
                (vec)[i] = (vec)[i + 1]; \
            } \
            (count)--; \
            (vec)[(count)] = NULL; \
        } \
    } while (0)

/*
** Get an element at the specified index
** Returns NULL if index is out of bounds
**
** Parameters:
**   vec       - Pointer to array
**   count     - Count variable
**   index     - Index to get
**
** Returns:
**   Element at index, or NULL if out of bounds
*/
#define VECTOR_GET(vec, count, index) \
    (((vec) && (index) < (count)) ? (vec)[index] : NULL)

/*
** Free the vector array
** Does not free individual elements
**
** Parameters:
**   vec       - Pointer to array
*/
#define VECTOR_FREE(vec) \
    do { \
        free(vec); \
        (vec) = NULL; \
    } while (0)

/*
** Clear all elements from the vector
** Keeps the allocated memory
**
** Parameters:
**   vec       - Pointer to array
**   count     - Count variable
*/
#define VECTOR_CLEAR(vec, count) \
    do { \
        if (vec) { \
            (count) = 0; \
        } \
    } while (0)

#endif /* LIBCAD_UTIL_VECTOR_H */
