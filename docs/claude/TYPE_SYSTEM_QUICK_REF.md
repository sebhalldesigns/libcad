# Type System Quick Reference

## Creating a New Type (3 Steps)

### 1. Header File (your_type.h)

```c
#include <types/core/object/object.h>

// Instance structure
typedef struct YourType {
    Object parent;       // MUST be first for inheritance
    // Your fields here
    int field1;
    char* field2;
} YourType;

// Class structure (vtable)
typedef struct YourTypeClass {
    ObjectClass parent_class;
    // Your virtual methods here
    void (*your_method)(YourType* self, int arg);
} YourTypeClass;

// Public API
TypeHandle your_type_get_type(void);
YourType* your_type_new(void);
void your_type_free(YourType* self);

// Your methods
void your_type_set_field1(YourType* self, int value);
void your_method_impl(YourType* self, int arg);
```

### 2. Source File (your_type.c)

```c
#include "your_type.h"

// ONE LINE to register type!
LIBCAD_DEFINE_TYPE(YourType, your_type, object_get_type())

// Setup vtable (called once)
static void your_type_class_init(YourTypeClass* klass)
{
    // Override parent methods if needed
    ObjectClass* parent = (ObjectClass*)klass;
    parent->debug_print = (void(*)(Object*))your_type_debug_print;

    // Setup your methods
    klass->your_method = your_method_impl;
}

// Initialize instance (called per object)
static void your_type_init(YourType* self)
{
    // Parent is already initialized!
    self->field1 = 0;
    self->field2 = NULL;
}

// Cleanup (called when freed)
static void your_type_finalize(YourType* self)
{
    free(self->field2);
    // Parent cleanup is automatic!
}

// Register finalizer (temporary hack - will improve later)
static void your_type_class_init_finalizer(void) __attribute__((constructor));
static void your_type_class_init_finalizer(void)
{
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        TypeHandle type = your_type_get_type();
        YourTypeClass* klass = (YourTypeClass*)type_class_peek(type);
        if (klass) {
            klass->parent_class.parent_class.instance_finalize =
                (void(*)(void*))your_type_finalize;
        }
    }
}

// Implement your methods
void your_type_set_field1(YourType* self, int value)
{
    if (!self) return;
    self->field1 = value;
}

void your_method_impl(YourType* self, int arg)
{
    if (!self) return;
    printf("Method called with arg=%d\n", arg);
}
```

### 3. That's It!

Constructor and destructor are auto-generated:
- `your_type_new()` ✅
- `your_type_free()` ✅
- `your_type_get_type()` ✅

## Common Patterns

### Calling Parent Methods

```c
void your_type_debug_print(YourType* self)
{
    // Call parent implementation first
    object_debug_print((Object*)self);

    // Add your own info
    printf("field1=%d\n", self->field1);
}
```

### Polymorphic Calls

```c
Object* obj = (Object*)your_type_new();

// Polymorphic - calls YOUR implementation!
OBJECT_DEBUG_PRINT(obj);

// Direct vtable call
obj->klass->debug_print(obj);

// Or use macro
LIBCAD_CALL(obj, debug_print);
```

### Type Checking

```c
Object* obj = ...;

if (IS_OBJECT(obj)) {
    // It's an Object (or derived)
}

if (type_is_a(your_type_get_type(), object_get_type())) {
    // YourType inherits from Object
}

// Safe downcast
if (IS_YOUR_TYPE(obj)) {
    YourType* typed = YOUR_TYPE(obj);
}
```

### Casting Helpers

```c
// Define in your header:
#define YOUR_TYPE(obj) ((YourType*)obj)
#define YOUR_TYPE_CLASS(klass) ((YourTypeClass*)klass)
#define IS_YOUR_TYPE(obj) \
    (obj && type_is_a((TypeHandle)LIBCAD_GET_CLASS(obj), your_type_get_type()))

// Upcast to parent
#define YOUR_TYPE_AS_OBJECT(yt) ((Object*)(yt))
```

## Comparison with Old System

