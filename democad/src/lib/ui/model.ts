export interface RibbonAction {
  id: string;
  label: string;
  icon: string;
  hint: string;
  // Ribbon actions use fixed tile footprints for a more consistent CAD-style layout.
  size?: 'large' | 'small';
  selected?: boolean;
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

const BI_BASE = 'https://cdn.jsdelivr.net/npm/bootstrap-icons@1.13.1/icons';
const bi = (name: string): string => `${BI_BASE}/${name}.svg`;

export const ribbonTabs: RibbonTab[] = [
  {
    id: 'home',
    label: 'Home',
    groups: [
      {
        id: 'project',
        label: 'Project',
        actions: [
          { id: 'new-sketch', label: 'New Sketch', icon: bi('pencil-square'), hint: 'Create sketch on selected plane', size: 'large', selected: true, emphasized: true },
          { id: 'import', label: 'Import', icon: bi('box-arrow-in-down'), hint: 'Load STEP / STL / DXF', size: 'small' },
          { id: 'save', label: 'Save', icon: bi('floppy'), hint: 'Save project snapshot', size: 'small' }
        ]
      },
      {
        id: 'draw',
        label: 'Draw',
        actions: [
          { id: 'line', label: 'Line', icon: bi('slash-lg'), hint: '2-point line tool', size: 'large' },
          { id: 'arc', label: 'Arc', icon: bi('circle'), hint: '3-point arc tool', size: 'small' },
          { id: 'constraints', label: 'Constraints', icon: bi('link-45deg'), hint: 'Horizontal / tangent / equal', size: 'small' }
        ]
      },
      {
        id: 'create',
        label: 'Create',
        actions: [
          { id: 'extrude', label: 'Extrude', icon: bi('box'), hint: 'Create volume from profile', size: 'large', selected: true, emphasized: true },
          { id: 'revolve', label: 'Revolve', icon: bi('arrow-repeat'), hint: 'Revolve around axis', size: 'small' },
          { id: 'loft', label: 'Loft', icon: bi('layers'), hint: 'Blend profiles', size: 'small' }
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
          { id: 'centerline', label: 'Centerline', icon: bi('dash-lg'), hint: 'Toggle construction style', size: 'large' },
          { id: 'offset', label: 'Offset', icon: bi('arrows-move'), hint: 'Offset selected geometry', size: 'small' },
          { id: 'mirror', label: 'Mirror', icon: bi('symmetry-horizontal'), hint: 'Mirror across axis', size: 'small' }
        ]
      },
      {
        id: 'dimensions',
        label: 'Dimensions',
        actions: [
          { id: 'smart-dim', label: 'Smart Dim', icon: bi('rulers'), hint: 'Add parametric dimensions', size: 'large', selected: true, emphasized: true },
          { id: 'driven', label: 'Driven', icon: bi('123'), hint: 'Add reference dimension', size: 'small' },
          { id: 'table', label: 'Table', icon: bi('table'), hint: 'Inspect parameters as table', size: 'small' }
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
          { id: 'fit', label: 'Fit', icon: bi('arrows-fullscreen'), hint: 'Zoom to extents', size: 'large' },
          { id: 'ortho', label: 'Ortho', icon: bi('bounding-box'), hint: 'Orthographic camera', size: 'small', selected: true, emphasized: true },
          { id: 'perspective', label: 'Persp', icon: bi('camera'), hint: 'Perspective camera', size: 'small' }
        ]
      },
      {
        id: 'display',
        label: 'Display',
        actions: [
          { id: 'wireframe', label: 'Wire', icon: bi('grid-3x3'), hint: 'Wireframe display mode', size: 'large' },
          { id: 'shaded', label: 'Shaded', icon: bi('eye'), hint: 'Shaded + edges mode', size: 'small' },
          { id: 'section', label: 'Section', icon: bi('scissors'), hint: 'Section clipping plane', size: 'small' }
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
