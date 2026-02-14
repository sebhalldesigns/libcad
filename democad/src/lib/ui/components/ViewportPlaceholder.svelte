<script lang="ts">
  export let title = 'Viewport';
  export let subtitle = '';
  export let mode: 'sketch' | 'scene' = 'scene';

  // Hook point: replace this container with the canvas your WASM CAD module owns.
  // The outer frame is intentionally separate so styling survives renderer swaps.
</script>

<section class="viewport-shell {mode}">
  <header>
    <h3>{title}</h3>
    <small>{subtitle}</small>
  </header>

  <div class="viewport-canvas" role="img" aria-label={`${title} viewport placeholder`}>
    <div class="grid-overlay" aria-hidden="true"></div>
    <p>{mode === 'sketch' ? '2D sketch canvas placeholder' : '3D scene canvas placeholder'}</p>
  </div>
</section>
