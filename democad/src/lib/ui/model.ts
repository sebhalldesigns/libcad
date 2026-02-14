export interface RibbonAction {
  id: string;
  label: string;
  icon: string;
  hint: string;
  emphasized?: boolean;
}

export interface RibbonGroup {
  id: string;
  label: string;
  actions: RibbonAction[];
}

export interface RibbonTab {
  id: string;
  label: string;
  groups: RibbonGroup[];
}

export interface ProjectNode {
  id: string;
  name: string;
  type: 'folder' | 'sketch' | 'solid' | 'operation';
  depth: number;
  active?: boolean;
}

export interface InspectorField {
  label: string;
  value: string;
  editable?: boolean;
}

export interface ConsoleMessage {
  level: 'info' | 'warn' | 'ok';
  text: string;
  time: string;
}

export const ribbonTabs: RibbonTab[] = [
  {
    id: 'home',
    label: 'Home',
    groups: [
      {
        id: 'project',
        label: 'Project',
        actions: [
          { id: 'new-sketch', label: 'New Sketch', icon: '?', hint: 'Create sketch on selected plane', emphasized: true },
          { id: 'import', label: 'Import', icon: '?', hint: 'Load STEP / STL / DXF' },
          { id: 'save', label: 'Save', icon: '??', hint: 'Save project snapshot' }
        ]
      },
      {
        id: 'draw',
        label: 'Draw',
        actions: [
          { id: 'line', label: 'Line', icon: '?', hint: '2-point line tool' },
          { id: 'arc', label: 'Arc', icon: '?', hint: '3-point arc tool' },
          { id: 'constraints', label: 'Constraints', icon: '?', hint: 'Horizontal / tangent / equal' }
        ]
      },
      {
        id: 'create',
        label: 'Create',
        actions: [
          { id: 'extrude', label: 'Extrude', icon: '?', hint: 'Create volume from profile', emphasized: true },
          { id: 'revolve', label: 'Revolve', icon: '?', hint: 'Revolve around axis' },
          { id: 'loft', label: 'Loft', icon: '?', hint: 'Blend profiles' }
        ]
      }
    ]
  },
  {
    id: 'sketch',
    label: 'Sketch',
    groups: [
      {
        id: 'construction',
        label: 'Construction',
        actions: [
          { id: 'centerline', label: 'Centerline', icon: '?', hint: 'Toggle construction style' },
          { id: 'offset', label: 'Offset', icon: '?', hint: 'Offset selected geometry' },
          { id: 'mirror', label: 'Mirror', icon: '?', hint: 'Mirror across axis' }
        ]
      },
      {
        id: 'dimensions',
        label: 'Dimensions',
        actions: [
          { id: 'smart-dim', label: 'Smart Dim', icon: '?', hint: 'Add parametric dimensions', emphasized: true },
          { id: 'driven', label: 'Driven', icon: '?', hint: 'Add reference dimension' },
          { id: 'table', label: 'Table', icon: '?', hint: 'Inspect parameters as table' }
        ]
      }
    ]
  },
  {
    id: 'view',
    label: 'View',
    groups: [
      {
        id: 'camera',
        label: 'Camera',
        actions: [
          { id: 'fit', label: 'Fit', icon: '?', hint: 'Zoom to extents' },
          { id: 'ortho', label: 'Ortho', icon: '?', hint: 'Orthographic camera', emphasized: true },
          { id: 'perspective', label: 'Persp', icon: '?', hint: 'Perspective camera' }
        ]
      },
      {
        id: 'display',
        label: 'Display',
        actions: [
          { id: 'wireframe', label: 'Wire', icon: '?', hint: 'Wireframe display mode' },
          { id: 'shaded', label: 'Shaded', icon: '?', hint: 'Shaded + edges mode' },
          { id: 'section', label: 'Section', icon: '?', hint: 'Section clipping plane' }
        ]
      }
    ]
  }
];

export const projectTree: ProjectNode[] = [
  { id: 'root', name: 'democad_project', type: 'folder', depth: 0 },
  { id: 'sketches', name: 'Sketches', type: 'folder', depth: 1 },
  { id: 'sketch-1', name: 'Sketch 001 (Top Plane)', type: 'sketch', depth: 2, active: true },
  { id: 'solids', name: 'Bodies', type: 'folder', depth: 1 },
  { id: 'solid-1', name: 'Body 001', type: 'solid', depth: 2 },
  { id: 'op-1', name: 'Extrude 001', type: 'operation', depth: 3 },
  { id: 'op-2', name: 'Fillet 001', type: 'operation', depth: 3 }
];

export const inspectorFields: InspectorField[] = [
  { label: 'Active Tool', value: 'Sketch > Line' },
  { label: 'Plane', value: 'Top (XY)', editable: true },
  { label: 'Grid Snap', value: '1.00 mm', editable: true },
  { label: 'Selection', value: 'No geometry selected' },
  { label: 'Solver Status', value: 'Healthy (0 underconstrained)' }
];

export const consoleMessages: ConsoleMessage[] = [
  { level: 'info', text: 'Session initialized. CAD kernel not attached.', time: '09:41:02' },
  { level: 'ok', text: 'UI workbench ready. Waiting for first sketch.', time: '09:41:09' },
  { level: 'warn', text: 'Renderer placeholder active. Canvas context will be provided by WASM module.', time: '09:41:13' }
];