| Task | Old Way | New Way |
|------|---------|---------|
| **Define type** | Manual `type_register()` | `LIBCAD_DEFINE_TYPE()` |
| **Create instance** | `object_create()` | `object_new()` |
| **Destroy instance** | `object_destroy()` | `object_free()` |
| **Call method** | `type_instance_call_method()` | `obj->klass->method()` |
| **Override method** | `type_register_method()` | Set in `class_init()` |
| **Register property** | `type_register_property()` | TBD (future) |
| **Set lifecycle** | `type_set_init/destroy()` | Define `_init/_finalize()` |

## Macros Reference

### LIBCAD_DEFINE_TYPE(TypeName, type_name, parent_func)

Generates:
- `TypeName* type_name_new(void)`
- `void type_name_free(TypeName* self)`
- `TypeHandle type_name_get_type(void)`
- `TypeNameClass* type_name_class_get(void)`

**Requires you to define:**
- `static void type_name_class_init(TypeNameClass* klass)`
- `static void type_name_init(TypeName* self)`

### LIBCAD_GET_CLASS(instance)

Returns the `TypeClass*` for an instance.

```c
Object* obj = ...;
ObjectClass* klass = (ObjectClass*)LIBCAD_GET_CLASS(obj);
```

### LIBCAD_CALL(instance, method, ...)

Calls a virtual method via the vtable.

```c
LIBCAD_CALL(obj, debug_print);
LIBCAD_CALL(obj, some_method, arg1, arg2);
```

### LIBCAD_CALL_IF_EXISTS(instance, method, ...)

Calls method only if it's not NULL.

```c
LIBCAD_CALL_IF_EXISTS(obj, optional_method);
```

### LIBCAD_PARENT_CLASS(klass)

Gets the parent class from a class structure.

```c
static void your_type_class_init(YourTypeClass* klass)
{
    ObjectClass* parent = (ObjectClass*)LIBCAD_PARENT_CLASS(klass);
    // or just: ObjectClass* parent = &klass->parent_class;
}
```

## Performance Tips

### Fast Path: Direct C Calls

```c
// Fastest - direct function call
object_set_name(obj, "foo");

// Still fast - one pointer dereference
obj->klass->debug_print(obj);

// Slowest - string lookup (don't use in hot paths)
// ... removed in new system!
```

### Memory Layout

```c
// Good - single allocation
Object obj = {
    .klass = &object_class,
    .name = "foo"
};

// Bad - would need multiple allocations
// ... old system did this, new system doesn't!
```

### Vtable Caching

```c
// Cache the class if calling methods in a loop
ObjectClass* klass = (ObjectClass*)LIBCAD_GET_CLASS(obj);
for (int i = 0; i < 1000; i++) {
    klass->debug_print(obj);  // Cached lookup!
}
```

## Common Mistakes

### ❌ Forgetting parent in struct

```c
typedef struct YourType {
    int field;  // ERROR: parent not first!
    Object parent;
} YourType;
```

✅ **Fix:** Parent MUST be first
```c
typedef struct YourType {
    Object parent;  // CORRECT
    int field;
} YourType;
```

### ❌ Setting klass manually

```c
static void your_type_init(YourType* self)
{
    self->parent.klass = ...; // DON'T DO THIS!
}
```

✅ **Fix:** klass is set automatically
```c
static void your_type_init(YourType* self)
{
    // klass is already set!
    self->field = 0;
}
```

### ❌ Calling parent finalize

```c
static void your_type_finalize(YourType* self)
{
    free(self->data);
    object_finalize(&self->parent);  // DON'T DO THIS!
}
```

✅ **Fix:** Parent finalize is automatic
```c
static void your_type_finalize(YourType* self)
{
    free(self->data);
    // Parent finalize called automatically!
}
```

## Need Help?

- See `TYPE_SYSTEM_COMPARISON.md` for detailed comparison
- See `USAGE_EXAMPLE.c` for working examples
- See `MIGRATION_COMPLETE.md` for migration guide
- Check the source: `lib/model/type/type.h` has detailed comments
