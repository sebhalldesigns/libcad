# Method Pattern: Normal Functions + Type System Methods

## The Pattern

For any method you want to be polymorphic (overridable), implement **both**:

1. **Normal C function** - for direct calls (fast path)
2. **Type system method** - for dynamic dispatch (polymorphism)

## Example: `debug_print`

### In object.c (base class):

```c
// 1. Normal C function - use this in everyday code
void object_debug_print(const object_t* obj)
{
    if (!obj) return;
    log_info("Object: name='%s'", obj->name ? obj->name : "(null)");
}

// 2. Method wrapper - bridges to type system
static void object_debug_print_method(type_instance_t* self, void* args, void* result)
{
    object_t* obj = (object_t*)self->data;
    object_debug_print(obj);  // Call the C function
}

// 3. Register in object_register_type():
type_register_method(object_type_handle, "debug_print", object_debug_print_method);
```

### In document.c (derived class):

```c
// 1. Override the C function
void document_debug_print(const document_t* doc)
{
    if (!doc) return;
    log_info("Document: name='%s', path='%s'",
             doc->base.name ? doc->base.name : "(null)",
             doc->path ? doc->path : "(null)");
}

// 2. Override the method wrapper
static void document_debug_print_method(type_instance_t* self, void* args, void* result)
{
    document_t* doc = (document_t*)self->data;
    document_debug_print(doc);  // Call the C function
}

// 3. Override in document_register_type():
type_register_method(document_type_handle, "debug_print", document_debug_print_method);
```

## Usage: Two Ways to Call

### Way 1: Direct C Function (Fast - use 99% of the time)

```c
document_t* doc = document_create();
document_set_path(doc, "/path/to/file");

// Direct call - compile-time dispatch, no overhead
document_debug_print(doc);  // Calls document version

// Upcast and call base version if you want
object_t* obj = document_as_object(doc);
object_debug_print(obj);  // Calls object version
```

### Way 2: Type System Method (Polymorphic - use for generic code)

```c
// Suppose you have an array of mixed types
type_instance_handle_t instances[3];
instances[0] = type_instance_create(object_get_type_handle());
instances[1] = type_instance_create(document_get_type_handle());
instances[2] = type_instance_create(document_get_type_handle());

// Polymorphic call - runtime dispatch, finds the right implementation
method_handle_t debug_print = type_get_method("debug_print", object_get_type_handle());
for (int i = 0; i < 3; i++) {
    type_instance_call_method(instances[i], debug_print, NULL, NULL);
    // Calls object_debug_print for instance[0]
    // Calls document_debug_print for instance[1] and [2]
}
```

## When to Use Each

| Scenario | Use |
|----------|-----|
| You know the exact type | ✅ Direct C function (`document_debug_print(doc)`) |
| Generic container of mixed types | 🔧 Type system method |
| Scripting/FFI callback | 🔧 Type system method |
| Undo/redo system | 🔧 Type system method (generic operation replay) |
| Normal application code | ✅ Direct C function |

## Benefits

- ✅ **Fast path is fast** - Direct C calls have zero overhead
- ✅ **Polymorphism when needed** - Type system enables virtual dispatch
- ✅ **Code reuse** - The method wrapper just calls the C function
- ✅ **Both APIs work** - Can mix and match as needed

## Summary

**The pattern is:**
1. Write a normal C function (the real implementation)
2. Write a thin wrapper for the type system (just extracts data and calls #1)
3. Register the wrapper as a method
4. Use direct C calls in normal code, type system when you need polymorphism
