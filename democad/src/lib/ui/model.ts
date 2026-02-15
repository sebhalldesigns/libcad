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
  entityId?: number;
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
          { id: 'new-project', label: 'New Project', icon: bi('file-earmark-plus'), hint: 'Create a new project file', size: 'large', emphasized: true },
          { id: 'open-project', label: 'Open', icon: bi('folder2-open'), hint: 'Open an existing project', size: 'small' },
          { id: 'export-project', label: 'Export', icon: bi('box-arrow-up-right'), hint: 'Export geometry to exchange format', size: 'small' },
          { id: 'save-project', label: 'Save', icon: bi('floppy'), hint: 'Save current project', size: 'small' },
          { id: 'save-project-as', label: 'Save As', icon: bi('floppy2'), hint: 'Save project with a new name', size: 'small' }
        ]
      },
      {
        id: 'model',
        label: 'Model',
        actions: [
          { id: 'create-extrude', label: 'Extrude', icon: bi('box'), hint: 'Create volume from profile', size: 'large', emphasized: true },
          { id: 'create-revolve', label: 'Revolve', icon: bi('arrow-repeat'), hint: 'Revolve profile around axis', size: 'small' },
          { id: 'create-loft', label: 'Loft', icon: bi('layers'), hint: 'Blend between profiles', size: 'small' }
        ]
      },
      {
        id: 'construction',
        label: 'Construct',
        actions: [
          { id: 'create-sketch', label: 'Sketch', icon: bi('pencil-square'), hint: 'Start a new sketch', size: 'large' },
          { id: 'create-plane', label: 'Plane', icon: bi('square'), hint: 'Create a reference plane', size: 'small' },
          { id: 'create-axis', label: 'Axis', icon: bi('textarea-resize'), hint: 'Create a reference axis', size: 'small' }
        ]
      }
    ]
  },
  {
    id: 'sketch',
    label: 'Sketch',
    groups: [
      {
        id: 'sketch-session',
        label: 'Sketch',
        actions: [
          { id: 'sketch-apply', label: 'Apply', icon: bi('check2-circle'), hint: 'Apply sketch changes', size: 'large' },
          { id: 'sketch-close', label: 'Close', icon: bi('x-circle'), hint: 'Close active sketch session', size: 'large' }
        ]
      },
      {
        id: 'sketch-create',
        label: 'Create',
        actions: [
          { id: 'sketch-extrude', label: 'Extrude', icon: bi('box'), hint: 'Create an extrude from sketch', size: 'large' },
          { id: 'sketch-revolve', label: 'Revolve', icon: bi('arrow-repeat'), hint: 'Create a revolve from sketch', size: 'small' },
          { id: 'sketch-loft', label: 'Loft', icon: bi('layers'), hint: 'Create a loft from sketch', size: 'small' }
        ]
      },
      {
        id: 'draw',
        label: 'Draw',
        actions: [
          { id: 'draw-none', label: 'None', icon: bi('cursor'), hint: 'Exit active draw tool', size: 'large' },
          { id: 'draw-line', label: 'Line', icon: bi('slash-lg'), hint: '2-point line tool', size: 'large', emphasized: true },
          { id: 'draw-circle', label: 'Circle', icon: bi('circle'), hint: 'Center-point circle tool', size: 'small' },
          { id: 'draw-arc', label: 'Arc', icon: bi('pie-chart'), hint: '3-point arc tool', size: 'small' },
          { id: 'draw-center-rectangle', label: 'Center Rect', icon: bi('bounding-box'), hint: 'Rectangle from center', size: 'small' },
          { id: 'draw-corner-rectangle', label: 'Corner Rect', icon: bi('square'), hint: 'Rectangle from corner', size: 'small' },
          { id: 'draw-inscribed-polygon', label: 'Inscribed Poly', icon: bi('pentagon'), hint: 'Polygon inscribed in circle', size: 'small' },
          { id: 'draw-circumscribed-polygon', label: 'Circumscribed Poly', icon: bi('hexagon'), hint: 'Polygon circumscribed about circle', size: 'small' }
        ]
      },
      {
        id: 'constraints',
        label: 'Constraints',
        actions: [
          { id: 'constraint-incident', label: 'Incident', icon: bi('dot'), hint: 'Coincident / incident constraint', size: 'small' },
          { id: 'constraint-perpendicular', label: 'Perpendicular', icon: bi('distribute-vertical'), hint: 'Perpendicular constraint', size: 'small' },
          { id: 'constraint-parallel', label: 'Parallel', icon: bi('pause'), hint: 'Parallel constraint', size: 'small' },
          { id: 'constraint-concentric', label: 'Concentric', icon: bi('bullseye'), hint: 'Concentric circles/arcs', size: 'small' },
          { id: 'constraint-tangent', label: 'Tangent', icon: bi('bezier2'), hint: 'Tangent continuity constraint', size: 'small' },
          { id: 'constraint-equal', label: 'Equal', icon: bi('equals'), hint: 'Equal length/radius constraint', size: 'small' }
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
          { id: 'camera-fit', label: 'Fit', icon: bi('arrows-fullscreen'), hint: 'Fit model in view', size: 'large' },
          { id: 'camera-orthographic', label: 'Orthographic', icon: bi('bounding-box'), hint: 'Use orthographic projection', size: 'small', emphasized: true },
          { id: 'camera-perspective', label: 'Perspective', icon: bi('camera'), hint: 'Use perspective projection', size: 'small' },
          { id: 'camera-zoom-in', label: 'Zoom In', icon: bi('zoom-in'), hint: 'Zoom camera in', size: 'small' },
          { id: 'camera-zoom-out', label: 'Zoom Out', icon: bi('zoom-out'), hint: 'Zoom camera out', size: 'small' }
        ]
      },
      {
        id: 'display',
        label: 'Display',
        actions: [
          { id: 'display-wireframe', label: 'Wireframe', icon: bi('grid-3x3'), hint: 'Show wireframe mode', size: 'small' },
          { id: 'display-shaded', label: 'Shaded', icon: bi('eye-fill'), hint: 'Show shaded mode', size: 'small' },
          { id: 'display-shaded-edges', label: 'Shaded + Edges', icon: bi('bounding-box-circles'), hint: 'Show shaded mode with edges', size: 'small', emphasized: true },
          { id: 'display-shaded-hidden-edges', label: 'Shaded + Hidden Edges', icon: bi('eye-slash'), hint: 'Show shaded mode with hidden edges', size: 'small' }
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

import { writable } from 'svelte/store';

export const consoleMessages = writable<ConsoleMessage[]>([

]);

export function addConsoleMessage(level: 'info' | 'warn' | 'ok', text: string): void {
  const time = new Date().toLocaleTimeString('en-US', { hour12: false });
  consoleMessages.update(messages => [...messages.slice(-199), { level, text, time }]);  // Keep last 200 messages
}

// Helper to convert CAD document JSON to ProjectNode tree
export function convertDocumentToTree(docJson: any): ProjectNode[] {
  if (!docJson) return [];

  const nodes: ProjectNode[] = [];
  const normalizeEntityId = (raw: unknown): number | undefined => {
    if (typeof raw !== 'number' || !Number.isFinite(raw)) return undefined;
    const value = raw >>> 0;
    if (value === 0 || value === 0xFFFFFFFF) return undefined;
    return value;
  };

  function addNode(obj: any, depth: number, path: string): void {
    if (!obj) return;

    // Map CAD types to UI types
    let nodeType: ProjectNode['type'] = 'folder';
    if (obj.type === 'plane_t') nodeType = 'folder'; // Planes are construction elements
    else if (obj.type === 'axis_t') nodeType = 'folder'; // Axes are construction elements
    else if (obj.type === 'sketch_t') nodeType = 'sketch';
    else if (obj.type === 'line_t' || obj.type === 'circle_t' || obj.type === 'rectangle_t') nodeType = 'operation';
    else if (obj.type === 'body_t') nodeType = 'solid';
    else if (obj.type === 'solid_t') nodeType = 'solid';
    else if (obj.type === 'document_t') nodeType = 'folder';

    const entityId = normalizeEntityId(obj.entity_id);
    const stableId = entityId !== undefined ? `entity-${entityId.toString(16)}` : `path-${path}`;

    nodes.push({
      id: stableId,
      name: obj.name || obj.type || 'Unnamed',
      type: nodeType,
      depth: depth,
      entityId
    });

    // Recursively add children
    if (obj.children && Array.isArray(obj.children)) {
      for (let i = 0; i < obj.children.length; i++) {
        addNode(obj.children[i], depth + 1, `${path}-${i}`);
      }
    }
  }

  addNode(docJson, 0, '0');
  return nodes;
}
