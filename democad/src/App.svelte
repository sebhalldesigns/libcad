<!--
  Main Application Component

  Root component that sets up the overall layout structure.
  Implements a classic CAD application layout with:
  - Top toolbar for common operations
  - Left sidebar for model tree/hierarchy
  - Center viewport for 3D rendering
  - Right panel for properties
  - Bottom status bar
-->
<script lang="ts">
  import Toolbar from './lib/components/Toolbar.svelte';
  import Sidebar from './lib/components/Sidebar.svelte';
  import Viewport from './lib/components/Viewport.svelte';
  import PropertiesPanel from './lib/components/PropertiesPanel.svelte';
  import StatusBar from './lib/components/StatusBar.svelte';
  import { panelVisibility } from './lib/stores/appState';

  // Panel resize state
  let sidebarWidth = 250;
  let propertiesWidth = 280;
  let isResizingSidebar = false;
  let isResizingProperties = false;

  /**
   * Handle sidebar resize drag
   */
  function startSidebarResize() {
    isResizingSidebar = true;
  }

  /**
   * Handle properties panel resize drag
   */
  function startPropertiesResize() {
    isResizingProperties = true;
  }

  /**
   * Handle global mouse move for resizing
   */
  function handleGlobalMouseMove(e: MouseEvent) {
    if (isResizingSidebar) {
      sidebarWidth = Math.max(200, Math.min(400, e.clientX));
    }
    if (isResizingProperties) {
      propertiesWidth = Math.max(200, Math.min(400, window.innerWidth - e.clientX));
    }
  }

  /**
   * Handle global mouse up to stop resizing
   */
  function handleGlobalMouseUp() {
    isResizingSidebar = false;
    isResizingProperties = false;
  }
</script>

<svelte:window
  onmousemove={handleGlobalMouseMove}
  onmouseup={handleGlobalMouseUp}
/>

<div class="app-container">
  <!-- Top Toolbar -->
  <Toolbar />

  <!-- Main Content Area -->
  <div class="main-content">
    <!-- Left Sidebar -->
    {#if $panelVisibility.leftSidebar}
      <div
        class="sidebar-container"
        style="width: {sidebarWidth}px;"
      >
        <Sidebar />

        <!-- Resize Handle -->
        <div
          class="resize-handle resize-handle-right"
          role="separator"
          aria-orientation="vertical"
          onmousedown={startSidebarResize}
        ></div>
      </div>
    {/if}

    <!-- Center Viewport -->
    <div class="viewport-container">
      <Viewport />
    </div>

    <!-- Right Properties Panel -->
    {#if $panelVisibility.rightPanel}
      <div
        class="properties-container"
        style="width: {propertiesWidth}px;"
      >
        <!-- Resize Handle -->
        <div
          class="resize-handle resize-handle-left"
          role="separator"
          aria-orientation="vertical"
          onmousedown={startPropertiesResize}
        ></div>

        <PropertiesPanel />
      </div>
    {/if}
  </div>

  <!-- Bottom Status Bar -->
  {#if $panelVisibility.statusBar}
    <StatusBar />
  {/if}
</div>

<style>
  .app-container {
    display: flex;
    flex-direction: column;
    width: 100%;
    height: 100vh;
    background: var(--color-bg);
    overflow: hidden;
  }

  .main-content {
    display: flex;
    flex: 1;
    overflow: hidden;
    position: relative;
  }

  .sidebar-container {
    position: relative;
    min-width: 200px;
    max-width: 400px;
    height: 100%;
    display: flex;
    flex-shrink: 0;
  }

  .viewport-container {
    flex: 1;
    min-width: 400px;
    height: 100%;
    position: relative;
  }

  .properties-container {
    position: relative;
    min-width: 200px;
    max-width: 400px;
    height: 100%;
    display: flex;
    flex-shrink: 0;
  }

  /* Resize Handles */
  .resize-handle {
    position: absolute;
    top: 0;
    bottom: 0;
    width: 4px;
    cursor: col-resize;
    z-index: 10;
    transition: background-color var(--transition-fast);
  }

  .resize-handle-right {
    right: 0;
  }

  .resize-handle-left {
    left: 0;
  }

  .resize-handle:hover {
    background-color: var(--color-primary);
  }

  .resize-handle:active {
    background-color: var(--color-primary-dark);
  }

  /* Accessibility */
  .resize-handle:focus-visible {
    outline: 2px solid var(--color-primary);
    outline-offset: -1px;
  }
</style>
