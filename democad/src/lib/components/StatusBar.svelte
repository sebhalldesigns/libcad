<!--
  Status Bar Component

  Bottom status bar displaying application state, messages, and quick info.
  Shows current tool, coordinates, and system messages.
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
    <span class="status-label">Version:</span>
    <span class="status-value">0.1.0</span>
  </div>
</div>

<style>
  .status-bar {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 6px 16px;
    background: var(--color-surface);
    border-top: 1px solid var(--color-border);
    height: 32px;
    box-sizing: border-box;
    font-size: 12px;
  }

  .status-section {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .status-icon {
    font-size: 14px;
  }

  .status-text {
    color: var(--color-text);
    font-weight: 500;
  }

  .status-label {
    color: var(--color-text-secondary);
    font-weight: 500;
    text-transform: uppercase;
    font-size: 11px;
    letter-spacing: 0.3px;
  }

  .status-value {
    color: var(--color-text);
    font-weight: 600;
  }

  .status-value.mono {
    font-family: 'Consolas', 'Monaco', monospace;
    min-width: 50px;
    text-align: right;
  }

  .status-divider {
    width: 1px;
    height: 20px;
    background: var(--color-border);
  }

  .status-spacer {
    flex: 1;
  }
</style>
