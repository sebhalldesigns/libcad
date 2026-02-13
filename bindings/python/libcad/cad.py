import ctypes
import ctypes.util
import os


# Types
cad_ctx_t = ctypes.c_void_p
cad_model_t = ctypes.c_void_p

# Constants - Mouse buttons
MOUSE_LEFT_BUTTON = 1
MOUSE_MIDDLE_BUTTON = 2
MOUSE_RIGHT_BUTTON = 3

# Constants - Modifiers
MODIFIER_CONTROL = 1
MODIFIER_SHIFT = 2
MODIFIER_ALT = 3

# Constants - Cursor types
CURSOR_NORMAL = 0
CURSOR_MOVE = 1
CURSOR_RESIZE_H = 2
CURSOR_RESIZE_V = 3
CURSOR_RESIZE_NWSE = 4
CURSOR_RESIZE_NESW = 5
CURSOR_POINTER = 6

# Module-level library handle
_lib_handle = None


def _get_lib() -> ctypes.CDLL:
    """Get or initialize the libcad library handle."""
    global _lib_handle

    if _lib_handle is None:
        # Try to find the library
        lib_path = ctypes.util.find_library("cad")

        # On Windows, also try looking in the build directory
        if lib_path is None:
            # Try common build locations
            possible_paths = [
                os.path.join(os.path.dirname(__file__), "../../../build/Debug/cad.dll"),
                os.path.join(os.path.dirname(__file__), "../../../build/Release/cad.dll"),
                os.path.join(os.path.dirname(__file__), "../../../build/cad.dll"),
            ]
            for path in possible_paths:
                abs_path = os.path.abspath(path)
                if os.path.exists(abs_path):
                    lib_path = abs_path
                    break

        if lib_path is None:
            raise RuntimeError("Could not find libcad library. Make sure it's built and in your system path.")

        _lib_handle = ctypes.CDLL(lib_path)

        # Set up function signatures
        _setup_function_signatures(_lib_handle)

    return _lib_handle


def _setup_function_signatures(lib):
    """Configure ctypes function signatures for all libcad functions."""

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

    # Context management
    setup_func('cad_create_context', [], cad_ctx_t)
    setup_func('cad_destroy_context', [cad_ctx_t], None)

    # Model loading
    setup_func('cad_load_model_file', [cad_ctx_t, ctypes.POINTER(cad_model_t), ctypes.c_char_p], ctypes.c_bool)
    setup_func('cad_load_model_data', [cad_ctx_t, ctypes.POINTER(cad_model_t), ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t], ctypes.c_bool)
    setup_func('cad_unload_model', [cad_ctx_t], None)

    # Model writing
    setup_func('cad_write_model_file', [cad_ctx_t, cad_model_t, ctypes.c_char_p, ctypes.c_char_p], ctypes.c_bool)
    setup_func('cad_write_model_data', [cad_ctx_t, cad_model_t, ctypes.c_char_p, ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t)], ctypes.c_bool)

    # Viewport
    setup_func('cad_set_viewport', [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int], None)
    setup_func('cad_render_viewport', [], None)
    setup_func('cad_init_viewport', [], None)

    # Input handling
    setup_func('cad_set_cursor_pos', [ctypes.c_int, ctypes.c_int], None)
    setup_func('cad_cursor_lost', [], None)
    setup_func('cad_set_cursor_button_state', [ctypes.c_int, ctypes.c_bool], None)
    setup_func('cad_set_modifier_state', [ctypes.c_int, ctypes.c_bool], None)
    setup_func('cad_axis_delta', [ctypes.c_int, ctypes.c_float], None)

    # Tools
    setup_func('cad_start_modal_tool', [ctypes.c_int], None)
    setup_func('cad_clear_modal_tool', [], None)
    setup_func('cad_get_cursor_type', [], ctypes.c_int)

    # JSON serialization
    setup_func('cad_save_json', [ctypes.c_char_p], None)
    setup_func('cad_load_json', [ctypes.c_char_p], None)


# Context management functions
def create_context() -> cad_ctx_t:
    """Create a new CAD context."""
    return _get_lib().cad_create_context()


