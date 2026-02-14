/***************************************************************
** Usage Example - New Type System V2
**
** This shows how easy it is to use the new system
***************************************************************/

#include <types/core/object/object_v2.h>
#include <types/core/document/document_v2.h>

int main(void)
{
    /*
    ** BASIC USAGE
    */

    // Create instances - auto-generated constructors
    Object* obj = object_new();
    Document* doc = document_new();

    // Set properties - direct C API (fast!)
    object_set_name(obj, "My Object");
    object_set_name(DOCUMENT_AS_OBJECT(doc), "My Document");
    document_set_path(doc, "/path/to/file.cad");

    // Call methods directly (fast path)
    object_debug_print(obj);
    document_debug_print(doc);

    /*
    ** POLYMORPHISM
    */

    // Document IS-A Object (safe upcast)
    Object* doc_as_obj = DOCUMENT_AS_OBJECT(doc);

    // Polymorphic call - uses vtable
    // Calls document_debug_print() because that's what's in the vtable!
    OBJECT_DEBUG_PRINT(doc_as_obj);

    // Polymorphic serialization
    json_t* json1 = OBJECT_TO_JSON(obj);       // Calls object_to_json()
    json_t* json2 = OBJECT_TO_JSON(doc_as_obj); // Calls document_to_json()!

    /*
    ** RUNTIME TYPE CHECKING
    */

    if (IS_DOCUMENT(doc_as_obj)) {
        printf("It's a document!\n");
        // Safe downcast
        Document* doc_again = DOCUMENT(doc_as_obj);
        document_add_child(doc_again, obj);
    }

    /*
    ** INHERITANCE IN ACTION
    */

    // Add object as child to document
    document_add_child(doc, obj);

    // Save document (polymorphic - children serialize themselves)
    document_save(doc, "output.json");

    /*
    ** CLEANUP
    */

    // Auto-generated destructors with proper finalization chain
    object_free(obj);
    document_free(doc);

    json_decref(json1);
    json_decref(json2);

    return 0;
}

/***************************************************************
** DEFINING YOUR OWN TYPE - MINIMAL EXAMPLE
***************************************************************/

/*
** Let's say you want to create a "Layer" type that inherits from Object
*/

// layer.h
typedef struct Layer {
    Object parent;  // Inherit from Object

    // Your fields
    int z_index;
    bool visible;
} Layer;

typedef struct LayerClass {
    ObjectClass parent_class;  // Inherit vtable

    // Your virtual methods
    void (*set_z_index)(Layer* self, int z);
} LayerClass;

// Type system
TypeHandle layer_get_type(void);
Layer* layer_new(void);
void layer_free(Layer* self);

// Your API
void layer_set_z_index(Layer* self, int z);
int layer_get_z_index(const Layer* self);


// layer.c

// ONE LINE TO REGISTER!
LIBCAD_DEFINE_TYPE(Layer, layer, object_get_type())

// Setup vtable (called once)
static void layer_class_init(LayerClass* klass)
{
    // Inherit parent vtable automatically
    klass->set_z_index = layer_set_z_index;
}

// Initialize instance (called per object)
static void layer_init(Layer* self)
{
    self->z_index = 0;
    self->visible = true;
}

// Implement your methods (no wrappers needed!)
void layer_set_z_index(Layer* self, int z)
{
    if (!self) return;
    self->z_index = z;
}

int layer_get_z_index(const Layer* self)
{
    return self ? self->z_index : 0;
}

/*
** That's it! ~30 lines instead of ~150 lines!
**
** You get:
** - Automatic constructor/destructor
** - Inheritance from Object (name property, to_json, etc.)
** - Type registration
** - Polymorphism support
** - Fast vtable dispatch
**
** No manual registration, no wrapper functions, no boilerplate!
*/
