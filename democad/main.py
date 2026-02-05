import sys
import os
from PyQt6 import QtWidgets, QtCore, QtGui
from PyQt6.QtOpenGLWidgets import QOpenGLWidget
from PyQt6 import uic

sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), "../bindings/python"))
)

from libcad import cad


class CADViewport(QOpenGLWidget):
    """Custom OpenGL widget that integrates the libcad rendering."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.cad_ctx = None
        self.setMouseTracking(True)  # Enable mouse move events
        self.setFocusPolicy(QtCore.Qt.FocusPolicy.StrongFocus)  # Enable keyboard events

    def initializeGL(self):
        """Initialize OpenGL context and CAD viewport."""
        # Create CAD context
        self.cad_ctx = cad.create_context()

        # Initialize the CAD viewport
        cad.init_viewport()

        print(f"CAD context created: {self.cad_ctx}")

    def resizeGL(self, w, h):
        """Handle viewport resize."""
        if self.cad_ctx:
            # Set viewport to match widget size
            # Parameters: x, y, width, height, window_width, window_height
            cad.set_viewport(0, 0, w, h, w, h)

    def paintGL(self):
        """Render the CAD viewport."""
        if self.cad_ctx:
            # Render the CAD scene
            cad.render_viewport()

    # Mouse event handlers
    def mousePressEvent(self, event: QtGui.QMouseEvent):
        """Handle mouse button press."""
        if not self.cad_ctx:
            return

        button = self._qt_to_cad_button(event.button())
        if button:
            cad.set_cursor_button_state(button, True)
            self.update()  # Request repaint

    def mouseReleaseEvent(self, event: QtGui.QMouseEvent):
        """Handle mouse button release."""
        if not self.cad_ctx:
            return

        button = self._qt_to_cad_button(event.button())
        if button:
            cad.set_cursor_button_state(button, False)
            self.update()

    def mouseMoveEvent(self, event: QtGui.QMouseEvent):
        """Handle mouse movement."""
        if not self.cad_ctx:
            return

        pos = event.position()
        cad.set_cursor_pos(int(pos.x()), int(pos.y()))

        # Update modifier states
        self._update_modifiers(event.modifiers())

        self.update()

    def wheelEvent(self, event: QtGui.QWheelEvent):
        """Handle mouse wheel scrolling."""
        if not self.cad_ctx:
            return

        # Get scroll delta (positive = scroll up, negative = scroll down)
        delta = event.angleDelta().y() / 120.0  # Normalize to scroll "clicks"

        # Use axis 0 for vertical scroll (you can add horizontal if needed)
        cad.axis_delta(0, delta)

        self.update()

    def leaveEvent(self, event):
        """Handle cursor leaving the widget."""
        if self.cad_ctx:
            cad.cursor_lost()
            self.update()

    def keyPressEvent(self, event: QtGui.QKeyEvent):
        """Handle key press for modifiers."""
        if self.cad_ctx:
            self._update_modifiers(event.modifiers())
            self.update()

    def keyReleaseEvent(self, event: QtGui.QKeyEvent):
        """Handle key release for modifiers."""
        if self.cad_ctx:
            self._update_modifiers(event.modifiers())
            self.update()

    def _qt_to_cad_button(self, qt_button):
        """Convert Qt mouse button to CAD button constant."""
        if qt_button == QtCore.Qt.MouseButton.LeftButton:
            return cad.MOUSE_LEFT_BUTTON
        elif qt_button == QtCore.Qt.MouseButton.MiddleButton:
            return cad.MOUSE_MIDDLE_BUTTON
        elif qt_button == QtCore.Qt.MouseButton.RightButton:
            return cad.MOUSE_RIGHT_BUTTON
        return None

    def _update_modifiers(self, qt_modifiers):
        """Update CAD modifier states from Qt modifiers."""
        # Control key
        ctrl_pressed = bool(qt_modifiers & QtCore.Qt.KeyboardModifier.ControlModifier)
        cad.set_modifier_state(cad.MODIFIER_CONTROL, ctrl_pressed)

        # Shift key
        shift_pressed = bool(qt_modifiers & QtCore.Qt.KeyboardModifier.ShiftModifier)
        cad.set_modifier_state(cad.MODIFIER_SHIFT, shift_pressed)

        # Alt key
        alt_pressed = bool(qt_modifiers & QtCore.Qt.KeyboardModifier.AltModifier)
        cad.set_modifier_state(cad.MODIFIER_ALT, alt_pressed)

    def cleanup(self):
        """Clean up CAD context."""
        if self.cad_ctx:
            cad.destroy_context(self.cad_ctx)
            self.cad_ctx = None


class MainWindow(QtWidgets.QMainWindow):
    """Main application window."""

    def __init__(self):
        super().__init__()

        # Load UI file
        dirname = os.path.dirname(__file__)
        filename = os.path.join(dirname, 'democad.ui')
        uic.loadUi(filename, self)

        # Replace the QOpenGLWidget placeholder with our custom CADViewport
        self.setup_viewport()

        # Connect UI buttons to CAD tools
        self.connect_tool_buttons()

    def setup_viewport(self):
        """Replace the placeholder viewport with our custom CAD viewport."""
        # Find the splitter that contains the viewport
        splitter = self.findChild(QtWidgets.QSplitter, "splitter")

        if splitter:
            # Find the existing viewport widget
            old_viewport = self.findChild(QOpenGLWidget, "viewport")

            if old_viewport:
                # Get the index of the old viewport in the splitter
                index = splitter.indexOf(old_viewport)

                # Create our custom viewport
                self.viewport = CADViewport(self)
                self.viewport.setObjectName("viewport")
                self.viewport.setMinimumSize(500, 1)

                # Remove old viewport and insert new one at the same position
                old_viewport.setParent(None)
                splitter.insertWidget(index, self.viewport)

    def connect_tool_buttons(self):
        """Connect UI buttons to CAD tool functions."""
        # None/Select button - clears modal tool
        btn = self.findChild(QtWidgets.QPushButton, "sketch_none_button")
        if btn:
            btn.clicked.connect(self.clear_tool)

        # Line tool
        btn = self.findChild(QtWidgets.QPushButton, "sketch_line_button")
        if btn:
            btn.clicked.connect(lambda: self.start_tool(1))

        # Circle tool
        btn = self.findChild(QtWidgets.QPushButton, "sketch_circle_button")
        if btn:
            btn.clicked.connect(lambda: self.start_tool(2))

        # Rectangle tool
        btn = self.findChild(QtWidgets.QPushButton, "sketch_rectangle_button")
        if btn:
            btn.clicked.connect(lambda: self.start_tool(3))

        # Triangle tool
        btn = self.findChild(QtWidgets.QPushButton, "sketch_triangle_button")
        if btn:
            btn.clicked.connect(lambda: self.start_tool(4))

    def start_tool(self, tool_id: int):
        """Start a modal CAD tool."""
        if hasattr(self, 'viewport') and self.viewport.cad_ctx:
            cad.start_modal_tool(tool_id)
            self.viewport.update()

    def clear_tool(self):
        """Clear the current modal tool."""
        if hasattr(self, 'viewport') and self.viewport.cad_ctx:
            cad.clear_modal_tool()
            self.viewport.update()

    def closeEvent(self, event):
        """Clean up when window is closed."""
        if hasattr(self, 'viewport'):
            self.viewport.cleanup()
        event.accept()


def main():
    app = QtWidgets.QApplication(sys.argv)

    window = MainWindow()
    window.show()

    sys.exit(app.exec())


if __name__ == "__main__":
    main()