def destroy_context(ctx: cad_ctx_t):
    """Destroy a CAD context."""
    _get_lib().cad_destroy_context(ctx)


# Model loading functions
def load_model_file(ctx: cad_ctx_t, filepath: str) -> tuple[bool, cad_model_t]:
    """
    Load a model from a file.

    Returns:
        tuple[bool, cad_model_t]: (success, model_handle)
    """
    model = cad_model_t()
    success = _get_lib().cad_load_model_file(ctx, ctypes.byref(model), filepath.encode('utf-8'))
    return success, model


def load_model_data(ctx: cad_ctx_t, data: bytes) -> tuple[bool, cad_model_t]:
    """
    Load a model from bytes data.

    Returns:
        tuple[bool, cad_model_t]: (success, model_handle)
    """
    model = cad_model_t()
    data_array = (ctypes.c_uint8 * len(data)).from_buffer_copy(data)
    success = _get_lib().cad_load_model_data(ctx, ctypes.byref(model), data_array, len(data))
    return success, model


def unload_model(ctx: cad_ctx_t):
    """Unload the current model."""
    _get_lib().cad_unload_model(ctx)


# Model writing functions
def write_model_file(ctx: cad_ctx_t, model: cad_model_t, format: str, filepath: str) -> bool:
    """
    Write a model to a file.

    Args:
        ctx: CAD context
        model: Model handle
        format: Format string (e.g., "obj", "stl")
        filepath: Output file path

    Returns:
        bool: Success status
    """
    return _get_lib().cad_write_model_file(ctx, model, format.encode('utf-8'), filepath.encode('utf-8'))


def write_model_data(ctx: cad_ctx_t, model: cad_model_t, format: str) -> tuple[bool, bytes]:
    """
    Write a model to bytes data.

    Args:
        ctx: CAD context
        model: Model handle
        format: Format string (e.g., "obj", "stl")

    Returns:
        tuple[bool, bytes]: (success, data)
    """
    data_ptr = ctypes.POINTER(ctypes.c_uint8)()
    size = ctypes.c_size_t()
    success = _get_lib().cad_write_model_data(ctx, model, format.encode('utf-8'), ctypes.byref(data_ptr), ctypes.byref(size))

    if success and size.value > 0:
        data = bytes(data_ptr[:size.value])
        return success, data
    return success, b''


# Viewport functions
def set_viewport(x: int, y: int, width: int, height: int, window_width: int, window_height: int):
    """Set the viewport parameters."""
    _get_lib().cad_set_viewport(x, y, width, height, window_width, window_height)


def render_viewport():
    """Render the viewport."""
    _get_lib().cad_render_viewport()


def init_viewport():
    """Initialize the viewport."""
    _get_lib().cad_init_viewport()


# Input handling functions
def set_cursor_pos(x: int, y: int):
    """Set the cursor position."""
    _get_lib().cad_set_cursor_pos(x, y)


def cursor_lost():
    """Notify that the cursor has left the viewport."""
    _get_lib().cad_cursor_lost()


def set_cursor_button_state(button: int, pressed: bool):
    """Set the state of a mouse button."""
    _get_lib().cad_set_cursor_button_state(button, pressed)


def set_modifier_state(modifier: int, state: bool):
    """Set the state of a keyboard modifier."""
    _get_lib().cad_set_modifier_state(modifier, state)


def axis_delta(axis: int, delta: float):
    """Report axis movement (e.g., mouse wheel)."""
    _get_lib().cad_axis_delta(axis, delta)


# Tool functions
def start_modal_tool(tool_id: int):
    """Start a modal tool."""
    _get_lib().cad_start_modal_tool(tool_id)


def clear_modal_tool():
    """Clear the current modal tool."""
    _get_lib().cad_clear_modal_tool()


def get_cursor_type() -> int:
    """Get the current cursor type."""
    return _get_lib().cad_get_cursor_type()


# JSON serialization functions
def save_json(path: str):
    """Save to a JSON file."""
    _get_lib().cad_save_json(path.encode('utf-8'))


def load_json(path: str):
    """Load from a JSON file."""
    _get_lib().cad_load_json(path.encode('utf-8'))
