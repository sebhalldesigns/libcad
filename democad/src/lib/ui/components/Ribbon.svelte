<script lang="ts">
  import type { RibbonTab } from '../model';

  export let tabs: RibbonTab[] = [];
  export let activeTabId = '';
  export let onTabChange: (tabId: string) => void = () => {};
  export let onAction: (actionId: string) => void = () => {};

  $: activeTab = tabs.find((tab) => tab.id === activeTabId) ?? tabs[0];
</script>

<section class="ribbon-shell" aria-label="Main CAD ribbon">
  <div class="app-strip">
    <div class="badge">DEMO CAD</div>
    <div class="doc-title">Untitled Assembly</div>
    <div class="status">Draft Mode</div>
  </div>

  <div class="tab-row" role="tablist" aria-label="Ribbon tabs">
    {#each tabs as tab}
      <button
        class="tab-btn"
        class:active={tab.id === activeTab.id}
        role="tab"
        aria-selected={tab.id === activeTab.id}
        on:click={() => onTabChange(tab.id)}
      >
        {tab.label}
      </button>
    {/each}
  </div>

  <div class="group-row" role="region" aria-label={`${activeTab.label} tools`}>
    {#each activeTab.groups as group}
      <section class="group-card">
        <div class="actions">
          {#each group.actions as action}
            <button
              class="action-btn"
              class:emphasized={action.emphasized}
              title={action.hint}
              on:click={() => onAction(action.id)}
            >
              <span class="icon" aria-hidden="true">{action.icon}</span>
              <span class="label">{action.label}</span>
            </button>
          {/each}
        </div>
        <footer>{group.label}</footer>
      </section>
    {/each}
  </div>
</section>
