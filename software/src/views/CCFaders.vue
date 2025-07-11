<script setup lang="ts">
  //---------------------------------------------------
  //
  //  Imports
  //
  //---------------------------------------------------
  import Fader from '@/components/Fader.vue';
  import Bluetooth from '@/utils/Bluetooth.ts';
  import { reactive } from 'vue';

  //---------------------------------------------------
  //
  //  Interfaces
  //
  //---------------------------------------------------
  interface CCObject {
    number: number;
    value: number;
  }
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

  const ccs = reactive([
    { number: 51, value: 0 },
    { number: 52, value: 0 },
    { number: 53, value: 0 },
    { number: 54, value: 0 },
    { number: 55, value: 0 },
    { number: 56, value: 0 },
    { number: 57, value: 0 },
    { number: 58, value: 0 },
    { number: 59, value: 0 },
    { number: 60, value: 0 },
    { number: 61, value: 0 },
    { number: 62, value: 0 },
  ] as CCObject[]);

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
  // onMounted(() => {});
  // onBeforeUpdate(() => {});
  // onUpdated(() => {});
  // onActivated(() => {});
  // onDeactivated(() => {});
  // onBeforeUnmount(() => {});
  // onUnmounted(() => {});

  //---------------------------------------------------
  //
  //  Methods
  //
  //---------------------------------------------------
  async function handleChange(ccNumber: number, value: number) {
    if (Bluetooth.connected) {
      Bluetooth.sendMidiCC(ccNumber, value);
    }
  }
  //---------------------------------------------------
  //
  //  Expose
  //
  //---------------------------------------------------
  // defineExpose({});
</script>

<template>
  <div id="faders">
    <Fader
      v-for="(cc, idx) in ccs"
      :key="`fader-${idx}`"
      :label="`CC-${String(cc.number).padStart(3, '0')}`"
      v-model="cc.value"
      @change="handleChange(cc.number, $event)"
    />
  </div>
</template>

<style lang="scss">
#faders {
  padding: calc(var(--baseunit) * 2) 0 ;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: calc(var(--baseunit) * 4);
  width: 100%;
}
</style>

<i18n />
