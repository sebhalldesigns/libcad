<script lang="ts">
  import { onDestroy } from 'svelte';
  import Ribbon from './lib/ui/components/Ribbon.svelte';
  import WorkbenchPane from './lib/ui/components/WorkbenchPane.svelte';
  import ViewportPlaceholder from './lib/ui/components/ViewportPlaceholder.svelte';
  import {
    consoleMessages,
    inspectorFields,
    projectTree,
    ribbonTabs,
    type ProjectNode
  } from './lib/ui/model';

  let activeTabId = ribbonTabs[0].id;
  let selectedNodeId = projectTree.find((node) => node.active)?.id ?? projectTree[0].id;
  let workspaceEl: HTMLElement | null = null;
  const appLinks = [
    { label: 'Settings' },
    { label: 'Help' }
  ];
  const minSidePane = 180;
  const minConsoleHeight = 96;
  const consoleCloseThreshold = 44;
  const minViewportWidth = 320;
  const minViewportHeight = 180;
  let leftPaneWidth = 250;
  let rightPaneWidth = 292;
  let consoleHeight = 148;
  let consoleVisible = true;

  type DragMode = 'left' | 'right' | 'console' | null;
  let dragMode: DragMode = null;
  let dragStartX = 0;
  let dragStartY = 0;
  let dragStartLeft = 0;
  let dragStartRight = 0;
  let dragStartConsole = 0;

  const nodeIcon: Record<ProjectNode['type'], string> = {
    folder: '+',
    sketch: '/',
    solid: '#',
    operation: '*'
  };

  function handleRibbonAction(actionId: string): void {
    // Initial shell behavior: logging keeps the interaction flow testable
    // before command routing + WASM-backed operations are integrated.
    console.info(`[ribbon] action selected: ${actionId}`);
  }

  function handleAppLink(label: string): void {
    console.info(`[app] command selected: ${label}`);
  }

  function clamp(value: number, min: number, max: number): number {
    return Math.max(min, Math.min(max, value));
  }

  function getWorkspaceSize(): { width: number; height: number } {
    const width = workspaceEl?.clientWidth ?? window.innerWidth;
    const height = workspaceEl?.clientHeight ?? window.innerHeight;
    return { width, height };
  }

  function onDragMove(event: PointerEvent): void {
    if (!dragMode) return;

    const { width, height } = getWorkspaceSize();

    if (dragMode === 'left') {
      const maxLeft = Math.max(minSidePane, width - rightPaneWidth - minViewportWidth);
      leftPaneWidth = clamp(dragStartLeft + (event.clientX - dragStartX), minSidePane, maxLeft);
      return;
    }

    if (dragMode === 'right') {
      const maxRight = Math.max(minSidePane, width - leftPaneWidth - minViewportWidth);
      rightPaneWidth = clamp(dragStartRight - (event.clientX - dragStartX), minSidePane, maxRight);
      return;
    }

    const maxConsole = Math.max(minConsoleHeight, height - minViewportHeight);
    const rawConsoleHeight = dragStartConsole - (event.clientY - dragStartY);
    consoleHeight = clamp(rawConsoleHeight, 0, maxConsole);
  }

  function stopDrag(): void {
    if (dragMode === 'console' && consoleVisible) {
      const { height } = getWorkspaceSize();
      const maxConsole = Math.max(minConsoleHeight, height - minViewportHeight);
      if (consoleHeight <= consoleCloseThreshold) {
        setConsoleVisible(false);
      } else {
        consoleHeight = clamp(consoleHeight, minConsoleHeight, maxConsole);
      }
    }

    dragMode = null;
    document.body.style.userSelect = '';
    document.body.style.cursor = '';
    window.removeEventListener('pointermove', onDragMove);
    window.removeEventListener('pointerup', stopDrag);
  }

  function startDrag(event: PointerEvent, mode: Exclude<DragMode, null>): void {
    dragMode = mode;
    dragStartX = event.clientX;
    dragStartY = event.clientY;
    dragStartLeft = leftPaneWidth;
    dragStartRight = rightPaneWidth;
    dragStartConsole = consoleHeight;

    document.body.style.userSelect = 'none';
    document.body.style.cursor = mode === 'console' ? 'row-resize' : 'col-resize';
    window.addEventListener('pointermove', onDragMove);
    window.addEventListener('pointerup', stopDrag);
  }

  function setConsoleVisible(visible: boolean): void {
    consoleVisible = visible;
    if (!visible && dragMode === 'console') {
      stopDrag();
    }
    if (visible && consoleHeight < minConsoleHeight) {
      consoleHeight = 148;
    }
  }

  function toggleConsole(): void {
    setConsoleVisible(!consoleVisible);
  }

  onDestroy(() => {
    stopDrag();
  });
