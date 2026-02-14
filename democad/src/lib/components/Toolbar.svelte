<!--
  Ribbon Toolbar Component

  Classic ribbon-style toolbar with skeumorphic design.
  Features gradients, shadows, and visual depth for character.
-->
<script lang="ts">
  import { currentTool, setTool, setStatus } from '../stores/appState';

  // Active ribbon tab
  let activeTab: 'home' | 'sketch' | 'model' | 'view' = 'home';

  /**
   * Handle file operations
   */
  function handleNew() {
    setStatus('New document created');
  }

  function handleOpen() {
    setStatus('Open file dialog...');
  }

  function handleSave() {
    setStatus('Saving document...');
  }

  function handleSaveAs() {
    setStatus('Save As dialog...');
  }

  function handleExport() {
    setStatus('Export dialog...');
  }

  /**
   * Handle edit operations
   */
  function handleUndo() {
    setStatus('Undo');
  }

  function handleRedo() {
    setStatus('Redo');
  }

  function handleCut() {
    setStatus('Cut');
  }

  function handleCopy() {
    setStatus('Copy');
  }

  function handlePaste() {
    setStatus('Paste');
  }

  /**
   * Handle sketch operations
   */
  function handleLine() {
    setTool('sketch');
    setStatus('Draw Line');
  }

  function handleRectangle() {
    setTool('sketch');
    setStatus('Draw Rectangle');
  }

  function handleCircle() {
    setTool('sketch');
    setStatus('Draw Circle');
  }

  function handleArc() {
    setTool('sketch');
    setStatus('Draw Arc');
  }

  /**
   * Handle view operations
   */
  function handleFitView() {
    setStatus('Fit view to window');
  }

  function handleZoomIn() {
    setStatus('Zoom in');
  }

  function handleZoomOut() {
    setStatus('Zoom out');
  }
</script>

