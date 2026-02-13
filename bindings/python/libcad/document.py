"""
libcad Document Model Python Bindings

Provides object-oriented Python wrappers for the libcad document model.
"""

import ctypes
from typing import Optional
from .cad import _get_lib


# Type handles
type_handle_t = ctypes.c_void_p
property_handle_t = ctypes.c_void_p
method_handle_t = ctypes.c_void_p
type_instance_handle_t = ctypes.c_void_p

TYPE_INVALID_HANDLE = 0


def _setup_document_signatures(lib):
    """Configure ctypes function signatures for document model functions."""

    # Helper to safely set up function signatures
    def setup_func(name, argtypes, restype):
        try:
            func = getattr(lib, name)
            func.argtypes = argtypes
            func.restype = restype
            return True
        except AttributeError:
            # Function doesn't exist in DLL, skip it
            return False

    # Type system initialization
    setup_func('cad_init_type_system', [], None)

    # Object C API
    setup_func('object_create', [], ctypes.c_void_p)
    setup_func('object_destroy', [ctypes.c_void_p], None)
    setup_func('object_set_name', [ctypes.c_void_p, ctypes.c_char_p], None)
    setup_func('object_get_name', [ctypes.c_void_p], ctypes.c_char_p)
    setup_func('object_debug_print', [ctypes.c_void_p], None)

    # Document C API
    setup_func('document_create', [], ctypes.c_void_p)
    setup_func('document_destroy', [ctypes.c_void_p], None)
    setup_func('document_set_path', [ctypes.c_void_p, ctypes.c_char_p], None)
    setup_func('document_get_path', [ctypes.c_void_p], ctypes.c_char_p)
    setup_func('document_debug_print', [ctypes.c_void_p], None)

    # Document children management
    setup_func('document_add_child', [ctypes.c_void_p, ctypes.c_void_p], None)
    setup_func('document_remove_child', [ctypes.c_void_p, ctypes.c_size_t], None)
    setup_func('document_get_child', [ctypes.c_void_p, ctypes.c_size_t], ctypes.c_void_p)
    setup_func('document_get_child_count', [ctypes.c_void_p], ctypes.c_size_t)

    # Document serialization
    setup_func('document_save', [ctypes.c_void_p, ctypes.c_char_p], ctypes.c_bool)
    setup_func('document_load', [ctypes.c_char_p], ctypes.c_void_p)

    # TextField C API
    setup_func('textfield_create', [], ctypes.c_void_p)
    setup_func('textfield_destroy', [ctypes.c_void_p], None)
    setup_func('textfield_set_text', [ctypes.c_void_p, ctypes.c_char_p], None)
    setup_func('textfield_get_text', [ctypes.c_void_p], ctypes.c_char_p)
    setup_func('textfield_set_position', [ctypes.c_void_p, ctypes.c_float, ctypes.c_float], None)
    setup_func('textfield_set_font_size', [ctypes.c_void_p, ctypes.c_float], None)
    setup_func('textfield_debug_print', [ctypes.c_void_p], None)

    # Type system API
    setup_func('object_get_type_handle', [], type_handle_t)
    setup_func('document_get_type_handle', [], type_handle_t)
    setup_func('type_instance_create', [type_handle_t], type_instance_handle_t)
    setup_func('type_instance_destroy', [type_instance_handle_t], None)
    setup_func('type_get_property', [ctypes.c_char_p, type_handle_t], property_handle_t)
    setup_func('type_get_method', [ctypes.c_char_p, type_handle_t], method_handle_t)
    setup_func('type_instance_call_method', [type_instance_handle_t, method_handle_t, ctypes.c_void_p, ctypes.c_void_p], None)


# Initialize signatures when module is imported
_setup_document_signatures(_get_lib())


# Initialize the type system (call once)
_type_system_initialized = False

def init_type_system():
    """Initialize the type system. Call this once before using document model."""
    global _type_system_initialized
    if not _type_system_initialized:
        _get_lib().cad_init_type_system()
        _type_system_initialized = True


