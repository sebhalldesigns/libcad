<script lang="ts">
  import Ribbon from './lib/ui/components/Ribbon.svelte';
  import WorkbenchPane from './lib/ui/components/WorkbenchPane.svelte';
  import ViewportPlaceholder from './lib/ui/components/ViewportPlaceholder.svelte';
  import {
    consoleMessages,
    inspectorFields,
    projectTree,
    ribbonTabs,
    type ProjectNode
  } from './lib/ui/model';

  let activeTabId = ribbonTabs[0].id;
  let selectedNodeId = projectTree.find((node) => node.active)?.id ?? projectTree[0].id;
  const appLinks = [
    { label: 'SVG Icon', href: '/native/icons/icon.svg' },
    { label: 'ICO Icon', href: '/native/icons/icon.ico' }
  ];

  const nodeIcon: Record<ProjectNode['type'], string> = {
    folder: '+',
    sketch: '/',
    solid: '#',
    operation: '*'
  };

  function handleRibbonAction(actionId: string): void {
    // Initial shell behavior: logging keeps the interaction flow testable
    // before command routing + WASM-backed operations are integrated.
    console.info(`[ribbon] action selected: ${actionId}`);
  }
</script>

<div class="cad-app">
  <Ribbon
    tabs={ribbonTabs}
    {activeTabId}
    appIconHref="/native/icons/icon.svg"
    {appLinks}
    onTabChange={(tabId) => (activeTabId = tabId)}
    onAction={handleRibbonAction}
  />

  <main class="workbench">
    <section class="workspace-stack">
      <div class="panel-center">
        <ViewportPlaceholder
          title="Workspace View"
          subtitle="Sketch + Scene Context"
          mode="scene"
          showHeader={false}
        />
      </div>

      <aside class="panel-left panel-floating">
        <WorkbenchPane title="Browser" subtitle="Features and project tree">
          <ul class="tree">
            {#each projectTree as node}
              <li>
                <button
                  class="tree-node"
                  class:selected={node.id === selectedNodeId}
                  style={`--depth:${node.depth}`}
                  on:click={() => (selectedNodeId = node.id)}
                >
                  <span class="glyph" aria-hidden="true">{nodeIcon[node.type]}</span>
                  <span>{node.name}</span>
                </button>
              </li>
            {/each}
          </ul>
        </WorkbenchPane>
      </aside>

      <aside class="panel-right panel-floating">
        <WorkbenchPane title="Inspector" subtitle="Selection and tool parameters">
          <ul class="inspector-list">
            {#each inspectorFields as field}
              <li>
                <span>{field.label}</span>
                <button class="value" class:editable={field.editable}>{field.value}</button>
              </li>
            {/each}
          </ul>
        </WorkbenchPane>
      </aside>

      <section class="panel-bottom panel-floating">
        <WorkbenchPane title="Console" subtitle="Kernel, solver and action log" compact={true}>
          <ul class="console-log">
            {#each consoleMessages as entry}
              <li class={`level-${entry.level}`}>
                <span class="time">[{entry.time}]</span>
                <span>{entry.text}</span>
              </li>
            {/each}
          </ul>
        </WorkbenchPane>
      </section>
    </section>
  </main>
</div>
