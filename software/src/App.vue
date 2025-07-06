<script setup lang="ts">
  //---------------------------------------------------
  //
  //  Imports
  //
  //---------------------------------------------------
  import { onBeforeUnmount, onMounted } from 'vue';
  import Bluetooth from '@/utils/Bluetooth.ts';

  //---------------------------------------------------
  //
  //  Properties
  //
  //---------------------------------------------------
  // const props = defineProps({});

  //---------------------------------------------------
  //
  //  Emits
  //
  //---------------------------------------------------
  // const emit = defineEmits([]);

  //---------------------------------------------------
  //
  //  Data Model
  //
  //---------------------------------------------------
  // const value = ref();

  //---------------------------------------------------
  //
  //  Computed Properties
  //
  //---------------------------------------------------
  // const computedProperty = computed(() => { return null; });

  //---------------------------------------------------
  //
  //  Watch Properties
  //
  //---------------------------------------------------
  // watch(value, (newval, oldval) => { console.log(newval, oldval); });

  //---------------------------------------------------
  //
  //  Vue Lifecycle
  //
  //---------------------------------------------------
  // onBeforeMount(() => {});
  onMounted(() => {
  });
  // onBeforeUpdate(() => {});
  // onUpdated(() => {});
  // onActivated(() => {});
  // onDeactivated(() => {});
  onBeforeUnmount(() => {
    Bluetooth.disconnect();
  });
  // onUnmounted(() => {});

  //---------------------------------------------------
  //
  //  Methods
  //
  //---------------------------------------------------
  async function connect() {
    const connected = await Bluetooth.connect();
    if (connected) {
      const settings = await Bluetooth.getSettings();
      console.log(settings);
    }
  }

  function disconnect() {
    Bluetooth.disconnect();
  }
  //---------------------------------------------------
  //
  //  Expose
  //
  //---------------------------------------------------
  // defineExpose({});
</script>

<template>
  <button @click="connect">Connect</button>
  <button @click="disconnect">Discconect</button>
  <RouterView />
</template>

<style lang="scss">
  @use '/src/assets/scss/app';

  button {
    font-family: inherit;
    color: var(--color-text-primary);
    border: 1px solid var(--color-borders);
    background: transparent;
  }
</style>
