<script setup lang="ts">
  //---------------------------------------------------
  //
  //  Imports
  //
  //---------------------------------------------------
  import Bluetooth from '@/utils/Bluetooth.ts';
  import { computed, onBeforeUnmount, onMounted, ref } from 'vue';
  import { Pages } from '@/types/interfaces.ts';
  import { useRoute, useRouter } from 'vue-router';

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
  const route = useRoute();
  const router = useRouter();
  const activePage = ref(Pages.CONFIGURATOR);

  //---------------------------------------------------
  //
  //  Computed Properties
  //
  //---------------------------------------------------
  const availablePages = computed(() => {
    return Object.keys(Pages).map((key) => {
      return {
        id: Pages[key],
        label: Pages[key],
      };
    });
  });

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
    if (route.path === '/') {
      router.push(`/${Pages.CONFIGURATOR}`);
    }
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
  function handlePage(page) {
    router.push(`/${page.id}`);
  }
  //---------------------------------------------------
  //
  //  Expose
  //
  //---------------------------------------------------
  // defineExpose({});
</script>

<template>
  <RouterView />
  <div class="pages">
    <button
      v-for="(page, pdx) in availablePages"
      :key="`page-${pdx}`"
      :class="{'active': activePage === page.id }"
      @click="handlePage(page)"
    >
      {{ page.label }}
    </button>
  </div>
</template>

<style lang="scss">
  @use '/src/assets/scss/app';

  body {
    display: flex;
    flex-direction: column;
    gap: 2px;
    width: 100%;
    max-height: 100vh;

    & > div[id] {
      width: 100%;
      height: calc(100vh - 66px);
    }

    & > .pages {
      display: flex;
      height: 64px;
      min-height: 64px;
      width: 100%;
      gap: 2px;
      background: transparent;

      & > button {
        width: 100%;
      }
    }
  }
</style>
