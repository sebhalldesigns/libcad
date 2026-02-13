# libcad Architecture

## Type System Design: Hybrid Approach

The type system uses a **dual-API design** that balances performance with flexibility:

### 1. Primary API: Normal C (Fast Path)

For everyday coding, you work with **normal C structs and functions**:

```c
// Create a document the normal way
document_t* doc = document_create();

// Direct struct access - fast, type-safe
doc->path = "/path/to/file.lcad";

// Or use setter functions
document_set_path(doc, "/path/to/file.lcad");

// Inheritance works via upcasting
object_t* obj = document_as_object(doc);
object_set_name(obj, "My Document");

// Access inherited fields directly
printf("Name: %s\n", doc->base.name);

// Clean up
document_destroy(doc);
```

**Benefits:**
- ✅ Fast (no indirection, no lookups)
- ✅ Type-safe (compiler catches errors)
- ✅ Familiar C idioms
- ✅ Easy to debug

### 2. Secondary API: Type System (Metadata Layer)

The type system provides **runtime reflection** for special cases:

```c
// Register types once at startup
object_register_type();
document_register_type();

// When you need reflection (e.g., serialization, scripting):
type_instance_handle_t instance = type_instance_create(document_get_type_handle());

// Access via property lookup (slower, but dynamic)
property_handle_t path_prop = type_get_property("path", document_get_type_handle());
const char* path = "/some/path";
type_instance_set_property(instance, path_prop, &path);

// The instance->data points to a document_t
document_t* doc = (document_t*)((type_instance_t*)instance)->data;

// Clean up
type_instance_destroy(instance);
```

**Use cases:**
- 📄 **Serialization:** Save/load documents to JSON/binary
- 🔌 **FFI:** Expose types to Python/Lua/JavaScript
- 🔍 **Introspection:** Property editors, debugging tools
- 🔄 **Generic algorithms:** Copy, compare, validate across all types

## Inheritance in C

Since C doesn't have native inheritance, we use **struct embedding**:

```c
typedef struct object_t {
    char* name;  // base fields
} object_t;

typedef struct document_t {
    object_t base;  // MUST be first member
    char* path;     // derived fields
} document_t;
```

**This enables safe upcasting:**
```c
document_t* doc = document_create();
object_t* obj = (object_t*)doc;  // Safe because base is first
```

**The type system tracks this:**
```c
type_register("document",
              object_get_type_handle(),  // parent type
              sizeof(document_t) - sizeof(object_t));  // local size only
```

## Memory Layout

```
┌─────────────────────────────────────────┐
│  document_t                             │
├─────────────────────────────────────────┤
│  object_t base:                         │
│    char* name                           │  ← offset 0
├─────────────────────────────────────────┤
│  char* path                             │  ← offset sizeof(object_t)
└─────────────────────────────────────────┘
```

Properties use **global offsets** from the start of the struct, so inherited properties work transparently.

## When to Use Each API

| Scenario | Use |
|----------|-----|
| Creating/destroying objects | ✅ Normal C API |
| Setting/getting properties | ✅ Normal C API (direct field access) |
| Method calls | ✅ Normal C functions |
| Loading from file | 🔧 Type system (property iteration) |
| Saving to file | 🔧 Type system (property introspection) |
| Python bindings | 🔧 Type system (dynamic access) |
| Property editor UI | 🔧 Type system (enumerate properties) |
| Undo/redo system | 🔧 Type system (generic property get/set) |

## Summary

- **Write code using the normal C API** - it's fast, safe, and ergonomic
- **The type system is metadata** - it describes your structs for reflection
- **Both APIs work on the same data** - `type_instance_t::data` points to your struct
- **Register types once at startup** - then use normal C for everything else
