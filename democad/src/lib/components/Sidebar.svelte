<!--
  Sidebar Component

  Left sidebar with skeumorphic design displaying the model tree.
  Features visual depth with gradients and shadows.
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
    border-right: 2px solid var(--color-border);
    box-shadow: 2px 0 8px rgba(0, 0, 0, 0.4);
    overflow: hidden;
  }

  .sidebar-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 8px 12px;
    background: linear-gradient(180deg, rgba(255, 255, 255, 0.05) 0%, transparent 100%);
    border-bottom: 1px solid var(--color-border);
    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.3);
  }

  .sidebar-header h3 {
    margin: 0;
    font-size: 13px;
    font-weight: 700;
    color: var(--color-text);
    text-shadow: 0 1px 2px rgba(0, 0, 0, 0.6);
  }

  .icon-btn {
    background: var(--gradient-button);
    border: 1px solid var(--color-border);
    border-radius: var(--radius-sm);
    padding: 3px 8px;
    cursor: pointer;
    transition: all var(--transition-fast);
    color: var(--color-text);
    font-size: 12px;
    box-shadow:
      0 1px 2px rgba(0, 0, 0, 0.4),
      inset 0 1px 0 rgba(255, 255, 255, 0.1);
  }

  .icon-btn:hover {
    background: var(--gradient-button-hover);
    border-color: var(--color-primary);
    box-shadow:
      0 2px 4px rgba(0, 0, 0, 0.5),
      0 0 8px var(--color-primary-glow);
    transform: translateY(-1px);
  }

  .icon-btn:active {
    transform: translateY(0);
    box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.5);
  }

  .tree {
    flex: 1;
    overflow-y: auto;
    padding: 6px;
    background: linear-gradient(180deg, rgba(0, 0, 0, 0.1) 0%, transparent 50px);
  }

  .tree-item {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 6px 10px;
    margin-bottom: 2px;
    border-radius: var(--radius-sm);
    cursor: pointer;
    transition: all var(--transition-fast);
    user-select: none;
    background: var(--gradient-button);
    border: 1px solid transparent;
    box-shadow: 0 1px 2px rgba(0, 0, 0, 0.2);
  }

  .tree-item:hover {
    background: var(--gradient-button-hover);
    border-color: var(--color-border-light);
    box-shadow:
      0 2px 4px rgba(0, 0, 0, 0.3),
      inset 0 1px 0 rgba(255, 255, 255, 0.1);
    transform: translateX(2px);
  }

  .tree-item.selected {
    background: var(--gradient-primary);
    color: white;
    border-color: var(--color-primary-dark);
    box-shadow:
      0 2px 6px var(--color-primary-glow),
      inset 0 1px 0 rgba(255, 255, 255, 0.3);
  }

  .tree-item.child {
    margin-left: 20px;
    font-size: 12px;
  }

  .visibility-toggle {
    background: transparent;
    border: none;
    cursor: pointer;
    padding: 2px;
    font-size: 13px;
    opacity: 0.4;
    transition: all var(--transition-fast);
    filter: grayscale(1);
  }

  .visibility-toggle:hover {
    opacity: 1;
    filter: grayscale(0);
    transform: scale(1.1);
  }

  .visibility-toggle.visible {
    opacity: 1;
    filter: grayscale(0);
  }

  .item-icon {
    font-size: 15px;
    filter: drop-shadow(0 1px 1px rgba(0, 0, 0, 0.5));
  }

  .item-name {
    flex: 1;
    font-size: 12px;
    font-weight: 600;
    text-shadow: 0 1px 1px rgba(0, 0, 0, 0.5);
  }

  .item-type {
    font-size: 9px;
    opacity: 0.7;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    font-weight: 700;
  }

  .tree-children {
    margin-left: 8px;
  }

  .sidebar-footer {
    padding: 8px;
    border-top: 1px solid var(--color-border);
    background: linear-gradient(180deg, transparent 0%, rgba(0, 0, 0, 0.2) 100%);
    box-shadow: 0 -2px 6px rgba(0, 0, 0, 0.3);
  }

  .action-btn {
    width: 100%;
    padding: 8px 12px;
    background: var(--gradient-primary);
    color: white;
    border: 1px solid var(--color-primary-dark);
    border-radius: var(--radius-sm);
    cursor: pointer;
    font-size: 12px;
    font-weight: 700;
    transition: all var(--transition-fast);
    box-shadow:
      0 2px 6px var(--color-primary-glow),
      inset 0 1px 0 rgba(255, 255, 255, 0.3);
    text-shadow: 0 1px 2px rgba(0, 0, 0, 0.5);
  }

  .action-btn:hover {
    background: linear-gradient(135deg, #7a9bff 0%, #6b8fff 50%, #5b7fff 100%);
    box-shadow:
      0 3px 8px var(--color-primary-glow),
      inset 0 1px 0 rgba(255, 255, 255, 0.4);
    transform: translateY(-2px);
  }

  .action-btn:active {
    transform: translateY(0);
    box-shadow:
      inset 0 2px 4px rgba(0, 0, 0, 0.3),
      0 1px 3px var(--color-primary-glow);
  }
</style>
