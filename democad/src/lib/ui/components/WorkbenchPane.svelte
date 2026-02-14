<script lang="ts">
  export let title = 'Panel';
  export let subtitle = '';
  export let compact = false;
</script>

<section class="panel" class:compact>
  <header>
    <div class="panel-heading">
      <h2>{title}</h2>
      {#if subtitle}
        <p>{subtitle}</p>
      {/if}
    </div>
    <div class="panel-actions">
      <slot name="header-actions" />
    </div>
  </header>
  <div class="content">
    <slot />
  </div>
</section>
