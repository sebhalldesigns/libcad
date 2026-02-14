# DemoCAD

A modern CAD application built with Svelte 5 and Tauri.

## 🏗️ Architecture

### Project Structure

```
democad/
├── src/
│   ├── lib/
│   │   ├── components/          # Svelte UI components
│   │   │   ├── Toolbar.svelte   # Top toolbar with tools and operations
│   │   │   ├── Sidebar.svelte   # Left model tree/hierarchy
│   │   │   ├── Viewport.svelte  # Center 3D canvas viewport
│   │   │   ├── PropertiesPanel.svelte  # Right properties panel
│   │   │   └── StatusBar.svelte # Bottom status bar
│   │   └── stores/
│   │       └── appState.ts      # Centralized state management
│   ├── app.css                  # Global styles and CSS variables
│   ├── App.svelte              # Main app component
│   └── main.ts                 # Entry point
├── native/                     # Tauri native wrapper
└── dist/                       # Build output
```

### Component Overview

#### **Toolbar** (`Toolbar.svelte`)
- File operations (New, Open, Save)
- Edit operations (Undo, Redo)
- Drawing tools (Select, Sketch, Extrude, Revolve)
- View tools (Pan, Rotate, Zoom)

#### **Sidebar** (`Sidebar.svelte`)
- Model tree displaying sketches and features
- Visibility toggles for each item
- Selection management
- Quick actions for creating new sketches

#### **Viewport** (`Viewport.svelte`)
- Main 3D rendering canvas (ready for WASM integration)
- Mouse interaction handlers for camera control
- Heads-Up Display (HUD) showing current tool and view
- View cube for orientation reference
- Placeholder grid rendering (will be replaced by WASM)

#### **PropertiesPanel** (`PropertiesPanel.svelte`)
- Displays properties of selected objects
- Editable parameters
- Empty state when nothing is selected

#### **StatusBar** (`StatusBar.svelte`)
- Status messages
- Current tool indicator
- Mouse coordinates
- Version info

### State Management

The application uses Svelte's reactive stores for state management:

- **`currentTool`**: Active tool mode
- **`modelTree`**: Hierarchy of sketches and features
- **`selectedItem`**: Currently selected tree item
- **`objectProperties`**: Properties of selected object
- **`panelVisibility`**: Toggle state for UI panels
- **`statusMessage`**: Current status bar message

### Styling System

Dark theme CAD interface using CSS custom properties:

```css
--color-primary: #6366f1  /* Indigo accent */
--color-bg: #0f0f0f       /* Dark background */
--color-surface: #1a1a1a  /* Panel backgrounds */
--color-text: #e0e0e0     /* Primary text */
```

## 🚀 Development

### Running the Web App

```bash
cd democad
npm install
npm run dev
```

### Building for Production

```bash
npm run build
```

### Running as Tauri App

```bash
cd native
cargo tauri dev
```

### Building Tauri App

```bash
cd native
cargo tauri build
```

## 🔌 WASM Integration Points

The UI is ready for your WASM CAD library integration:

### Canvas Access

The `Viewport.svelte` component exposes a canvas element that your WASM renderer can access:

```typescript
// In Viewport.svelte - onMount hook
const canvas = document.querySelector('canvas');
const ctx = canvas.getContext('webgl2'); // or webgpu

// Initialize your WASM renderer here
// wasmRenderer.init(canvas);
```

### Event Handlers

Mouse/keyboard events are already wired up and can be forwarded to WASM:

- `handleMouseDown` - Mouse button press
- `handleMouseMove` - Mouse movement (camera control)
- `handleMouseUp` - Mouse button release
- `handleWheel` - Scroll wheel (zoom)

### State Synchronization

To sync the UI with your WASM engine:

```typescript
// Update model tree when WASM creates features
import { modelTree } from '../stores/appState';

modelTree.set([
  { id: '1', name: 'Sketch 1', type: 'sketch', visible: true },
  { id: '2', name: 'Extrude 1', type: 'extrude', visible: true }
]);

// Update properties when selection changes
import { objectProperties } from '../stores/appState';

objectProperties.set({
  name: 'Extrude 1',
  type: 'Extrude',
  parameters: { distance: 10, direction: 'Z+' }
});
```

## 📝 TODO

- [ ] Integrate WASM CAD kernel
- [ ] Implement actual file operations
- [ ] Add undo/redo functionality
- [ ] Create sketch drawing tools
- [ ] Add constraint solver integration
- [ ] Implement feature operations (extrude, revolve, etc.)
- [ ] Add keyboard shortcuts
- [ ] Create context menus
- [ ] Add file format support (import/export)
- [ ] Implement view cube interaction

## 🎨 Features

- ✅ Professional CAD layout
- ✅ Dark theme optimized for long sessions
- ✅ Resizable panels
- ✅ Reactive state management
- ✅ Well-commented, maintainable code
- ✅ TypeScript support
- ✅ Canvas ready for 3D rendering
- ✅ Mouse interaction framework
- ✅ Status feedback system

## 📦 Tech Stack

- **Frontend**: Svelte 5
- **Build Tool**: Vite
- **Language**: TypeScript
- **Native Wrapper**: Tauri
- **Styling**: CSS with custom properties
- **State**: Svelte Stores