<div class="ribbon">
  <!-- Ribbon Tabs -->
  <div class="ribbon-tabs">
    <button
      class="ribbon-tab"
      class:active={activeTab === 'home'}
      onclick={() => activeTab = 'home'}
    >
      Home
    </button>

    <button
      class="ribbon-tab"
      class:active={activeTab === 'sketch'}
      onclick={() => activeTab = 'sketch'}
    >
      Sketch
    </button>

    <button
      class="ribbon-tab"
      class:active={activeTab === 'model'}
      onclick={() => activeTab = 'model'}
    >
      Model
    </button>

    <button
      class="ribbon-tab"
      class:active={activeTab === 'view'}
      onclick={() => activeTab = 'view'}
    >
      View
    </button>
  </div>

  <!-- Ribbon Content -->
  <div class="ribbon-content">
    {#if activeTab === 'home'}
      <!-- HOME TAB -->
      <div class="ribbon-groups">
        <!-- File Group -->
        <div class="ribbon-group">
          <div class="group-tools">
            <button class="ribbon-btn large" onclick={handleNew} title="New (Ctrl+N)">
              <span class="icon">📄</span>
              <span class="label">New</span>
            </button>

            <div class="vertical-stack">
              <button class="ribbon-btn small" onclick={handleOpen} title="Open (Ctrl+O)">
                <span class="icon">📂</span>
                <span class="label">Open</span>
              </button>
              <button class="ribbon-btn small" onclick={handleSave} title="Save (Ctrl+S)">
                <span class="icon">💾</span>
                <span class="label">Save</span>
              </button>
            </div>

            <div class="vertical-stack">
              <button class="ribbon-btn small" onclick={handleSaveAs} title="Save As">
                <span class="icon">📝</span>
                <span class="label">Save As</span>
              </button>
              <button class="ribbon-btn small" onclick={handleExport} title="Export">
                <span class="icon">📤</span>
                <span class="label">Export</span>
              </button>
            </div>
          </div>
          <div class="group-label">File</div>
        </div>

        <div class="ribbon-separator"></div>

        <!-- Clipboard Group -->
        <div class="ribbon-group">
          <div class="group-tools">
            <button class="ribbon-btn medium" onclick={handleCut} title="Cut (Ctrl+X)">
              <span class="icon">✂️</span>
              <span class="label">Cut</span>
            </button>

            <button class="ribbon-btn medium" onclick={handleCopy} title="Copy (Ctrl+C)">
              <span class="icon">📋</span>
              <span class="label">Copy</span>
            </button>

            <button class="ribbon-btn medium" onclick={handlePaste} title="Paste (Ctrl+V)">
              <span class="icon">📌</span>
              <span class="label">Paste</span>
            </button>
          </div>
          <div class="group-label">Clipboard</div>
        </div>

        <div class="ribbon-separator"></div>

        <!-- History Group -->
        <div class="ribbon-group">
          <div class="group-tools">
            <button class="ribbon-btn medium" onclick={handleUndo} title="Undo (Ctrl+Z)">
              <span class="icon">↶</span>
              <span class="label">Undo</span>
            </button>

            <button class="ribbon-btn medium" onclick={handleRedo} title="Redo (Ctrl+Y)">
              <span class="icon">↷</span>
              <span class="label">Redo</span>
            </button>
          </div>
          <div class="group-label">History</div>
        </div>

        <div class="ribbon-separator"></div>

        <!-- Select Group -->
        <div class="ribbon-group">
          <div class="group-tools">
            <button
              class="ribbon-btn large"
              class:active={$currentTool === 'select'}
              onclick={() => setTool('select')}
              title="Select (Esc)"
            >
              <span class="icon large-icon">➤</span>
              <span class="label">Select</span>
            </button>
          </div>
          <div class="group-label">Selection</div>
        </div>
      </div>

    {:else if activeTab === 'sketch'}
      <!-- SKETCH TAB -->
      <div class="ribbon-groups">
        <!-- Create Group -->
        <div class="ribbon-group">
          <div class="group-tools">
            <button
              class="ribbon-btn large"
              class:active={$currentTool === 'sketch'}
              onclick={() => setTool('sketch')}
              title="New Sketch"
            >
              <span class="icon large-icon">✏️</span>
              <span class="label">Create<br>Sketch</span>
            </button>
          </div>
          <div class="group-label">Create</div>
        </div>

        <div class="ribbon-separator"></div>

        <!-- Draw Group -->
        <div class="ribbon-group">
          <div class="group-tools">
            <button class="ribbon-btn medium" onclick={handleLine} title="Line (L)">
              <span class="icon">📏</span>
              <span class="label">Line</span>
            </button>

            <button class="ribbon-btn medium" onclick={handleRectangle} title="Rectangle (R)">
              <span class="icon">▭</span>
              <span class="label">Rectangle</span>
            </button>

            <button class="ribbon-btn medium" onclick={handleCircle} title="Circle (C)">
              <span class="icon">⭕</span>
              <span class="label">Circle</span>
            </button>

            <button class="ribbon-btn medium" onclick={handleArc} title="Arc (A)">
              <span class="icon">⌒</span>
              <span class="label">Arc</span>
            </button>
          </div>
          <div class="group-label">Draw</div>
        </div>

        <div class="ribbon-separator"></div>

        <!-- Modify Group -->
        <div class="ribbon-group">
          <div class="group-tools">
            <button class="ribbon-btn medium" title="Trim">
              <span class="icon">✂️</span>
              <span class="label">Trim</span>
            </button>

            <button class="ribbon-btn medium" title="Extend">
              <span class="icon">↔️</span>
              <span class="label">Extend</span>
            </button>

            <button class="ribbon-btn medium" title="Offset">
              <span class="icon">⇄</span>
              <span class="label">Offset</span>
            </button>
          </div>
          <div class="group-label">Modify</div>
        </div>

        <div class="ribbon-separator"></div>

        <!-- Constrain Group -->
        <div class="ribbon-group">
          <div class="group-tools">
            <button class="ribbon-btn medium" title="Horizontal">
              <span class="icon">—</span>
              <span class="label">Horizontal</span>
            </button>

            <button class="ribbon-btn medium" title="Vertical">
              <span class="icon">|</span>
              <span class="label">Vertical</span>
            </button>

            <button class="ribbon-btn medium" title="Dimension">
              <span class="icon">↔</span>
              <span class="label">Dimension</span>
            </button>
          </div>
          <div class="group-label">Constrain</div>
        </div>
      </div>

    {:else if activeTab === 'model'}
      <!-- MODEL TAB -->
      <div class="ribbon-groups">
        <!-- Create Group -->
        <div class="ribbon-group">
          <div class="group-tools">
            <button
              class="ribbon-btn large"
              class:active={$currentTool === 'extrude'}
              onclick={() => setTool('extrude')}
              title="Extrude (E)"
            >
              <span class="icon large-icon">⬆️</span>
              <span class="label">Extrude</span>
            </button>

            <button
              class="ribbon-btn large"
              class:active={$currentTool === 'revolve'}
              onclick={() => setTool('revolve')}
              title="Revolve (R)"
            >
              <span class="icon large-icon">🔄</span>
              <span class="label">Revolve</span>
            </button>
          </div>
          <div class="group-label">Create</div>
        </div>

        <div class="ribbon-separator"></div>

        <!-- Modify Group -->
        <div class="ribbon-group">
          <div class="group-tools">
            <button class="ribbon-btn medium" title="Fillet">
              <span class="icon">⌒</span>
              <span class="label">Fillet</span>
            </button>

            <button class="ribbon-btn medium" title="Chamfer">
              <span class="icon">📐</span>
              <span class="label">Chamfer</span>
            </button>

            <button class="ribbon-btn medium" title="Shell">
              <span class="icon">🔳</span>
              <span class="label">Shell</span>
            </button>
          </div>
          <div class="group-label">Modify</div>
        </div>

        <div class="ribbon-separator"></div>

        <!-- Pattern Group -->
        <div class="ribbon-group">
          <div class="group-tools">
            <button class="ribbon-btn medium" title="Linear Pattern">
              <span class="icon">⫴</span>
              <span class="label">Linear</span>
            </button>

            <button class="ribbon-btn medium" title="Circular Pattern">
              <span class="icon">◎</span>
              <span class="label">Circular</span>
            </button>

            <button class="ribbon-btn medium" title="Mirror">
              <span class="icon">⇄</span>
              <span class="label">Mirror</span>
            </button>
          </div>
          <div class="group-label">Pattern</div>
        </div>
      </div>

    {:else if activeTab === 'view'}
      <!-- VIEW TAB -->
      <div class="ribbon-groups">
        <!-- Navigate Group -->
        <div class="ribbon-group">
          <div class="group-tools">
            <button
              class="ribbon-btn medium"
              class:active={$currentTool === 'pan'}
              onclick={() => setTool('pan')}
              title="Pan View"
            >
              <span class="icon">✋</span>
              <span class="label">Pan</span>
            </button>

            <button
              class="ribbon-btn medium"
              class:active={$currentTool === 'rotate'}
              onclick={() => setTool('rotate')}
              title="Rotate View"
            >
              <span class="icon">🔄</span>
              <span class="label">Rotate</span>
            </button>

            <button
              class="ribbon-btn medium"
              class:active={$currentTool === 'zoom'}
              onclick={() => setTool('zoom')}
              title="Zoom"
            >
              <span class="icon">🔍</span>
              <span class="label">Zoom</span>
            </button>
          </div>
          <div class="group-label">Navigate</div>
        </div>

        <div class="ribbon-separator"></div>

        <!-- Zoom Group -->
        <div class="ribbon-group">
          <div class="group-tools">
            <button class="ribbon-btn medium" onclick={handleFitView} title="Fit to Window (F)">
              <span class="icon">⛶</span>
              <span class="label">Fit</span>
            </button>

            <button class="ribbon-btn medium" onclick={handleZoomIn} title="Zoom In (+)">
              <span class="icon">🔍+</span>
              <span class="label">Zoom In</span>
            </button>

            <button class="ribbon-btn medium" onclick={handleZoomOut} title="Zoom Out (-)">
              <span class="icon">🔍-</span>
              <span class="label">Zoom Out</span>
            </button>
          </div>
          <div class="group-label">Zoom</div>
        </div>

        <div class="ribbon-separator"></div>

        <!-- Orientation Group -->
        <div class="ribbon-group">
          <div class="group-tools">
            <div class="vertical-stack">
              <button class="ribbon-btn small" title="Top View">
                <span class="icon">⬇️</span>
                <span class="label">Top</span>
              </button>
              <button class="ribbon-btn small" title="Front View">
                <span class="icon">⬅️</span>
                <span class="label">Front</span>
              </button>
            </div>

            <div class="vertical-stack">
              <button class="ribbon-btn small" title="Right View">
                <span class="icon">⬆️</span>
                <span class="label">Right</span>
              </button>
              <button class="ribbon-btn small" title="Isometric">
                <span class="icon">📦</span>
                <span class="label">Iso</span>
              </button>
            </div>
          </div>
          <div class="group-label">Orientation</div>
        </div>

        <div class="ribbon-separator"></div>

        <!-- Display Group -->
        <div class="ribbon-group">
          <div class="group-tools">
            <button class="ribbon-btn medium" title="Wireframe">
              <span class="icon">◻️</span>
              <span class="label">Wireframe</span>
            </button>

            <button class="ribbon-btn medium" title="Shaded">
              <span class="icon">◼️</span>
              <span class="label">Shaded</span>
            </button>

            <button class="ribbon-btn medium" title="Rendered">
              <span class="icon">🎨</span>
              <span class="label">Rendered</span>
            </button>
          </div>
          <div class="group-label">Display</div>
        </div>
      </div>
    {/if}
  </div>
</div>

<style>
  .ribbon {
    display: flex;
    flex-direction: column;
    background: var(--color-surface);
    border-bottom: 2px solid var(--color-border);
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.6);
    user-select: none;
  }

  /* Ribbon Tabs with Depth */
  .ribbon-tabs {
    display: flex;
    gap: 1px;
    padding: 0 6px;
    background: linear-gradient(180deg, #121216 0%, #0f0f13 100%);
    border-bottom: 1px solid var(--color-border);
  }

  .ribbon-tab {
    padding: 6px 18px;
    background: transparent;
    border: 1px solid transparent;
    border-bottom: none;
    color: var(--color-text-secondary);
    font-size: 12px;
    font-weight: 600;
    cursor: pointer;
    transition: all var(--transition-fast);
    position: relative;
    text-shadow: 0 1px 2px rgba(0, 0, 0, 0.8);
  }

  .ribbon-tab::before {
    content: '';
    position: absolute;
    bottom: 0;
    left: 0;
    right: 0;
    height: 3px;
    background: transparent;
    transition: background var(--transition-fast);
  }

  .ribbon-tab:hover {
    background: linear-gradient(180deg, rgba(255, 255, 255, 0.06) 0%, rgba(255, 255, 255, 0.02) 100%);
    color: var(--color-text);
  }

  .ribbon-tab.active {
    background: var(--color-surface);
    border-color: var(--color-border);
    border-bottom-color: transparent;
    color: var(--color-primary-light);
  }

  .ribbon-tab.active::before {
    background: var(--gradient-primary);
    box-shadow: 0 0 8px var(--color-primary-glow);
  }

  /* Ribbon Content with Texture */
  .ribbon-content {
    padding: 8px 10px;
    min-height: 90px;
    background: var(--color-surface);
    position: relative;
  }

  .ribbon-content::before {
    content: '';
    position: absolute;
    inset: 0;
    background:
      repeating-linear-gradient(
        90deg,
        transparent,
        transparent 2px,
        rgba(255, 255, 255, 0.01) 2px,
        rgba(255, 255, 255, 0.01) 4px
      );
    pointer-events: none;
  }

  .ribbon-groups {
    display: flex;
    gap: 6px;
    align-items: flex-start;
  }

  /* Ribbon Group */
  .ribbon-group {
    display: flex;
    flex-direction: column;
    gap: 5px;
  }

  .group-tools {
    display: flex;
    gap: 3px;
    align-items: flex-start;
  }

  .group-label {
    text-align: center;
    font-size: 9px;
    color: var(--color-text-secondary);
    text-transform: uppercase;
    letter-spacing: 0.5px;
    font-weight: 600;
    padding: 3px 0 2px;
    background: linear-gradient(180deg, transparent 0%, rgba(0, 0, 0, 0.2) 100%);
    border-top: 1px solid var(--color-border);
    text-shadow: 0 1px 1px rgba(0, 0, 0, 0.8);
  }

  /* Vertical Stack */
  .vertical-stack {
    display: flex;
    flex-direction: column;
    gap: 3px;
  }

  /* Ribbon Separator with Depth */
  .ribbon-separator {
    width: 1px;
    background: linear-gradient(180deg, transparent 0%, var(--color-border-light) 20%, var(--color-border-light) 80%, transparent 100%);
    box-shadow: 1px 0 0 rgba(255, 255, 255, 0.05);
    margin: 0 3px;
    align-self: stretch;
  }

  /* Ribbon Buttons with Skeumorphism */
  .ribbon-btn {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 3px;
    padding: 6px 10px;
    background: var(--gradient-button);
    border: 1px solid var(--color-border);
    border-radius: var(--radius-sm);
    color: var(--color-text);
    cursor: pointer;
    transition: all var(--transition-fast);
    text-align: center;
    box-shadow:
      0 1px 3px rgba(0, 0, 0, 0.4),
      inset 0 1px 0 rgba(255, 255, 255, 0.1);
    position: relative;
  }

  .ribbon-btn::before {
    content: '';
    position: absolute;
    inset: 0;
    background: var(--gradient-glass);
    border-radius: var(--radius-sm);
    opacity: 0;
    transition: opacity var(--transition-fast);
  }

  .ribbon-btn:hover {
    background: var(--gradient-button-hover);
    border-color: var(--color-border-light);
    box-shadow:
      0 2px 4px rgba(0, 0, 0, 0.5),
      inset 0 1px 0 rgba(255, 255, 255, 0.15);
    transform: translateY(-1px);
  }

  .ribbon-btn:hover::before {
    opacity: 1;
  }

  .ribbon-btn:active {
    transform: translateY(0);
    box-shadow:
      inset 0 1px 3px rgba(0, 0, 0, 0.6),
      0 1px 2px rgba(0, 0, 0, 0.3);
  }

  .ribbon-btn.active {
    background: var(--gradient-primary);
    color: white;
    border-color: var(--color-primary-dark);
    box-shadow:
      0 3px 8px var(--color-primary-glow),
      inset 0 1px 0 rgba(255, 255, 255, 0.3),
      inset 0 -1px 0 rgba(0, 0, 0, 0.2);
    text-shadow: 0 1px 2px rgba(0, 0, 0, 0.5);
  }

  .ribbon-btn.active::before {
    opacity: 0;
  }

  /* Button Sizes - Reduced Padding */
  .ribbon-btn.large {
    min-width: 64px;
    min-height: 70px;
    padding: 8px 10px;
  }

  .ribbon-btn.medium {
    min-width: 54px;
    min-height: 62px;
    padding: 6px 8px;
  }

  .ribbon-btn.small {
    min-width: 54px;
    min-height: 28px;
    flex-direction: row;
    justify-content: flex-start;
    gap: 5px;
    padding: 4px 8px;
  }

  /* Icons with Glow */
  .ribbon-btn .icon {
    font-size: 18px;
    line-height: 1;
    filter: drop-shadow(0 1px 2px rgba(0, 0, 0, 0.5));
  }

  .ribbon-btn.large .icon.large-icon {
    font-size: 26px;
  }

  .ribbon-btn.small .icon {
    font-size: 15px;
  }

  .ribbon-btn.active .icon {
    filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.6));
  }

  /* Labels */
  .ribbon-btn .label {
    font-size: 10px;
    font-weight: 600;
    line-height: 1.2;
    text-shadow: 0 1px 1px rgba(0, 0, 0, 0.6);
  }

  .ribbon-btn.small .label {
    font-size: 10px;
  }
</style>
