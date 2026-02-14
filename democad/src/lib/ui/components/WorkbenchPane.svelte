<script lang="ts">
  export let title = 'Panel';
  export let subtitle = '';
  export let compact = false;
</script>

<section class="panel" class:compact>
  <header>
    <h2>{title}</h2>
    {#if subtitle}
      <p>{subtitle}</p>
    {/if}
  </header>
  <div class="content">
    <slot />
  </div>
</section>
