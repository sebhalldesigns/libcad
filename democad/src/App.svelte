<script lang="ts">
  import { onDestroy, onMount } from 'svelte';
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
  const minTrayHeight = 180;
  const minViewportHeightWhenTrayMaxed = 5;
  let leftPaneWidth = 250;
  let rightPaneWidth = 292;
  let consoleHeight = 148;
  let consoleVisible = false;
  let activeTrayPanel: 'browser' | 'inspector' | 'console' = 'browser';
  let mobileTrayVisible = true;
  let mobileTrayHeight = 320;
  let coverWindow: 'settings' | 'help' | null = null;

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
    // Initial shell behavior: logging keeps the interaction flow testable
    // before command routing + WASM-backed operations are integrated.
    console.info(`[ribbon] action selected: ${actionId}`);
  }

  function handleAppLink(label: string): void {
    const normalized = label.trim().toLowerCase();
    if (normalized === 'settings' || normalized === 'help') {
      coverWindow = normalized;
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

  function clampMobileTrayHeight(): void {
    const { height } = getWorkspaceSize();
    const maxTrayHeight = Math.max(minTrayHeight, height - minViewportHeightWhenTrayMaxed);
    mobileTrayHeight = clamp(mobileTrayHeight, minTrayHeight, maxTrayHeight);
  }

  $: if (!consoleVisible && activeTrayPanel === 'console') {
    activeTrayPanel = 'browser';
  }

  onMount(() => {
    clampMobileTrayHeight();
    window.addEventListener('resize', clampMobileTrayHeight);

    smallViewportQuery = window.matchMedia('(max-width: 860px)');
    const handleSmallViewportChange = (event: MediaQueryListEvent | MediaQueryList): void => {
      if (event.matches) {
        mobileTrayVisible = false;
        setConsoleVisible(false);
      }
    };

    handleSmallViewportChange(smallViewportQuery);
    smallViewportQuery.addEventListener('change', handleSmallViewportChange);

    return () => {
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
          title="Workspace View"
          subtitle="Sketch + Scene Context"
          mode="scene"
          showHeader={false}
        />
      </div>

      <aside class="panel-left panel-floating desktop-panel">
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
                  {#each consoleMessages as entry}
                    <li class={`level-${entry.level}`}>
                      <span class="time">[{entry.time}]</span>
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
            <p>Settings panel placeholder. Add preferences and application options here.</p>
          {:else}
            <p>Help panel placeholder. Add docs links, shortcuts, and onboarding guidance here.</p>
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
