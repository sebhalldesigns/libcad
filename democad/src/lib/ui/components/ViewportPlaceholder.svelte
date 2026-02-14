<script lang="ts">
  import { onDestroy, onMount } from 'svelte';

  export let title = 'Viewport';
  export let subtitle = '';
  export let mode: 'sketch' | 'scene' = 'scene';
  export let showHeader = true;

  type CadModule = {
    onRuntimeInitialized?: () => void;
    locateFile?: (path: string) => string;
    canvas?: HTMLCanvasElement | null;
    print?: (text: string) => void;
    printErr?: (text: string) => void;
    _cad_create_context?: () => number;
    _cad_destroy_context?: (ctx: number) => void;
    _cad_set_viewport?: (
      x: number,
      y: number,
      width: number,
      height: number,
      windowWidth: number,
      windowHeight: number
    ) => void;
    _cad_render_viewport?: () => void;
    _cad_init_viewport?: () => void;
    calledRun?: boolean;
  };

  let canvasEl: HTMLCanvasElement | null = null;
  let containerEl: HTMLDivElement | null = null;
  let resizeObserver: ResizeObserver | null = null;
  let cadCtx: number | null = null;
  let cadModule: CadModule | null = null;
  let renderFrameId: number | null = null;

  const updateCadViewport = (): void => {
    if (!canvasEl || !cadModule?._cad_set_viewport) return;
    console.log('[cad] Setting viewport:', canvasEl.width, 'x', canvasEl.height);
    cadModule._cad_set_viewport(
      0,
      0,
      Math.max(1, canvasEl.width),
      Math.max(1, canvasEl.height),
      Math.max(1, Math.floor(window.innerWidth)),
      Math.max(1, Math.floor(window.innerHeight))
    );
  };

  const startRenderLoop = (): void => {
    if (!cadModule?._cad_render_viewport || renderFrameId !== null) return;
    const frame = (): void => {
      cadModule?._cad_render_viewport?.();
      renderFrameId = window.requestAnimationFrame(frame);
    };
    renderFrameId = window.requestAnimationFrame(frame);
  };

  const resizeCanvas = (): void => {
    if (!canvasEl || !containerEl) return;
    const dpr = Math.max(1, window.devicePixelRatio || 1);
    const cssWidth = Math.max(1, Math.floor(containerEl.clientWidth));
    const cssHeight = Math.max(1, Math.floor(containerEl.clientHeight));
    const pixelWidth = Math.floor(cssWidth * dpr);
    const pixelHeight = Math.floor(cssHeight * dpr);

    // Update canvas pixel dimensions
    if (canvasEl.width !== pixelWidth || canvasEl.height !== pixelHeight) {
      canvasEl.width = pixelWidth;
      canvasEl.height = pixelHeight;
    }

    // Also update canvas CSS size to match container
    canvasEl.style.width = cssWidth + 'px';
    canvasEl.style.height = cssHeight + 'px';

    updateCadViewport();
  };

  onMount(() => {
    if (!canvasEl) {
      console.error('[cad] Canvas element not available');
      return;
    }

    // Don't create WebGL context here - let Emscripten handle it
    resizeCanvas();

    resizeObserver = new ResizeObserver(() => {
      console.log('[cad] Resize detected');
      resizeCanvas();
    });
    if (containerEl) {
      resizeObserver.observe(containerEl);
    }

    const win = window as Window & { Module?: CadModule };
    const loadCadModule = (): Promise<CadModule> => new Promise((resolve, reject) => {
      const moduleObject: CadModule = {
        canvas: canvasEl!,
        print: (text: string) => console.log('[cad]', text),
        printErr: (text: string) => console.error('[cad]', text)
      };
      moduleObject.onRuntimeInitialized = () => {
        console.log('[cad] Runtime initialized');
        resolve(moduleObject);
      };
      win.Module = moduleObject;

      document.getElementById('cad-wasm-script')?.remove();

      const scriptCandidates = import.meta.env.DEV
        ? ['/dist/cad.js', '/cad.js']
        : ['/cad.js', '/dist/cad.js'];
      const cacheBust = `?t=${Date.now()}`;
      const tryLoad = (index: number): void => {
        if (index >= scriptCandidates.length) {
          reject(new Error('Failed to load cad.js from /cad.js or /dist/cad.js'));
          return;
        }

        const scriptSrc = `${scriptCandidates[index]}${cacheBust}`;
        fetch(scriptSrc, { method: 'GET', cache: 'no-store', credentials: 'same-origin' })
          .then((response) => {
            if (!response.ok) {
              tryLoad(index + 1);
              return;
            }

            const baseUrl = new URL(scriptSrc, window.location.origin);
            const basePath = baseUrl.pathname.replace(/\/[^/]*$/, '/');
            // Must be set before cad.js executes so wasm path + cache-busting
            // are consistent for this specific JS candidate.
            moduleObject.locateFile = (path: string) => `${basePath}${path}${cacheBust}`;

            const script = document.createElement('script');
            script.id = 'cad-wasm-script';
            script.src = scriptSrc;
            script.async = true;

            script.onload = () => {
              if (win.Module?._cad_create_context && win.Module.calledRun) {
                resolve(win.Module);
              }
            };

            script.onerror = () => {
              script.remove();
              tryLoad(index + 1);
            };

            document.head.appendChild(script);
          })
          .catch(() => {
            tryLoad(index + 1);
          });
      };

      tryLoad(0);
    });

    loadCadModule()
      .then((cad) => {
        cadModule = cad;
        if (!cad._cad_create_context) return;
        cadCtx = cad._cad_create_context();
        cad._cad_init_viewport?.();
        updateCadViewport();
        startRenderLoop();
      })
      .catch((error) => {
        console.error('[cad] WASM module load failed:', error);
      });
  });

  onDestroy(() => {
    resizeObserver?.disconnect();
    if (renderFrameId !== null) {
      window.cancelAnimationFrame(renderFrameId);
      renderFrameId = null;
    }
    const win = window as Window & { Module?: CadModule };
    if (cadCtx !== null && win.Module?._cad_destroy_context) {
      win.Module._cad_destroy_context(cadCtx);
      cadCtx = null;
    }
    cadModule = null;
  });
</script>

<section class="viewport-shell {mode}" class:no-header={!showHeader}>
  {#if showHeader}
    <header>
      <h3>{title}</h3>
      <small>{subtitle}</small>
    </header>
  {/if}

  <div class="viewport-canvas" bind:this={containerEl}>
    <canvas id="cad-canvas" bind:this={canvasEl} class="gl-canvas" aria-label={`${title} WebGL canvas`}></canvas>
  </div>
</section>
