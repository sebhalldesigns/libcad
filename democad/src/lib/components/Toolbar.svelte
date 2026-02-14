<!--
  Toolbar Component

  Top toolbar containing common CAD operations and tool selection.
  Provides quick access to file operations, undo/redo, and drawing tools.
-->
<script lang="ts">
  import { currentTool, setTool, setStatus } from '../stores/appState';

  /**
   * Handle file operations
   */
  function handleNew() {
    setStatus('New document created');
    // TODO: Implement new document logic
  }

  function handleOpen() {
    setStatus('Open file dialog...');
    // TODO: Implement file open dialog
  }

  function handleSave() {
    setStatus('Saving document...');
    // TODO: Implement save functionality
  }

  /**
   * Handle edit operations
   */
  function handleUndo() {
    setStatus('Undo');
    // TODO: Implement undo functionality
  }

  function handleRedo() {
    setStatus('Redo');
    // TODO: Implement redo functionality
  }
</script>

<div class="toolbar">
  <!-- File Operations -->
  <div class="toolbar-group">
    <button
      class="toolbar-btn"
      title="New (Ctrl+N)"
      onclick={handleNew}
    >
      <span class="icon">📄</span>
      <span class="label">New</span>
    </button>

    <button
      class="toolbar-btn"
      title="Open (Ctrl+O)"
      onclick={handleOpen}
    >
      <span class="icon">📂</span>
      <span class="label">Open</span>
    </button>

    <button
      class="toolbar-btn"
      title="Save (Ctrl+S)"
      onclick={handleSave}
    >
      <span class="icon">💾</span>
      <span class="label">Save</span>
    </button>
  </div>

  <div class="separator"></div>

  <!-- Edit Operations -->
  <div class="toolbar-group">
    <button
      class="toolbar-btn"
      title="Undo (Ctrl+Z)"
      onclick={handleUndo}
    >
      <span class="icon">↶</span>
    </button>

    <button
      class="toolbar-btn"
      title="Redo (Ctrl+Y)"
      onclick={handleRedo}
    >
      <span class="icon">↷</span>
    </button>
  </div>

  <div class="separator"></div>

  <!-- Drawing Tools -->
  <div class="toolbar-group">
    <button
      class="toolbar-btn"
      class:active={$currentTool === 'select'}
      title="Select (Esc)"
      onclick={() => setTool('select')}
    >
      <span class="icon">➤</span>
      <span class="label">Select</span>
    </button>

    <button
      class="toolbar-btn"
      class:active={$currentTool === 'sketch'}
      title="Sketch (S)"
      onclick={() => setTool('sketch')}
    >
      <span class="icon">✏️</span>
      <span class="label">Sketch</span>
    </button>

    <button
      class="toolbar-btn"
      class:active={$currentTool === 'extrude'}
      title="Extrude (E)"
      onclick={() => setTool('extrude')}
    >
      <span class="icon">⬆️</span>
      <span class="label">Extrude</span>
    </button>

    <button
      class="toolbar-btn"
      class:active={$currentTool === 'revolve'}
      title="Revolve (R)"
      onclick={() => setTool('revolve')}
    >
      <span class="icon">🔄</span>
      <span class="label">Revolve</span>
    </button>
  </div>

  <div class="separator"></div>

  <!-- View Tools -->
  <div class="toolbar-group">
    <button
      class="toolbar-btn"
      class:active={$currentTool === 'pan'}
      title="Pan (Middle Mouse)"
      onclick={() => setTool('pan')}
    >
      <span class="icon">✋</span>
      <span class="label">Pan</span>
    </button>

    <button
      class="toolbar-btn"
      class:active={$currentTool === 'rotate'}
      title="Rotate View"
      onclick={() => setTool('rotate')}
    >
      <span class="icon">🔄</span>
      <span class="label">Rotate</span>
    </button>

    <button
      class="toolbar-btn"
      class:active={$currentTool === 'zoom'}
      title="Zoom (Mouse Wheel)"
      onclick={() => setTool('zoom')}
    >
      <span class="icon">🔍</span>
      <span class="label">Zoom</span>
    </button>
  </div>
</div>

<style>
  .toolbar {
    display: flex;
    align-items: center;
    gap: 4px;
    padding: 8px 12px;
    background: var(--color-surface);
    border-bottom: 1px solid var(--color-border);
    height: 52px;
    box-sizing: border-box;
  }

  .toolbar-group {
    display: flex;
    gap: 4px;
  }

  .separator {
    width: 1px;
    height: 32px;
    background: var(--color-border);
    margin: 0 8px;
  }

  .toolbar-btn {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 2px;
    padding: 6px 12px;
    background: transparent;
    border: 1px solid transparent;
    border-radius: 4px;
    color: var(--color-text);
    cursor: pointer;
    transition: all 0.15s ease;
    font-size: 11px;
    min-width: 60px;
  }

  .toolbar-btn:hover {
    background: var(--color-hover);
    border-color: var(--color-border);
  }

  .toolbar-btn.active {
    background: var(--color-primary);
    color: var(--color-bg);
    border-color: var(--color-primary);
  }

  .toolbar-btn .icon {
    font-size: 16px;
    line-height: 1;
  }

  .toolbar-btn .label {
    font-size: 10px;
    font-weight: 500;
    text-transform: uppercase;
    letter-spacing: 0.3px;
  }
</style>
