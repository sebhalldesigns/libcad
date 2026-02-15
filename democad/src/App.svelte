<script lang="ts">
  import { onDestroy, onMount } from 'svelte';
  import Ribbon from './lib/ui/components/Ribbon.svelte';
  import WorkbenchPane from './lib/ui/components/WorkbenchPane.svelte';
  import ViewportPlaceholder from './lib/ui/components/ViewportPlaceholder.svelte';
  import {
    addConsoleMessage,
    consoleMessages,
    inspectorFields,
    ribbonTabs,
    convertDocumentToTree,
    type ProjectNode
  } from './lib/ui/model';

  let activeTabId = ribbonTabs[0].id;
  let projectTree: ProjectNode[] = [];
  let selectedNodeId = '';
  let hoveredNodeId: string | null = null;
  const INVALID_ENTITY_ID = 0xFFFFFFFF;
  const PLANE_ENTITY_TYPE_MASK = 0xF0000000;
  const PLANE_ENTITY_TYPE_VALUE = 0x20000000;
  let selectedEntityId = INVALID_ENTITY_ID;
  let hoveredEntityId = INVALID_ENTITY_ID;
  let nodeIdToEntity = new Map<string, number>();
  let entityToNodeId = new Map<number, string>();
  let workspaceEl: HTMLElement | null = null;
  let viewportRef: any = null;
  const appLinks = [
    { label: 'Settings' },
    { label: 'Help' }
  ];
  const minSidePane = 180;
  const minConsoleHeight = 96;
  const consoleCloseThreshold = 44;
  const minViewportWidth = 320;
  const minViewportHeight = 180;
  const minTrayHeight = 180;
  const minViewportHeightWhenTrayMaxed = 5;
  let leftPaneWidth = 250;
  let rightPaneWidth = 292;
  let consoleHeight = 148;
  let consoleVisible = true;
  let activeTrayPanel: 'browser' | 'inspector' | 'console' = 'browser';
  let mobileTrayVisible = false;
  let mobileTrayHeight = 320;
  let coverWindow: 'settings' | 'help' | null = null;
  let settingsTab: 'general' | 'appearance' | 'debug' = 'general';
  let helpTab: 'about' | 'docs' = 'about';

  type DragMode = 'left' | 'right' | 'console' | 'tray' | null;
  let dragMode: DragMode = null;
  let dragStartX = 0;
  let dragStartY = 0;
  let dragStartLeft = 0;
  let dragStartRight = 0;
  let dragStartConsole = 0;
  let dragStartTray = 0;
  let smallViewportQuery: MediaQueryList | null = null;

  const nodeIcon: Record<ProjectNode['type'], string> = {
    folder: '+',
    sketch: '/',
    solid: '#',
    operation: '*'
  };

  function handleRibbonAction(actionId: string): void {
    if (actionId === 'create-sketch') {
      if (!isPlaneEntityId(selectedEntityId)) {
        addConsoleMessage('warn', 'Select a plane in the scene or browser, then click Sketch.');
        return;
      }

      const created = viewportRef?.createSketchOnPlane?.(selectedEntityId);
      if (created) {
        addConsoleMessage('ok', 'Created sketch on selected plane.');
        updateProjectTree();
      } else {
        addConsoleMessage('warn', 'Failed to create sketch on selected plane.');
      }
      return;
    }

    console.info(`[ribbon] action selected: ${actionId}`);
  }

  function handleAppLink(label: string): void {
    const normalized = label.trim().toLowerCase();
    if (normalized === 'settings') {
      coverWindow = 'settings';
      settingsTab = 'general';
      return;
    }
    if (normalized === 'help') {
      coverWindow = 'help';
      helpTab = 'about';
      return;
    }
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

    if (dragMode === 'console') {
      const maxConsole = Math.max(minConsoleHeight, height - minViewportHeight);
      const rawConsoleHeight = dragStartConsole - (event.clientY - dragStartY);
      consoleHeight = clamp(rawConsoleHeight, 0, maxConsole);
      return;
    }

    const maxTrayHeight = Math.max(minTrayHeight, height - minViewportHeightWhenTrayMaxed);
    const rawTrayHeight = dragStartTray - (event.clientY - dragStartY);
    mobileTrayHeight = clamp(rawTrayHeight, minTrayHeight, maxTrayHeight);
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
    dragStartTray = mobileTrayHeight;

    document.body.style.userSelect = 'none';
    document.body.style.cursor = mode === 'console' || mode === 'tray' ? 'row-resize' : 'col-resize';
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

  function closeCoverWindow(): void {
    coverWindow = null;
  }

  function onConsoleCheckboxChange(event: Event): void {
    const target = event.currentTarget as HTMLInputElement | null;
    if (!target) return;
    setConsoleVisible(target.checked);
  }

  function clampMobileTrayHeight(): void {
    const { height } = getWorkspaceSize();
    const maxTrayHeight = Math.max(minTrayHeight, height - minViewportHeightWhenTrayMaxed);
    mobileTrayHeight = clamp(mobileTrayHeight, minTrayHeight, maxTrayHeight);
  }

  function normalizeEntityId(raw: unknown): number {
    if (typeof raw !== 'number' || !Number.isFinite(raw)) return INVALID_ENTITY_ID;
    const value = raw >>> 0;
    if (value === 0 || value === INVALID_ENTITY_ID) return INVALID_ENTITY_ID;
    return value;
  }

  function isPlaneEntityId(entityId: number): boolean {
    if (entityId === INVALID_ENTITY_ID) return false;
    return (entityId & PLANE_ENTITY_TYPE_MASK) === PLANE_ENTITY_TYPE_VALUE;
  }

  function rebuildEntityMaps(): void {
    const nextNodeToEntity = new Map<string, number>();
    const nextEntityToNode = new Map<number, string>();

    for (const node of projectTree) {
      if (typeof node.entityId === 'number') {
        const entityId = normalizeEntityId(node.entityId);
        if (entityId !== INVALID_ENTITY_ID) {
          nextNodeToEntity.set(node.id, entityId);
          nextEntityToNode.set(entityId, node.id);
        }
      }
    }

    nodeIdToEntity = nextNodeToEntity;
    entityToNodeId = nextEntityToNode;

    if (selectedEntityId !== INVALID_ENTITY_ID) {
      selectedNodeId = entityToNodeId.get(selectedEntityId) ?? selectedNodeId;
    } else if (selectedNodeId && !projectTree.some((node) => node.id === selectedNodeId)) {
      selectedNodeId = projectTree[0]?.id ?? '';
    } else if (!selectedNodeId && projectTree.length > 0) {
      selectedNodeId = projectTree[0].id;
    }

    if (hoveredEntityId !== INVALID_ENTITY_ID) {
      hoveredNodeId = entityToNodeId.get(hoveredEntityId) ?? null;
    } else {
      hoveredNodeId = null;
    }
  }

  $: if (!consoleVisible && activeTrayPanel === 'console') {
    activeTrayPanel = 'browser';
  }

  function updateProjectTree(): void {
    if (!viewportRef?.getDocumentJSON) return;
    const docJson = viewportRef.getDocumentJSON();
    applyDocumentJson(docJson);
  }

  function applyDocumentJson(docJson: unknown): void {
    if (!docJson) return;
    const newTree = convertDocumentToTree(docJson);
    if (newTree.length > 0) {
      projectTree = newTree;
      rebuildEntityMaps();
    }
  }

  function handleNodeClick(nodeId: string): void {
    selectedNodeId = nodeId;
    const entityId = nodeIdToEntity.get(nodeId) ?? INVALID_ENTITY_ID;
    selectedEntityId = entityId;
    viewportRef?.selectEntity?.(entityId);
  }

  function handleNodeHover(nodeId: string): void {
    hoveredNodeId = nodeId;
    const entityId = nodeIdToEntity.get(nodeId) ?? INVALID_ENTITY_ID;
    hoveredEntityId = entityId;
    viewportRef?.setHoveredEntity?.(entityId);
  }

  function clearNodeHover(): void {
    hoveredNodeId = null;
    hoveredEntityId = INVALID_ENTITY_ID;
    viewportRef?.setHoveredEntity?.(INVALID_ENTITY_ID);
  }

  function handleEntitySelected(event: CustomEvent): void {
    const entityId = normalizeEntityId(event.detail?.entityId);
    selectedEntityId = entityId;
    if (entityId === INVALID_ENTITY_ID) {
      selectedNodeId = '';
      return;
    }
    selectedNodeId = entityToNodeId.get(entityId) ?? selectedNodeId;
  }

  function handleEntityHovered(event: CustomEvent): void {
    const entityId = normalizeEntityId(event.detail?.entityId);
    hoveredEntityId = entityId;
    hoveredNodeId = entityId === INVALID_ENTITY_ID ? null : (entityToNodeId.get(entityId) ?? null);
  }

  function handleDocumentUpdated(event: CustomEvent): void {
    applyDocumentJson(event.detail?.docJson);
  }

  onMount(() => {
    rebuildEntityMaps();
    clampMobileTrayHeight();
    window.addEventListener('resize', clampMobileTrayHeight);

    // Listen for entity hover/selection events from viewport
    window.addEventListener('cad-entity-selected', handleEntitySelected as EventListener);
    window.addEventListener('cad-entity-hovered', handleEntityHovered as EventListener);
    window.addEventListener('cad-document-updated', handleDocumentUpdated as EventListener);

    // Update project tree after a delay to ensure WASM is loaded
    setTimeout(updateProjectTree, 1000);
    // Poll for updates (temporary - should be event-driven in production)
    const interval = setInterval(updateProjectTree, 5000);

    smallViewportQuery = window.matchMedia('(max-width: 860px)');
    const handleSmallViewportChange = (event: MediaQueryListEvent | MediaQueryList): void => {
      if (event.matches) {
        // Keep console visible on mobile for debugging
        mobileTrayVisible = false;
        // Don't hide console: setConsoleVisible(false);
      }
    };

    handleSmallViewportChange(smallViewportQuery);
    smallViewportQuery.addEventListener('change', handleSmallViewportChange);

    return () => {
      clearInterval(interval);
      window.removeEventListener('cad-entity-selected', handleEntitySelected as EventListener);
      window.removeEventListener('cad-entity-hovered', handleEntityHovered as EventListener);
      window.removeEventListener('cad-document-updated', handleDocumentUpdated as EventListener);
      smallViewportQuery?.removeEventListener('change', handleSmallViewportChange);
      window.removeEventListener('resize', clampMobileTrayHeight);
    };
  });

  onDestroy(() => {
    stopDrag();
  });
</script>

<div class="cad-app" class:tray-hidden-mobile={!mobileTrayVisible}>
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
      class:tray-visible={mobileTrayVisible}
      bind:this={workspaceEl}
      style={`--left-pane-w:${leftPaneWidth}px; --right-pane-w:${rightPaneWidth}px; --console-h:${consoleHeight}px; --tray-h:${mobileTrayHeight}px;`}
    >
      <div class="panel-center">
        <ViewportPlaceholder
          bind:this={viewportRef}
          title="Workspace View"
          subtitle="Sketch + Scene Context"
          mode="scene"
          showHeader={false}
        />
      </div>

      <aside class="panel-left panel-floating desktop-panel">
        <WorkbenchPane title="Browser">
          <ul class="tree" on:mouseleave={clearNodeHover}>
            {#each projectTree as node}
              <li>
                <button
                  class="tree-node"
                  class:selected={node.id === selectedNodeId}
                  class:hovered={node.id === hoveredNodeId}
                  style={`--depth:${node.depth}`}
                  on:click={() => handleNodeClick(node.id)}
                  on:mouseenter={() => handleNodeHover(node.id)}
                >
                  <span class="glyph" aria-hidden="true">{nodeIcon[node.type]}</span>
                  <span>{node.name}</span>
                </button>
              </li>
            {/each}
          </ul>
        </WorkbenchPane>
      </aside>

      <aside class="panel-right panel-floating desktop-panel">
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
        <section class="panel-bottom panel-floating desktop-panel">
          <WorkbenchPane title="Console" compact={true}>
            <ul class="console-log">
              {#each $consoleMessages as entry}
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
        class="splitter splitter-left desktop-panel"
        role="separator"
        aria-label="Resize Browser panel"
        aria-orientation="vertical"
        on:pointerdown={(event) => startDrag(event, 'left')}
      ></div>
      <div
        class="splitter splitter-right desktop-panel"
        role="separator"
        aria-label="Resize Inspector panel"
        aria-orientation="vertical"
        on:pointerdown={(event) => startDrag(event, 'right')}
      ></div>
      {#if consoleVisible}
        <div
          class="splitter splitter-bottom desktop-panel"
          role="separator"
          aria-label="Resize Console panel"
          aria-orientation="horizontal"
          on:pointerdown={(event) => startDrag(event, 'console')}
        ></div>
      {/if}

      {#if mobileTrayVisible}
        <section class="mobile-tray" aria-label="Workspace panels">
          <button
            class="tray-grip"
            type="button"
            aria-label="Resize panel tray"
            on:pointerdown={(event) => startDrag(event, 'tray')}
          ></button>
          <nav class="tray-tabs" aria-label="Panel tray">
            <button
              class="tray-tab"
              class:active={activeTrayPanel === 'browser'}
              type="button"
              on:click={() => (activeTrayPanel = 'browser')}
            >
              Browser
            </button>
            <button
              class="tray-tab"
              class:active={activeTrayPanel === 'inspector'}
              type="button"
              on:click={() => (activeTrayPanel = 'inspector')}
            >
              Inspector
            </button>
            {#if consoleVisible}
              <button
                class="tray-tab"
                class:active={activeTrayPanel === 'console'}
                type="button"
                on:click={() => (activeTrayPanel = 'console')}
            >
              Console
            </button>
            {/if}
          </nav>
          <button class="panel-close tray-close tray-close-overlay" type="button" on:click={() => (mobileTrayVisible = false)}>
            Close
          </button>

          <div class="tray-body">
            {#if activeTrayPanel === 'browser'}
              <WorkbenchPane title="Browser">
                <ul class="tree" on:mouseleave={clearNodeHover}>
                  {#each projectTree as node}
                    <li>
                      <button
                        class="tree-node"
                        class:selected={node.id === selectedNodeId}
                        class:hovered={node.id === hoveredNodeId}
                        style={`--depth:${node.depth}`}
                        on:click={() => handleNodeClick(node.id)}
                        on:mouseenter={() => handleNodeHover(node.id)}
                      >
                        <span class="glyph" aria-hidden="true">{nodeIcon[node.type]}</span>
                        <span>{node.name}</span>
                      </button>
                    </li>
                  {/each}
                </ul>
              </WorkbenchPane>
            {:else if activeTrayPanel === 'inspector'}
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
            {:else if consoleVisible}
              <WorkbenchPane title="Console" compact={true}>
                <ul class="console-log">
                  {#each $consoleMessages as entry}
                    <li class={`level-${entry.level}`}>
                      <span>{entry.text}</span>
                    </li>
                  {/each}
                </ul>
              </WorkbenchPane>
            {/if}
          </div>
        </section>
      {/if}
    </section>
  </main>

  {#if coverWindow}
    <section class="cover-backdrop" aria-hidden="true">
      <div
        class="cover-window"
        role="dialog"
        aria-modal="true"
        aria-label={coverWindow === 'settings' ? 'Settings' : 'Help'}
      >
        <header class="cover-header">
          <h2>{coverWindow === 'settings' ? 'Settings' : 'Help'}</h2>
          <button class="panel-close cover-close" type="button" on:click={closeCoverWindow}>Close</button>
        </header>
        <div class="cover-content">
          {#if coverWindow === 'settings'}
            <nav class="cover-tab-row" aria-label="Settings sections">
              <button
                class="cover-tab"
                class:active={settingsTab === 'general'}
                type="button"
                on:click={() => (settingsTab = 'general')}
              >
                General
              </button>
              <button
                class="cover-tab"
                class:active={settingsTab === 'appearance'}
                type="button"
                on:click={() => (settingsTab = 'appearance')}
              >
                Appearance
              </button>
              <button
                class="cover-tab"
                class:active={settingsTab === 'debug'}
                type="button"
                on:click={() => (settingsTab = 'debug')}
              >
                Debug
              </button>
            </nav>
            <section class="cover-panel">
              {#if settingsTab === 'general'}
                <h3>Project Defaults</h3>
                <p>Example preferences for units, autosave interval, and startup template.</p>
              {:else if settingsTab === 'appearance'}
                <h3>Appearance</h3>
                <p>Example preferences for theme accents, panel density, and icon scale.</p>
              {:else}
                <h3>Debug</h3>
                <label class="setting-check">
                  <input type="checkbox" checked={consoleVisible} on:change={onConsoleCheckboxChange} />
                  <span>Show console</span>
                </label>
                <p class="setting-note">Use this to reveal or hide the console panel.</p>
              {/if}
            </section>
          {:else}
            <nav class="cover-tab-row" aria-label="Help sections">
              <button class="cover-tab" class:active={helpTab === 'about'} type="button" on:click={() => (helpTab = 'about')}>
                About
              </button>
              <button class="cover-tab" class:active={helpTab === 'docs'} type="button" on:click={() => (helpTab = 'docs')}>
                Docs
              </button>
            </nav>
            <section class="cover-panel" class:docs-panel={helpTab === 'docs'}>
              {#if helpTab === 'about'}
                <div class="about-box">
                  <h3>democad</h3>
                  <p class="version">Version 0.1.0-dev</p>
                  <p>
                    democad is an example implementation of the libcad project, 
                    set up to feel like a conventional parametric CAD application, but as a website!
                  </p>
                </div>
              {:else}
                <iframe class="docs-frame" src="https://libcad.org/docs" title="libcad documentation"></iframe>
              {/if}
            </section>
          {/if}
        </div>
      </div>
    </section>
  {/if}

  <footer class="statusbar" role="status" aria-label="Application status">
    <span class="status-text">democad</span>
    <button
      class="status-toggle console-toggle"
      type="button"
      aria-pressed={consoleVisible}
      on:click={toggleConsole}
    >
      {consoleVisible ? 'Hide Console' : 'Show Console'}
    </button>
    <button class="status-toggle show-panels-toggle" type="button" on:click={() => (mobileTrayVisible = true)}>
      Show Panels
    </button>
  </footer>
</div>
