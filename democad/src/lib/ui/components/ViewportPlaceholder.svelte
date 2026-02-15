<script lang="ts">
  import { onDestroy, onMount } from 'svelte';
  import { addConsoleMessage } from '../model';

  export let title = 'Viewport';
  export let subtitle = '';
  export let mode: 'sketch' | 'scene' = 'scene';
  export let showHeader = true;

  // Export function to get document JSON
  export function getDocumentJSON(): any {
    if (!cadModule?._cad_get_document_json || !cadModule?.UTF8ToString) {
      return null;
    }
    const ptr = cadModule._cad_get_document_json();
    const jsonStr = cadModule.UTF8ToString(ptr);
    try {
      return JSON.parse(jsonStr);
    } catch (e) {
      console.error('[cad] Failed to parse document JSON:', e);
      return null;
    }
  }

  // Export function to select entity by ID
  export function selectEntity(entityId: number): void {
    if (!cadModule?._cad_set_selected_entity) return;
    const normalized = normalizePickedEntityId(entityId);
    cadModule._cad_set_selected_entity(normalized);
    lastDispatchedSelectedEntityId = normalized;
  }

  // Export function to set hovered entity by ID (used by explorer hover)
  export function setHoveredEntity(entityId: number): void {
    if (!cadModule?._cad_set_hovered_entity) return;
    const normalized = normalizePickedEntityId(entityId);
    cadModule._cad_set_hovered_entity(normalized);
    lastDispatchedHoverEntityId = normalized;
  }

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
    _cad_set_dpi_scale?: (scale: number) => void;
    _cad_render_viewport?: () => void;
    _cad_init_viewport?: () => void;
    _cad_set_cursor_pos?: (x: number, y: number) => void;
    _cad_set_cursor_button_state?: (button: number, pressed: boolean) => void;
    _cad_set_modifier_state?: (modifier: number, state: boolean) => void;
    _cad_axis_delta?: (axis: number, delta: number) => void;
    _cad_camera_orbit?: (deltaX: number, deltaY: number) => void;
    _cad_camera_pan?: (deltaX: number, deltaY: number) => void;
    _cad_camera_zoom?: (delta: number) => void;
    _cad_pick_entity?: (screenX: number, screenY: number) => number;
    _cad_set_hovered_entity?: (entityId: number) => void;
    _cad_get_hovered_entity?: () => number;
    _cad_set_selected_entity?: (entityId: number) => void;
    _cad_get_selected_entity?: () => number;
    _cad_get_document_json?: () => number;
    UTF8ToString?: (ptr: number) => string;
    calledRun?: boolean;
  };

  let canvasEl: HTMLCanvasElement | null = null;
  let containerEl: HTMLDivElement | null = null;
  let resizeObserver: ResizeObserver | null = null;
  let cadCtx: number | null = null;
  let cadModule: CadModule | null = null;
  let renderFrameId: number | null = null;

  // Cached bounding rect for performance
  let cachedRect: DOMRect | null = null;

  // Touch state
  let touchMode: 'none' | 'orbit' | 'pan' = 'none';
  let lastTouchX = 0;
  let lastTouchY = 0;
  let lastPinchDistance = 0;

  // RAF throttling for touch events
  let pendingTouchUpdate = false;
  let pendingTouchData: { x: number; y: number; zoom?: number } | null = null;

  // Click-to-select state
  let mouseDownX = 0;
  let mouseDownY = 0;
  let mouseDownTime = 0;
  const clickThreshold = 5; // pixels - max movement to still be considered a click
  const clickTimeThreshold = 300; // milliseconds

  const panThreshold = 3; // pixels center must move before panning
  const zoomSensitivity = 0.04; // zoom speed multiplier (lower = slower)
  const INVALID_ENTITY_ID = 0xFFFFFFFF;
  let lastDispatchedHoverEntityId = INVALID_ENTITY_ID;
  let lastDispatchedSelectedEntityId = INVALID_ENTITY_ID;

  const normalizePickedEntityId = (entityId: number): number => {
    const normalized = entityId >>> 0;
    if (normalized === 0 || normalized === INVALID_ENTITY_ID) {
      return INVALID_ENTITY_ID;
    }
    return normalized;
  };

  const dispatchHoveredEntity = (entityId: number): void => {
    if (entityId === lastDispatchedHoverEntityId) return;
    lastDispatchedHoverEntityId = entityId;
    window.dispatchEvent(new CustomEvent('cad-entity-hovered', { detail: { entityId } }));
  };

  const dispatchSelectedEntity = (entityId: number): void => {
    if (entityId === lastDispatchedSelectedEntityId) return;
    lastDispatchedSelectedEntityId = entityId;
    window.dispatchEvent(new CustomEvent('cad-entity-selected', { detail: { entityId } }));
  };

  const clearHoveredEntity = (): void => {
    cadModule?._cad_set_hovered_entity?.(INVALID_ENTITY_ID);
    dispatchHoveredEntity(INVALID_ENTITY_ID);
  };

  const dispatchDocumentUpdated = (): void => {
    const docJson = getDocumentJSON();
    if (!docJson) return;
    window.dispatchEvent(new CustomEvent('cad-document-updated', { detail: { docJson } }));
  };

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

  const updateCachedRect = (): void => {
    if (!canvasEl) return;
    cachedRect = canvasEl.getBoundingClientRect();
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
    updateCachedRect();
  };

  const getCanvasPoint = (clientX: number, clientY: number): { x: number; y: number } => {
    if (!cachedRect) return { x: 0, y: 0 };
    return {
      x: Math.floor(clientX - cachedRect.left),
      y: Math.floor(clientY - cachedRect.top)
    };
  };

  const getCanvasPixelPoint = (clientX: number, clientY: number): { x: number; y: number } => {
    if (!cachedRect || !canvasEl) return { x: 0, y: 0 };

    const localX = clientX - cachedRect.left;
    const localY = clientY - cachedRect.top;
    const scaleX = canvasEl.width / Math.max(1, cachedRect.width);
    const scaleY = canvasEl.height / Math.max(1, cachedRect.height);

    const x = Math.round(localX * scaleX);
    const y = Math.round(localY * scaleY);

    return {
      x: Math.max(0, Math.min(canvasEl.width - 1, x)),
      y: Math.max(0, Math.min(canvasEl.height - 1, y))
    };
  };

  const applyTouchUpdate = (): void => {
    if (!pendingTouchData || !cadModule) return;

    const data = pendingTouchData;
    pendingTouchData = null;
    pendingTouchUpdate = false;

    // Calculate deltas from last position
    const deltaX = data.x - lastTouchX;
    const deltaY = data.y - lastTouchY;

    // Apply camera movement based on mode
    if (touchMode === 'orbit' && (deltaX !== 0 || deltaY !== 0)) {
      cadModule._cad_camera_orbit?.(deltaX, deltaY);
    } else if (touchMode === 'pan') {
      // Apply pan only if movement exceeds threshold
      const movement = Math.hypot(deltaX, deltaY);
      if (movement >= panThreshold) {
        cadModule._cad_camera_pan?.(deltaX, deltaY);
      }

      // Apply zoom if present and exceeds threshold (can happen simultaneously)
      if (data.zoom !== undefined && data.zoom !== 0) {
        cadModule._cad_camera_zoom?.(data.zoom);
      }
    }

    // Update last position
    lastTouchX = data.x;
    lastTouchY = data.y;
  };

  const scheduleTouchUpdate = (x: number, y: number, zoom?: number): void => {
    pendingTouchData = { x, y, zoom };

    if (!pendingTouchUpdate) {
      pendingTouchUpdate = true;
      requestAnimationFrame(applyTouchUpdate);
    }
  };

  // Hover picking state
  let hoverScheduled = false;
  let pendingHoverX = 0;
  let pendingHoverY = 0;

  const applyHoverPick = (): void => {
    hoverScheduled = false;
    if (!cadModule?._cad_pick_entity || !cadModule?._cad_set_hovered_entity) return;

    const rawEntityId = cadModule._cad_pick_entity(pendingHoverX, pendingHoverY);
    const entityId = normalizePickedEntityId(rawEntityId);
    cadModule._cad_set_hovered_entity(entityId);
    dispatchHoveredEntity(entityId);
  };

  const scheduleHoverPick = (x: number, y: number): void => {
    pendingHoverX = x;
    pendingHoverY = y;
    if (!hoverScheduled) {
      hoverScheduled = true;
      requestAnimationFrame(applyHoverPick);
    }
  };

  // Camera interaction handlers
  const handleMouseMove = (event: MouseEvent): void => {
    if (!cadModule?._cad_set_cursor_pos || !canvasEl) return;
    const cssPoint = getCanvasPoint(event.clientX, event.clientY);
    const pixelPoint = getCanvasPixelPoint(event.clientX, event.clientY);
    cadModule._cad_set_cursor_pos(cssPoint.x, cssPoint.y);

    // Schedule hover picking (throttled via RAF)
    scheduleHoverPick(pixelPoint.x, pixelPoint.y);
  };

  const handleMouseDown = (event: MouseEvent): void => {
    if (!cadModule?._cad_set_cursor_button_state) return;

    // Track mouse down position and time for click detection
    if (event.button === 0) { // Left click only
      mouseDownX = event.clientX;
      mouseDownY = event.clientY;
      mouseDownTime = Date.now();
    }

    // Map button: 0=left, 1=middle, 2=right -> 1=left, 2=middle, 3=right (libcad convention)
    const buttonMap = [1, 2, 3];
    const button = buttonMap[event.button] || 1;
    cadModule._cad_set_cursor_button_state(button, true);
  };

  const handleMouseLeave = (): void => {
    clearHoveredEntity();
  };

  const handleMouseUp = (event: MouseEvent): void => {
    if (!cadModule?._cad_set_cursor_button_state) return;

    // Check if this was a click (not a drag)
    if (event.button === 0) { // Left click only
      const deltaX = Math.abs(event.clientX - mouseDownX);
      const deltaY = Math.abs(event.clientY - mouseDownY);
      const deltaTime = Date.now() - mouseDownTime;

      if (deltaX < clickThreshold && deltaY < clickThreshold && deltaTime < clickTimeThreshold) {
        // This was a click! Pick and select the entity
        if (cadModule._cad_pick_entity && cadModule._cad_set_selected_entity) {
          const pixelPoint = getCanvasPixelPoint(event.clientX, event.clientY);
          const rawEntityId = cadModule._cad_pick_entity(pixelPoint.x, pixelPoint.y);
          const entityId = normalizePickedEntityId(rawEntityId);
          cadModule._cad_set_selected_entity(entityId);
          dispatchSelectedEntity(entityId);
        }
      }
    }

    const buttonMap = [1, 2, 3];
    const button = buttonMap[event.button] || 1;
    cadModule._cad_set_cursor_button_state(button, false);
  };

  const handleWheel = (event: WheelEvent): void => {
    if (!cadModule?._cad_axis_delta) return;
    event.preventDefault();
    // Normalize wheel delta and pass to CAD (positive = zoom in, negative = zoom out)
    const delta = -Math.sign(event.deltaY);
    cadModule._cad_axis_delta(0, delta);
  };

  const handleKeyDown = (event: KeyboardEvent): void => {
    if (!cadModule?._cad_set_modifier_state) return;
    // Modifier constants: 1=Control, 2=Shift, 3=Alt
    if (event.key === 'Shift') {
      cadModule._cad_set_modifier_state(2, true);
    } else if (event.key === 'Control') {
      cadModule._cad_set_modifier_state(1, true);
    } else if (event.key === 'Alt') {
      cadModule._cad_set_modifier_state(3, true);
    }
  };

  const handleKeyUp = (event: KeyboardEvent): void => {
    if (!cadModule?._cad_set_modifier_state) return;
    if (event.key === 'Shift') {
      cadModule._cad_set_modifier_state(2, false);
    } else if (event.key === 'Control') {
      cadModule._cad_set_modifier_state(1, false);
    } else if (event.key === 'Alt') {
      cadModule._cad_set_modifier_state(3, false);
    }
  };

  const handleContextMenu = (event: MouseEvent): void => {
    // Prevent context menu on canvas to allow right-click for camera controls
    event.preventDefault();
  };

  const handleTouchStart = (event: TouchEvent): void => {
    if (!cadModule || !canvasEl) return;
    if (event.cancelable) event.preventDefault();
    clearHoveredEntity();

    const touchCount = event.touches.length;
    if (touchCount === 1) {
      // Single finger = orbit (or tap for selection)
      const p = getCanvasPoint(event.touches[0].clientX, event.touches[0].clientY);
      touchMode = 'orbit';
      lastTouchX = p.x;
      lastTouchY = p.y;
      lastPinchDistance = 0;

      // Track touch start for tap detection
      mouseDownX = event.touches[0].clientX;
      mouseDownY = event.touches[0].clientY;
      mouseDownTime = Date.now();
    } else if (touchCount >= 2) {
      // Two fingers = pan + zoom (with thresholds)
      const p0 = getCanvasPoint(event.touches[0].clientX, event.touches[0].clientY);
      const p1 = getCanvasPoint(event.touches[1].clientX, event.touches[1].clientY);

      const centerX = Math.floor((p0.x + p1.x) * 0.5);
      const centerY = Math.floor((p0.y + p1.y) * 0.5);

      touchMode = 'pan';
      lastTouchX = centerX;
      lastTouchY = centerY;

      const dx = p0.x - p1.x;
      const dy = p0.y - p1.y;
      lastPinchDistance = Math.hypot(dx, dy);
    }
  };

  const handleTouchMove = (event: TouchEvent): void => {
    if (!cadModule || !canvasEl) return;
    if (event.cancelable) event.preventDefault();

    const touchCount = event.touches.length;
    if (touchCount === 1) {
      // Single finger orbit
      const p = getCanvasPoint(event.touches[0].clientX, event.touches[0].clientY);

      if (touchMode !== 'orbit') {
        touchMode = 'orbit';
        lastTouchX = p.x;
        lastTouchY = p.y;
        lastPinchDistance = 0;
        return;
      }

      scheduleTouchUpdate(p.x, p.y);
    } else if (touchCount >= 2) {
      // Two finger pan + zoom with smooth continuous zoom
      const p0 = getCanvasPoint(event.touches[0].clientX, event.touches[0].clientY);
      const p1 = getCanvasPoint(event.touches[1].clientX, event.touches[1].clientY);

      const centerX = Math.floor((p0.x + p1.x) * 0.5);
      const centerY = Math.floor((p0.y + p1.y) * 0.5);

      const dx = p0.x - p1.x;
      const dy = p0.y - p1.y;
      const distance = Math.hypot(dx, dy);

      if (touchMode !== 'pan') {
        touchMode = 'pan';
        lastTouchX = centerX;
        lastTouchY = centerY;
        lastPinchDistance = distance;
        return;
      }

      // Calculate smooth continuous zoom
      let zoomDelta = 0;
      if (lastPinchDistance > 0) {
        const distanceChange = distance - lastPinchDistance;
        // Convert pixel change to smooth zoom delta
        zoomDelta = distanceChange * zoomSensitivity;
        lastPinchDistance = distance;
      } else {
        lastPinchDistance = distance;
      }

      // Schedule update with both pan (if exceeds threshold) and zoom
      scheduleTouchUpdate(centerX, centerY, zoomDelta);
    }
  };

  const handleTouchEnd = (event: TouchEvent): void => {
    if (!cadModule || !canvasEl) return;
    if (event.cancelable) event.preventDefault();

    const touchCount = event.touches.length;
    if (touchCount === 0) {
      // All fingers lifted - check if this was a tap
      if (touchMode === 'orbit' && event.changedTouches.length > 0) {
        const touch = event.changedTouches[0];
        const deltaX = Math.abs(touch.clientX - mouseDownX);
        const deltaY = Math.abs(touch.clientY - mouseDownY);
        const deltaTime = Date.now() - mouseDownTime;

        if (deltaX < clickThreshold && deltaY < clickThreshold && deltaTime < clickTimeThreshold) {
          // This was a tap! Pick and select the entity
          if (cadModule._cad_pick_entity && cadModule._cad_set_selected_entity) {
            const pixelPoint = getCanvasPixelPoint(touch.clientX, touch.clientY);
            const rawEntityId = cadModule._cad_pick_entity(pixelPoint.x, pixelPoint.y);
            const entityId = normalizePickedEntityId(rawEntityId);
            cadModule._cad_set_selected_entity(entityId);
            dispatchSelectedEntity(entityId);
          }
        }
      }

      touchMode = 'none';
      lastPinchDistance = 0;
      pendingTouchData = null;
      pendingTouchUpdate = false;
    } else if (touchCount === 1) {
      // Transition back to single finger orbit
      const p = getCanvasPoint(event.touches[0].clientX, event.touches[0].clientY);
      touchMode = 'orbit';
      lastTouchX = p.x;
      lastTouchY = p.y;
      lastPinchDistance = 0;
    } else {
      // Still have 2+ fingers, reset pan/zoom state
      const p0 = getCanvasPoint(event.touches[0].clientX, event.touches[0].clientY);
      const p1 = getCanvasPoint(event.touches[1].clientX, event.touches[1].clientY);

      const centerX = Math.floor((p0.x + p1.x) * 0.5);
      const centerY = Math.floor((p0.y + p1.y) * 0.5);

      touchMode = 'pan';
      lastTouchX = centerX;
      lastTouchY = centerY;

      const dx = p0.x - p1.x;
      const dy = p0.y - p1.y;
      lastPinchDistance = Math.hypot(dx, dy);
    }
  };

  const handleTouchCancel = (event: TouchEvent): void => {
    if (event.cancelable) event.preventDefault();
    touchMode = 'none';
    lastPinchDistance = 0;
    pendingTouchData = null;
    pendingTouchUpdate = false;
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

    // Update cached rect on scroll/resize
    window.addEventListener('scroll', updateCachedRect, { passive: true });
    window.addEventListener('resize', updateCachedRect, { passive: true });

    const win = window as Window & { Module?: CadModule };
    const loadCadModule = (): Promise<CadModule> => new Promise((resolve, reject) => {
      const moduleObject: CadModule = {
        canvas: canvasEl!,
        print: (text: string) => {
          console.log('[cad]', text);
          if (text.toLowerCase().includes('error')) {
            addConsoleMessage('warn', text);
          }
          else if (text.toLowerCase().includes('warning')) {
            addConsoleMessage('info', text);
          }
          else
          {
            addConsoleMessage('ok', text);
          }
        },
        printErr: (text: string) => {
          console.error('[cad]', text);
          addConsoleMessage('warn', text);
        }
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
      const cacheBust = `?v=${Date.now()}_${Math.random()}`;
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

        // Set DPI scale after context is created
        const dpr = Math.max(1, window.devicePixelRatio || 1);
        const scale = dpr; // Use DPI directly for consistent line thickness across devices

        addConsoleMessage('info', `DPI: ${dpr}, setting scale to ${scale}`);

        if (typeof cad._cad_set_dpi_scale === 'function') {
          cad._cad_set_dpi_scale(scale);
          addConsoleMessage('ok', `Set DPI scale to ${scale}`);
        } else {
          addConsoleMessage('warn', `_cad_set_dpi_scale not found (type: ${typeof cad._cad_set_dpi_scale})`);
        }

        updateCadViewport();
        startRenderLoop();
        dispatchDocumentUpdated();

        // Attach camera interaction event listeners
        if (canvasEl) {
          canvasEl.addEventListener('mousemove', handleMouseMove);
          canvasEl.addEventListener('mousedown', handleMouseDown);
          canvasEl.addEventListener('mouseup', handleMouseUp);
          canvasEl.addEventListener('mouseleave', handleMouseLeave);
          canvasEl.addEventListener('wheel', handleWheel, { passive: false });
          canvasEl.addEventListener('contextmenu', handleContextMenu);
          canvasEl.addEventListener('touchstart', handleTouchStart, { passive: false });
          canvasEl.addEventListener('touchmove', handleTouchMove, { passive: false });
          canvasEl.addEventListener('touchend', handleTouchEnd, { passive: false });
          canvasEl.addEventListener('touchcancel', handleTouchCancel, { passive: false });
        }

        // Attach keyboard listeners to window for modifier keys
        window.addEventListener('keydown', handleKeyDown);
        window.addEventListener('keyup', handleKeyUp);
      })
      .catch((error) => {
        console.error('[cad] WASM module load failed:', error);
      });
  });

  onDestroy(() => {
    // Remove camera interaction event listeners
    if (canvasEl) {
      canvasEl.removeEventListener('mousemove', handleMouseMove);
      canvasEl.removeEventListener('mousedown', handleMouseDown);
      canvasEl.removeEventListener('mouseup', handleMouseUp);
      canvasEl.removeEventListener('mouseleave', handleMouseLeave);
      canvasEl.removeEventListener('wheel', handleWheel);
      canvasEl.removeEventListener('contextmenu', handleContextMenu);
      canvasEl.removeEventListener('touchstart', handleTouchStart);
      canvasEl.removeEventListener('touchmove', handleTouchMove);
      canvasEl.removeEventListener('touchend', handleTouchEnd);
      canvasEl.removeEventListener('touchcancel', handleTouchCancel);
    }

    // Remove keyboard listeners
    window.removeEventListener('keydown', handleKeyDown);
    window.removeEventListener('keyup', handleKeyUp);

    // Remove cached rect update listeners
    window.removeEventListener('scroll', updateCachedRect);
    window.removeEventListener('resize', updateCachedRect);

    // Clear touch state
    touchMode = 'none';
    pendingTouchData = null;
    pendingTouchUpdate = false;

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
