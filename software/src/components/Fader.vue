<script setup lang="ts">
  //---------------------------------------------------
  //
  //  Imports
  //
  //---------------------------------------------------
  import { computed } from 'vue';

  //---------------------------------------------------
  //
  //  Properties
  //
  //---------------------------------------------------
  const props = defineProps({
    label: {
      type: String,
      default: null,
    },
  });

  //---------------------------------------------------
  //
  //  Emits
  //
  //---------------------------------------------------
  const emit = defineEmits(['change']);

  //---------------------------------------------------
  //
  //  Data Model
  //
  //---------------------------------------------------
  const value = defineModel<string|number>({ type: [Number, String], default: 0 });

  //---------------------------------------------------
  //
  //  Computed Properties
  //
  //---------------------------------------------------
  const percentage = computed(() => {
    const val = parseInt(String(value.value || 0), 10);
    const percent = (val / 127) * 100;
    return `${percent}%`;
  });

  //---------------------------------------------------
  //
  //  Watch Properties
  //
  //---------------------------------------------------

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
  function handleInput(evt: Event) {
    const input = evt.target as HTMLInputElement;
    if (input) {
      const newValue = parseInt(input.value, 10);
      emit('change', newValue);
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
  <div class="fader">
    <input
      type="range"
      min="0"
      max="127"
      step="1"
      :style="`--value:${percentage}`"
      v-model="value"
      @input="handleInput"
    >
    <label v-if="props.label">{{ props.label }}</label>
  </div>
</template>

<style lang="scss">
  div.fader {
    position: relative;
    --size: calc(var(--baseunit) * 2);
    width: 100%;
    min-height: var(--size);
    max-height: var(--size);
    height: var(--size);
    max-width: 80vw;

    &:before {
      position: absolute;
      content: '';
      width: 100%;
      height: 100%;
      background: linear-gradient(90deg, var(--color-lever2), var(--color-lever1));
      border-radius: var(--size);
      z-index: 1;
    }

    &:after {
      position: absolute;
      inset: 0;
      content: '';
      width: calc(100% - 4px);
      height: calc(100% - 4px);
      transform: translate(2px, 2px);

      background: black;
      border-radius: var(--size);
      z-index: 2;
    }

    & > label {
      position: absolute;
      top: -12px;
      right: 12px;
      font-weight: 700;
      font-size: 0.9rem;
      opacity: 0.45;
      color: var(--color-lever1);
      user-select: none;
      pointer-events: none;
      z-index: 4;
      mix-blend-mode: screen;
    }

    input[type=range] {
      position: absolute;
      left: -1px;
      top: -1px;
      z-index: 3;
      appearance: none;
      width: calc(100% - 2px);
      height: calc(var(--size) - 2px);
      border-radius: var(--size);
      background-color: transparent;

      &::-webkit-slider-thumb {
        appearance: none;
        height: var(--size);
        width: 1px;
        background: transparent;
      }

      &::-webkit-slider-runnable-track {
        height: calc(100% - 2px);
        //noinspection CssInvalidFunction
        background: linear-gradient(90deg, var(--color-lever2) calc(var(--value) * 0.25), var(--color-lever1) var(--value), transparent calc(var(--value) + 1px));
        border-radius: var(--size);
      }

      &:focus {
        outline: none;
      }
    }
  }
</style>

<i18n />
