<!--
  Viewport Component

  Main 3D viewport where the CAD model is rendered.
  Contains a canvas element that will be used by the WASM renderer.
  Handles mouse interactions for camera control and object selection.
-->
<script lang="ts">
  import { onMount } from 'svelte';
  import { currentTool, setStatus } from '../stores/appState';

  let canvas: HTMLCanvasElement;
  let isDragging = false;
  let lastMouseX = 0;
  let lastMouseY = 0;

  /**
   * Initialize the viewport when component mounts
   */
  onMount(() => {
    if (canvas) {
      // Get the 2D context for now (WASM will take over later)
      const ctx = canvas.getContext('2d');

      if (ctx) {
        // Draw a placeholder grid
        drawGrid(ctx);
      }

      // Handle canvas resize
      const resizeObserver = new ResizeObserver(() => {
        resizeCanvas();
        if (ctx) drawGrid(ctx);
      });

      resizeObserver.observe(canvas.parentElement!);

      return () => {
        resizeObserver.disconnect();
      };
    }
  });

  /**
   * Resize canvas to match container size
   */
  function resizeCanvas() {
    if (canvas && canvas.parentElement) {
      const rect = canvas.parentElement.getBoundingClientRect();
      canvas.width = rect.width;
      canvas.height = rect.height;
    }
  }

  /**
   * Draw a simple grid as placeholder
   * TODO: This will be replaced by WASM renderer
   */
  function drawGrid(ctx: CanvasRenderingContext2D) {
    const width = canvas.width;
    const height = canvas.height;

    // Clear canvas
    ctx.fillStyle = '#1a1a1a';
    ctx.fillRect(0, 0, width, height);

    // Draw grid
    ctx.strokeStyle = '#2a2a2a';
    ctx.lineWidth = 1;

    const gridSize = 50;

    // Vertical lines
    for (let x = 0; x <= width; x += gridSize) {
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, height);
      ctx.stroke();
    }

    // Horizontal lines
    for (let y = 0; y <= height; y += gridSize) {
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(width, y);
      ctx.stroke();
    }

    // Draw axes
    const centerX = width / 2;
    const centerY = height / 2;

    // X axis (red)
    ctx.strokeStyle = '#ff4444';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(centerX, centerY);
    ctx.lineTo(centerX + 100, centerY);
    ctx.stroke();

    // Y axis (green)
    ctx.strokeStyle = '#44ff44';
    ctx.beginPath();
    ctx.moveTo(centerX, centerY);
    ctx.lineTo(centerX, centerY - 100);
    ctx.stroke();

    // Labels
    ctx.fillStyle = '#ffffff';
    ctx.font = '12px monospace';
    ctx.fillText('X', centerX + 110, centerY + 5);
    ctx.fillText('Y', centerX + 5, centerY - 110);

    // Info text
    ctx.fillStyle = '#666666';
    ctx.font = '14px system-ui';
    ctx.fillText('WASM Renderer Will Initialize Here', 20, 30);
    ctx.fillText('Canvas ready for 3D scene', 20, 50);
  }

  /**
   * Handle mouse down - start dragging
   */
  function handleMouseDown(e: MouseEvent) {
    isDragging = true;
    lastMouseX = e.clientX;
    lastMouseY = e.clientY;

    setStatus(`Mouse down at (${e.offsetX}, ${e.offsetY})`);
  }

  /**
   * Handle mouse move - camera control
   */
  function handleMouseMove(e: MouseEvent) {
    if (!isDragging) return;

    const deltaX = e.clientX - lastMouseX;
    const deltaY = e.clientY - lastMouseY;

    lastMouseX = e.clientX;
    lastMouseY = e.clientY;

    // TODO: Pass to WASM renderer for camera control
    setStatus(`Camera delta: (${deltaX.toFixed(1)}, ${deltaY.toFixed(1)})`);
  }

  /**
   * Handle mouse up - stop dragging
   */
  function handleMouseUp(e: MouseEvent) {
    isDragging = false;
    setStatus('Ready');
  }

  /**
   * Handle mouse wheel - zoom control
   */
  function handleWheel(e: WheelEvent) {
    e.preventDefault();
    const delta = e.deltaY;

    // TODO: Pass to WASM renderer for zoom control
    setStatus(`Zoom: ${delta > 0 ? 'out' : 'in'}`);
  }

  /**
   * Handle context menu (right-click)
   */
  function handleContextMenu(e: MouseEvent) {
    e.preventDefault();
    setStatus('Right-click menu (TODO)');
  }
</script>

<div class="viewport">
  <canvas
    bind:this={canvas}
    onmousedown={handleMouseDown}
    onmousemove={handleMouseMove}
    onmouseup={handleMouseUp}
    onmouseleave={handleMouseUp}
    onwheel={handleWheel}
    oncontextmenu={handleContextMenu}
  ></canvas>

  <!-- Viewport HUD (Heads-Up Display) -->
  <div class="viewport-hud">
    <div class="hud-item">
      <span class="hud-label">Tool:</span>
      <span class="hud-value">{$currentTool}</span>
    </div>

    <div class="hud-item">
      <span class="hud-label">View:</span>
      <span class="hud-value">Perspective</span>
    </div>
  </div>

  <!-- View Cube (TODO: Make interactive) -->
  <div class="view-cube">
    <div class="cube-face">TOP</div>
  </div>
</div>

<style>
  .viewport {
    position: relative;
    width: 100%;
    height: 100%;
    background: #1a1a1a;
    overflow: hidden;
  }

  canvas {
    display: block;
    width: 100%;
    height: 100%;
    cursor: crosshair;
  }

  /* Heads-Up Display */
  .viewport-hud {
    position: absolute;
    top: 12px;
    left: 12px;
    display: flex;
    flex-direction: column;
    gap: 8px;
    pointer-events: none;
  }

  .hud-item {
    display: flex;
    gap: 8px;
    padding: 6px 12px;
    background: rgba(0, 0, 0, 0.7);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 4px;
    font-size: 12px;
    font-family: monospace;
  }

  .hud-label {
    color: #888;
  }

  .hud-value {
    color: #fff;
    font-weight: 600;
  }

  /* View Cube */
  .view-cube {
    position: absolute;
    top: 12px;
    right: 12px;
    width: 80px;
    height: 80px;
    background: rgba(0, 0, 0, 0.7);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 8px;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    transition: all 0.15s ease;
  }

  .view-cube:hover {
    background: rgba(0, 0, 0, 0.85);
    border-color: var(--color-primary);
    transform: scale(1.05);
  }

  .cube-face {
    color: #fff;
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 1px;
  }
</style>
