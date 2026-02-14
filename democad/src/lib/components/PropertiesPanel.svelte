<!--
  Properties Panel Component

  Right-side panel displaying properties and parameters of the selected object.
  Allows users to inspect and modify feature parameters.
-->
<script lang="ts">
  import { selectedItem, objectProperties, hasSelection } from '../stores/appState';

  // Mock properties for demonstration
  // TODO: Replace with actual object properties from WASM
  const mockProperties = {
    name: 'Sketch 1',
    type: 'Sketch',
    parameters: {
      plane: 'XY',
      offset: 0,
      locked: false
    }
  };

  /**
   * Handle property value change
   */
  function handlePropertyChange(key: string, value: any) {
    // TODO: Update property in WASM and trigger re-render
    console.log(`Property changed: ${key} = ${value}`);
  }
</script>

<div class="properties-panel">
  {#if $hasSelection}
    <div class="panel-header">
      <h3>Properties</h3>
    </div>

    <div class="panel-content">
      <!-- Object Name -->
      <div class="property-group">
        <label class="property-label">Name</label>
        <input
          type="text"
          class="property-input"
          value={mockProperties.name}
          onchange={(e) => handlePropertyChange('name', e.currentTarget.value)}
        />
      </div>

      <!-- Object Type -->
      <div class="property-group">
        <label class="property-label">Type</label>
        <div class="property-value readonly">
          {mockProperties.type}
        </div>
      </div>

      <div class="divider"></div>

      <!-- Parameters -->
      <h4 class="section-title">Parameters</h4>

      <div class="property-group">
        <label class="property-label">Plane</label>
        <select
          class="property-select"
          value={mockProperties.parameters.plane}
          onchange={(e) => handlePropertyChange('plane', e.currentTarget.value)}
        >
          <option value="XY">XY Plane</option>
          <option value="XZ">XZ Plane</option>
          <option value="YZ">YZ Plane</option>
        </select>
      </div>

      <div class="property-group">
        <label class="property-label">Offset</label>
        <input
          type="number"
          class="property-input"
          value={mockProperties.parameters.offset}
          step="0.1"
          onchange={(e) => handlePropertyChange('offset', parseFloat(e.currentTarget.value))}
        />
      </div>

      <div class="property-group">
        <label class="property-label">Locked</label>
        <input
          type="checkbox"
          class="property-checkbox"
          checked={mockProperties.parameters.locked}
          onchange={(e) => handlePropertyChange('locked', e.currentTarget.checked)}
        />
      </div>

      <div class="divider"></div>

      <!-- Actions -->
      <div class="actions">
        <button class="action-btn primary">Apply</button>
        <button class="action-btn">Reset</button>
      </div>
    </div>
  {:else}
    <!-- No selection state -->
    <div class="empty-state">
      <div class="empty-icon">📋</div>
      <p class="empty-text">No object selected</p>
      <p class="empty-hint">Select an item from the model tree to view its properties</p>
    </div>
  {/if}
</div>

<style>
  .properties-panel {
    display: flex;
    flex-direction: column;
    height: 100%;
    background: var(--color-surface);
    border-left: 1px solid var(--color-border);
    overflow: hidden;
  }

  .panel-header {
    padding: 12px 16px;
    border-bottom: 1px solid var(--color-border);
  }

  .panel-header h3 {
    margin: 0;
    font-size: 14px;
    font-weight: 600;
    color: var(--color-text);
  }

  .panel-content {
    flex: 1;
    overflow-y: auto;
    padding: 16px;
  }

  .property-group {
    margin-bottom: 16px;
  }

  .property-label {
    display: block;
    font-size: 12px;
    font-weight: 500;
    color: var(--color-text-secondary);
    margin-bottom: 6px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
  }

  .property-input,
  .property-select {
    width: 100%;
    padding: 8px 12px;
    background: var(--color-bg);
    border: 1px solid var(--color-border);
    border-radius: 4px;
    color: var(--color-text);
    font-size: 13px;
    transition: all 0.15s ease;
  }

  .property-input:focus,
  .property-select:focus {
    outline: none;
    border-color: var(--color-primary);
    box-shadow: 0 0 0 2px rgba(99, 102, 241, 0.1);
  }

  .property-value.readonly {
    padding: 8px 12px;
    background: var(--color-bg);
    border: 1px solid var(--color-border);
    border-radius: 4px;
    color: var(--color-text-secondary);
    font-size: 13px;
  }

  .property-checkbox {
    width: 18px;
    height: 18px;
    cursor: pointer;
  }

  .section-title {
    font-size: 13px;
    font-weight: 600;
    color: var(--color-text);
    margin: 0 0 12px 0;
  }

  .divider {
    height: 1px;
    background: var(--color-border);
    margin: 20px 0;
  }

  .actions {
    display: flex;
    gap: 8px;
    margin-top: 20px;
  }

  .action-btn {
    flex: 1;
    padding: 8px 16px;
    background: transparent;
    border: 1px solid var(--color-border);
    border-radius: 4px;
    color: var(--color-text);
    font-size: 13px;
    font-weight: 500;
    cursor: pointer;
    transition: all 0.15s ease;
  }

  .action-btn:hover {
    background: var(--color-hover);
    border-color: var(--color-primary);
  }

  .action-btn.primary {
    background: var(--color-primary);
    color: var(--color-bg);
    border-color: var(--color-primary);
  }

  .action-btn.primary:hover {
    background: var(--color-primary-dark);
  }

  /* Empty State */
  .empty-state {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    height: 100%;
    padding: 32px;
    text-align: center;
  }

  .empty-icon {
    font-size: 48px;
    margin-bottom: 16px;
    opacity: 0.5;
  }

  .empty-text {
    font-size: 14px;
    font-weight: 500;
    color: var(--color-text);
    margin: 0 0 8px 0;
  }

  .empty-hint {
    font-size: 12px;
    color: var(--color-text-secondary);
    margin: 0;
    max-width: 200px;
  }
</style>
