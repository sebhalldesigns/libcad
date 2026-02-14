<!--
  Sidebar Component

  Left sidebar displaying the model tree (hierarchy of sketches and features).
  Allows users to select, toggle visibility, and organize their CAD elements.
-->
<script lang="ts">
  import { modelTree, selectedItem, setStatus } from '../stores/appState';
  import type { TreeItem } from '../stores/appState';

  /**
   * Handle item selection in the tree
   */
  function selectItem(id: string) {
    selectedItem.set(id);
    setStatus(`Selected: ${id}`);
  }

  /**
   * Toggle visibility of a tree item
   */
  function toggleVisibility(item: TreeItem) {
    modelTree.update(items => {
      const updateItem = (items: TreeItem[]): TreeItem[] => {
        return items.map(i => {
          if (i.id === item.id) {
            return { ...i, visible: !i.visible };
          }
          if (i.children) {
            return { ...i, children: updateItem(i.children) };
          }
          return i;
        });
      };
      return updateItem(items);
    });
  }

  /**
   * Get icon for tree item type
   */
  function getIcon(type: TreeItem['type']): string {
    const icons = {
      sketch: '📐',
      extrude: '⬆️',
      revolve: '🔄',
      chamfer: '📏',
      fillet: '⌒'
    };
    return icons[type] || '📄';
  }
</script>

<div class="sidebar">
  <div class="sidebar-header">
    <h3>Model Tree</h3>
    <button
      class="icon-btn"
      title="Add new sketch"
      onclick={() => setStatus('Add new sketch')}
    >
      ➕
    </button>
  </div>

  <div class="tree">
    {#each $modelTree as item (item.id)}
      <div
        class="tree-item"
        class:selected={$selectedItem === item.id}
        onclick={() => selectItem(item.id)}
      >
        <button
          class="visibility-toggle"
          class:visible={item.visible}
          onclick={(e) => {
            e.stopPropagation();
            toggleVisibility(item);
          }}
          title={item.visible ? 'Hide' : 'Show'}
        >
          {item.visible ? '👁️' : '👁️‍🗨️'}
        </button>

        <span class="item-icon">{getIcon(item.type)}</span>
        <span class="item-name">{item.name}</span>
        <span class="item-type">{item.type}</span>
      </div>

      {#if item.children}
        <div class="tree-children">
          {#each item.children as child (child.id)}
            <div
              class="tree-item child"
              class:selected={$selectedItem === child.id}
              onclick={() => selectItem(child.id)}
            >
              <button
                class="visibility-toggle"
                class:visible={child.visible}
                onclick={(e) => {
                  e.stopPropagation();
                  toggleVisibility(child);
                }}
              >
                {child.visible ? '👁️' : '👁️‍🗨️'}
              </button>

              <span class="item-icon">{getIcon(child.type)}</span>
              <span class="item-name">{child.name}</span>
            </div>
          {/each}
        </div>
      {/if}
    {/each}
  </div>

  <!-- Quick Actions -->
  <div class="sidebar-footer">
    <button class="action-btn">
      <span>➕</span> New Sketch
    </button>
  </div>
</div>

<style>
  .sidebar {
    display: flex;
    flex-direction: column;
    height: 100%;
    background: var(--color-surface);
    border-right: 1px solid var(--color-border);
    overflow: hidden;
  }

  .sidebar-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 12px 16px;
    border-bottom: 1px solid var(--color-border);
  }

  .sidebar-header h3 {
    margin: 0;
    font-size: 14px;
    font-weight: 600;
    color: var(--color-text);
  }

  .icon-btn {
    background: transparent;
    border: 1px solid var(--color-border);
    border-radius: 4px;
    padding: 4px 8px;
    cursor: pointer;
    transition: all 0.15s ease;
    color: var(--color-text);
  }

  .icon-btn:hover {
    background: var(--color-hover);
    border-color: var(--color-primary);
  }

  .tree {
    flex: 1;
    overflow-y: auto;
    padding: 8px;
  }

  .tree-item {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px 12px;
    margin-bottom: 2px;
    border-radius: 4px;
    cursor: pointer;
    transition: all 0.15s ease;
    user-select: none;
  }

  .tree-item:hover {
    background: var(--color-hover);
  }

  .tree-item.selected {
    background: var(--color-primary);
    color: var(--color-bg);
  }

  .tree-item.child {
    margin-left: 24px;
  }

  .visibility-toggle {
    background: transparent;
    border: none;
    cursor: pointer;
    padding: 0;
    font-size: 14px;
    opacity: 0.5;
    transition: opacity 0.15s ease;
  }

  .visibility-toggle:hover {
    opacity: 1;
  }

  .visibility-toggle.visible {
    opacity: 1;
  }

  .item-icon {
    font-size: 16px;
  }

  .item-name {
    flex: 1;
    font-size: 13px;
    font-weight: 500;
  }

  .item-type {
    font-size: 11px;
    opacity: 0.6;
    text-transform: uppercase;
  }

  .tree-children {
    margin-left: 12px;
  }

  .sidebar-footer {
    padding: 12px;
    border-top: 1px solid var(--color-border);
  }

  .action-btn {
    width: 100%;
    padding: 8px 12px;
    background: var(--color-primary);
    color: var(--color-bg);
    border: none;
    border-radius: 4px;
    cursor: pointer;
    font-size: 13px;
    font-weight: 500;
    transition: all 0.15s ease;
  }

  .action-btn:hover {
    background: var(--color-primary-dark);
    transform: translateY(-1px);
  }

  .action-btn:active {
    transform: translateY(0);
  }
</style>
