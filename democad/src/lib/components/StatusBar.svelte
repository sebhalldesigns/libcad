<!--
  Status Bar Component

  Bottom status bar with skeumorphic design showing app state and info.
  Features inset styling and visual depth.
-->
<script lang="ts">
  import { statusMessage, currentTool } from '../stores/appState';

  // Simulated coordinates (will come from WASM later)
  let mouseX = 0;
  let mouseY = 0;
  let mouseZ = 0;
</script>

<div class="status-bar">
  <!-- Status Message -->
  <div class="status-section">
    <span class="status-icon">ℹ️</span>
    <span class="status-text">{$statusMessage}</span>
  </div>

  <div class="status-divider"></div>

  <!-- Current Tool -->
  <div class="status-section">
    <span class="status-label">Tool:</span>
    <span class="status-value">{$currentTool}</span>
  </div>

  <div class="status-divider"></div>

  <!-- Coordinates -->
  <div class="status-section">
    <span class="status-label">X:</span>
    <span class="status-value mono">{mouseX.toFixed(2)}</span>

    <span class="status-label">Y:</span>
    <span class="status-value mono">{mouseY.toFixed(2)}</span>

    <span class="status-label">Z:</span>
    <span class="status-value mono">{mouseZ.toFixed(2)}</span>
  </div>

  <div class="status-spacer"></div>

  <!-- System Info -->
  <div class="status-section">
    <span class="status-label">DemoCAD</span>
    <span class="status-value">v0.1.0</span>
  </div>
</div>

<style>
  .status-bar {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 4px 12px;
    background: linear-gradient(180deg, #16161a 0%, #121216 100%);
    border-top: 1px solid var(--color-border);
    box-shadow:
      0 -2px 8px rgba(0, 0, 0, 0.6),
      inset 0 1px 0 rgba(255, 255, 255, 0.05);
    height: 28px;
    box-sizing: border-box;
    font-size: 11px;
  }

  .status-section {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 2px 8px;
    background: var(--color-inset);
    border: 1px solid var(--color-border);
    border-radius: var(--radius-sm);
    box-shadow: var(--shadow-inset);
  }

  .status-icon {
    font-size: 12px;
    filter: drop-shadow(0 1px 1px rgba(0, 0, 0, 0.5));
  }

  .status-text {
    color: var(--color-text);
    font-weight: 600;
    text-shadow: 0 1px 1px rgba(0, 0, 0, 0.6);
  }

  .status-label {
    color: var(--color-text-secondary);
    font-weight: 700;
    text-transform: uppercase;
    font-size: 9px;
    letter-spacing: 0.5px;
    text-shadow: 0 1px 1px rgba(0, 0, 0, 0.8);
  }

  .status-value {
    color: var(--color-text);
    font-weight: 700;
    text-shadow: 0 1px 1px rgba(0, 0, 0, 0.6);
  }

  .status-value.mono {
    font-family: var(--font-mono);
    min-width: 42px;
    text-align: right;
    font-size: 10px;
    color: var(--color-primary-light);
  }

  .status-divider {
    width: 1px;
    height: 16px;
    background: linear-gradient(180deg, transparent 0%, var(--color-border) 50%, transparent 100%);
    box-shadow: 1px 0 0 rgba(255, 255, 255, 0.05);
  }

  .status-spacer {
    flex: 1;
  }
</style>
