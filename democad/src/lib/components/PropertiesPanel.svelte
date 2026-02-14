<!--
  Properties Panel Component

  Right-side panel with skeumorphic design for object properties.
  Features inset inputs and visual depth.
-->
<script lang="ts">
  import { selectedItem, objectProperties, hasSelection } from '../stores/appState';

  // Mock properties for demonstration
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
        <label class="checkbox-wrapper">
          <input
            type="checkbox"
            class="property-checkbox"
            checked={mockProperties.parameters.locked}
            onchange={(e) => handlePropertyChange('locked', e.currentTarget.checked)}
          />
          <span class="checkbox-custom"></span>
        </label>
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
      <p class="empty-hint">Select an item from the model tree</p>
    </div>
  {/if}
</div>

<style>
  .properties-panel {
    display: flex;
    flex-direction: column;
    height: 100%;
    background: var(--color-surface);
    border-left: 2px solid var(--color-border);
    box-shadow: -2px 0 8px rgba(0, 0, 0, 0.4);
    overflow: hidden;
  }

  .panel-header {
    padding: 8px 12px;
    background: linear-gradient(180deg, rgba(255, 255, 255, 0.05) 0%, transparent 100%);
    border-bottom: 1px solid var(--color-border);
    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.3);
  }

  .panel-header h3 {
    margin: 0;
    font-size: 13px;
    font-weight: 700;
    color: var(--color-text);
    text-shadow: 0 1px 2px rgba(0, 0, 0, 0.6);
  }

  .panel-content {
    flex: 1;
    overflow-y: auto;
    padding: 12px;
    background: linear-gradient(180deg, rgba(0, 0, 0, 0.1) 0%, transparent 50px);
  }

  .property-group {
    margin-bottom: 12px;
  }

  .property-label {
    display: block;
    font-size: 10px;
    font-weight: 700;
    color: var(--color-text-secondary);
    margin-bottom: 4px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    text-shadow: 0 1px 1px rgba(0, 0, 0, 0.5);
  }

  .property-input,
  .property-select {
    width: 100%;
    padding: 6px 10px;
    background: var(--color-inset);
    border: 1px solid var(--color-border);
    border-radius: var(--radius-sm);
    color: var(--color-text);
    font-size: 12px;
    transition: all var(--transition-fast);
    box-shadow: var(--shadow-inset);
    font-weight: 500;
  }

  .property-input:focus,
  .property-select:focus {
    outline: none;
    border-color: var(--color-primary);
    box-shadow:
      var(--shadow-inset),
      0 0 8px var(--color-primary-glow);
  }

  .property-input:hover,
  .property-select:hover {
    border-color: var(--color-border-light);
  }

  .property-value.readonly {
    padding: 6px 10px;
    background: linear-gradient(135deg, rgba(255, 255, 255, 0.03) 0%, rgba(0, 0, 0, 0.1) 100%);
    border: 1px solid var(--color-border);
    border-radius: var(--radius-sm);
    color: var(--color-text-secondary);
    font-size: 12px;
    font-weight: 600;
    box-shadow: var(--shadow-inset);
  }

  /* Custom Checkbox */
  .checkbox-wrapper {
    display: inline-flex;
    align-items: center;
    cursor: pointer;
    position: relative;
  }

  .property-checkbox {
    position: absolute;
    opacity: 0;
    cursor: pointer;
  }

  .checkbox-custom {
    width: 18px;
    height: 18px;
    background: var(--color-inset);
    border: 1px solid var(--color-border);
    border-radius: var(--radius-sm);
    display: inline-block;
    position: relative;
    transition: all var(--transition-fast);
    box-shadow: var(--shadow-inset);
  }

  .property-checkbox:checked + .checkbox-custom {
    background: var(--gradient-primary);
    border-color: var(--color-primary-dark);
    box-shadow: 0 2px 6px var(--color-primary-glow);
  }

  .property-checkbox:checked + .checkbox-custom::after {
    content: '✓';
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    color: white;
    font-size: 12px;
    font-weight: bold;
    text-shadow: 0 1px 2px rgba(0, 0, 0, 0.5);
  }

  .checkbox-wrapper:hover .checkbox-custom {
    border-color: var(--color-primary);
  }

  .section-title {
    font-size: 11px;
    font-weight: 700;
    color: var(--color-text);
    margin: 0 0 10px 0;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    text-shadow: 0 1px 2px rgba(0, 0, 0, 0.6);
  }

  .divider {
    height: 1px;
    background: linear-gradient(90deg, transparent 0%, var(--color-border) 50%, transparent 100%);
    margin: 14px 0;
    box-shadow: 0 1px 0 rgba(255, 255, 255, 0.05);
  }

  .actions {
    display: flex;
    gap: 6px;
    margin-top: 16px;
  }

  .action-btn {
    flex: 1;
    padding: 7px 14px;
    background: var(--gradient-button);
    border: 1px solid var(--color-border);
    border-radius: var(--radius-sm);
    color: var(--color-text);
    font-size: 11px;
    font-weight: 700;
    cursor: pointer;
    transition: all var(--transition-fast);
    text-transform: uppercase;
    letter-spacing: 0.3px;
    box-shadow:
      0 1px 3px rgba(0, 0, 0, 0.4),
      inset 0 1px 0 rgba(255, 255, 255, 0.1);
    text-shadow: 0 1px 1px rgba(0, 0, 0, 0.5);
  }

  .action-btn:hover {
    background: var(--gradient-button-hover);
    border-color: var(--color-border-light);
    box-shadow:
      0 2px 4px rgba(0, 0, 0, 0.5),
      inset 0 1px 0 rgba(255, 255, 255, 0.15);
    transform: translateY(-1px);
  }

  .action-btn:active {
    transform: translateY(0);
    box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.5);
  }

  .action-btn.primary {
    background: var(--gradient-primary);
    color: white;
    border-color: var(--color-primary-dark);
    box-shadow:
      0 2px 6px var(--color-primary-glow),
      inset 0 1px 0 rgba(255, 255, 255, 0.3);
  }

  .action-btn.primary:hover {
    background: linear-gradient(135deg, #7a9bff 0%, #6b8fff 50%, #5b7fff 100%);
    box-shadow:
      0 3px 8px var(--color-primary-glow),
      inset 0 1px 0 rgba(255, 255, 255, 0.4);
  }

  /* Empty State */
  .empty-state {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    height: 100%;
    padding: 24px;
    text-align: center;
  }

  .empty-icon {
    font-size: 48px;
    margin-bottom: 12px;
    opacity: 0.3;
    filter: grayscale(1);
  }

  .empty-text {
    font-size: 13px;
    font-weight: 700;
    color: var(--color-text);
    margin: 0 0 6px 0;
    text-shadow: 0 1px 2px rgba(0, 0, 0, 0.5);
  }

  .empty-hint {
    font-size: 11px;
    color: var(--color-text-secondary);
    margin: 0;
    max-width: 180px;
    line-height: 1.4;
  }
</style>