</script>

<div class="cad-app">
  <Ribbon
    tabs={ribbonTabs}
    {activeTabId}
    {appLinks}
    onAppLink={handleAppLink}
    onTabChange={(tabId) => (activeTabId = tabId)}
    onAction={handleRibbonAction}
  />

  <main class="workbench">
    <section
      class="workspace-stack"
      bind:this={workspaceEl}
      style={`--left-pane-w:${leftPaneWidth}px; --right-pane-w:${rightPaneWidth}px; --console-h:${consoleHeight}px;`}
    >
      <div class="panel-center">
        <ViewportPlaceholder
          title="Workspace View"
          subtitle="Sketch + Scene Context"
          mode="scene"
          showHeader={false}
        />
      </div>

      <aside class="panel-left panel-floating">
        <WorkbenchPane title="Browser">
          <ul class="tree">
            {#each projectTree as node}
              <li>
                <button
                  class="tree-node"
                  class:selected={node.id === selectedNodeId}
                  style={`--depth:${node.depth}`}
                  on:click={() => (selectedNodeId = node.id)}
                >
                  <span class="glyph" aria-hidden="true">{nodeIcon[node.type]}</span>
                  <span>{node.name}</span>
                </button>
              </li>
            {/each}
          </ul>
        </WorkbenchPane>
      </aside>

      <aside class="panel-right panel-floating">
        <WorkbenchPane title="Inspector">
          <ul class="inspector-list">
            {#each inspectorFields as field}
              <li>
                <span>{field.label}</span>
                <button class="value" class:editable={field.editable}>{field.value}</button>
              </li>
            {/each}
          </ul>
        </WorkbenchPane>
      </aside>

      {#if consoleVisible}
        <section class="panel-bottom panel-floating">
          <WorkbenchPane title="Console" compact={true}>
            <ul class="console-log">
              {#each consoleMessages as entry}
                <li class={`level-${entry.level}`}>
                  <span class="time">[{entry.time}]</span>
                  <span>{entry.text}</span>
                </li>
              {/each}
            </ul>
          </WorkbenchPane>
        </section>
      {/if}

      <div
        class="splitter splitter-left"
        role="separator"
        aria-label="Resize Browser panel"
        aria-orientation="vertical"
        on:pointerdown={(event) => startDrag(event, 'left')}
      ></div>
      <div
        class="splitter splitter-right"
        role="separator"
        aria-label="Resize Inspector panel"
        aria-orientation="vertical"
        on:pointerdown={(event) => startDrag(event, 'right')}
      ></div>
      {#if consoleVisible}
        <div
          class="splitter splitter-bottom"
          role="separator"
          aria-label="Resize Console panel"
          aria-orientation="horizontal"
          on:pointerdown={(event) => startDrag(event, 'console')}
        ></div>
      {/if}
    </section>
  </main>

  <footer class="statusbar" role="status" aria-label="Application status">
    <span class="status-text">democad</span>
    <button
      class="status-toggle"
      type="button"
      aria-pressed={consoleVisible}
      on:click={toggleConsole}
    >
      {consoleVisible ? 'Hide Console' : 'Show Console'}
    </button>
  </footer>
</div>
