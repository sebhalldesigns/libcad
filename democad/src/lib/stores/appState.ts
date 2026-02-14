/**
 * Application State Management
 *
 * Centralized stores for managing the CAD application state.
 * Uses Svelte's reactive stores for automatic UI updates.
 */

import { writable, derived, type Writable } from 'svelte/store';

/**
 * Tool modes available in the application
 */
export type ToolMode = 'select' | 'sketch' | 'extrude' | 'revolve' | 'pan' | 'rotate' | 'zoom';

/**
 * Represents a sketch or feature in the model tree
 */
export interface TreeItem {
  id: string;
  name: string;
  type: 'sketch' | 'extrude' | 'revolve' | 'chamfer' | 'fillet';
  visible: boolean;
  children?: TreeItem[];
}

/**
 * Properties for the currently selected object
 */
export interface ObjectProperties {
  name: string;
  type: string;
  parameters: Record<string, any>;
}

/**
 * UI Panel visibility state
 */
export interface PanelState {
  leftSidebar: boolean;
  rightPanel: boolean;
  statusBar: boolean;
}

// Current active tool
export const currentTool: Writable<ToolMode> = writable('select');

// Model tree structure (sketches, features, etc.)
export const modelTree: Writable<TreeItem[]> = writable([
  {
    id: '1',
    name: 'Sketch 1',
    type: 'sketch',
    visible: true
  }
]);

// Currently selected item in the tree
export const selectedItem: Writable<string | null> = writable(null);

// Properties of the selected object
export const objectProperties: Writable<ObjectProperties | null> = writable(null);

// Panel visibility toggles
export const panelVisibility: Writable<PanelState> = writable({
  leftSidebar: true,
  rightPanel: true,
  statusBar: true
});

// Status bar message
export const statusMessage: Writable<string> = writable('Ready');

// Derived store: check if anything is selected
export const hasSelection = derived(
  selectedItem,
  $selectedItem => $selectedItem !== null
);

/**
 * Helper function to update status message
 */
export function setStatus(message: string) {
  statusMessage.set(message);
}

/**
 * Helper function to toggle tool
 */
export function setTool(tool: ToolMode) {
  currentTool.set(tool);
  setStatus(`Tool: ${tool}`);
}

/**
 * Helper function to toggle panel visibility
 */
export function togglePanel(panel: keyof PanelState) {
  panelVisibility.update(state => ({
    ...state,
    [panel]: !state[panel]
  }));
}