class Object:
    """
    Python wrapper for libcad object_t.

    Base class for all document model objects. Provides name property
    and debug printing.

    Example:
        >>> obj = Object()
        >>> obj.name = "My Object"
        >>> obj.debug_print()
        >>> del obj  # or obj.destroy()
    """

    def __init__(self, ptr: Optional[int] = None):
        """
        Create a new object or wrap an existing C pointer.

        Args:
            ptr: Optional C pointer to existing object_t. If None, creates new object.
        """
        if not _type_system_initialized:
            init_type_system()

        if ptr is None:
            self._ptr = _get_lib().object_create()
        else:
            self._ptr = ptr

        if not self._ptr:
            raise RuntimeError("Failed to create object")

    def destroy(self):
        """Explicitly destroy the object and free C memory."""
        if self._ptr:
            _get_lib().object_destroy(self._ptr)
            self._ptr = None

    def __del__(self):
        """Destructor - automatically called when Python object is garbage collected."""
        self.destroy()

    @property
    def name(self) -> Optional[str]:
        """Get the object name."""
        if not self._ptr:
            return None
        name_bytes = _get_lib().object_get_name(self._ptr)
        return name_bytes.decode('utf-8') if name_bytes else None

    @name.setter
    def name(self, value: Optional[str]):
        """Set the object name."""
        if self._ptr:
            name_bytes = value.encode('utf-8') if value else None
            _get_lib().object_set_name(self._ptr, name_bytes)

    def debug_print(self):
        """Print debug information about this object."""
        if self._ptr:
            _get_lib().object_debug_print(self._ptr)

    @property
    def _as_parameter_(self):
        """Allow this object to be passed directly to ctypes functions."""
        return self._ptr


class Document(Object):
    """
    Python wrapper for libcad document_t.

    Inherits from Object and adds path property. Documents represent
    files in the CAD system.

    Example:
        >>> doc = Document()
        >>> doc.name = "My Drawing"
        >>> doc.path = "/path/to/drawing.lcad"
        >>> doc.debug_print()
        >>> del doc
    """

    def __init__(self, ptr: Optional[int] = None):
        """
        Create a new document or wrap an existing C pointer.

        Args:
            ptr: Optional C pointer to existing document_t. If None, creates new document.
        """
        # Don't call super().__init__ because we create a different type
        if not _type_system_initialized:
            init_type_system()

        if ptr is None:
            self._ptr = _get_lib().document_create()
        else:
            self._ptr = ptr

        if not self._ptr:
            raise RuntimeError("Failed to create document")

    def destroy(self):
        """Explicitly destroy the document and free C memory."""
        if self._ptr:
            _get_lib().document_destroy(self._ptr)
            self._ptr = None

    @property
    def path(self) -> Optional[str]:
        """Get the document path."""
        if not self._ptr:
            return None
        path_bytes = _get_lib().document_get_path(self._ptr)
        return path_bytes.decode('utf-8') if path_bytes else None

    @path.setter
    def path(self, value: Optional[str]):
        """Set the document path."""
        if self._ptr:
            path_bytes = value.encode('utf-8') if value else None
            _get_lib().document_set_path(self._ptr, path_bytes)

    def debug_print(self):
        """Print debug information about this document (overrides Object version)."""
        if self._ptr:
            _get_lib().document_debug_print(self._ptr)


# Convenience functions for type system access

def get_object_type_handle() -> type_handle_t:
    """Get the type handle for object type."""
    init_type_system()
    return _get_lib().object_get_type_handle()


def get_document_type_handle() -> type_handle_t:
    """Get the type handle for document type."""
    init_type_system()
    return _get_lib().document_get_type_handle()


def create_instance_from_type(type_handle: type_handle_t) -> type_instance_handle_t:
    """
    Create a type instance using the type system (for advanced use).

    Args:
        type_handle: Type handle from get_object_type_handle() or get_document_type_handle()

    Returns:
        Type instance handle
    """
    return _get_lib().type_instance_create(type_handle)


def destroy_instance(instance_handle: type_instance_handle_t):
    """Destroy a type instance created with create_instance_from_type()."""
    _get_lib().type_instance_destroy(instance_handle)


def call_method(instance_handle: type_instance_handle_t, method_name: str, type_handle: type_handle_t):
    """
    Call a method on a type instance (for advanced use).

    Args:
        instance_handle: Instance handle
        method_name: Name of method (e.g., "debug_print")
        type_handle: Type handle
    """
    method_handle = _get_lib().type_get_method(method_name.encode('utf-8'), type_handle)
    if method_handle:
        _get_lib().type_instance_call_method(instance_handle, method_handle, None, None)
