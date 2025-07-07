<script setup lang="ts">
  //---------------------------------------------------
  //
  //  Imports
  //
  //---------------------------------------------------
  import { TouchSettings } from '@/types/interfaces.ts';
  import { PropType } from 'vue';
  import { useI18n } from 'vue-i18n';

  //---------------------------------------------------
  //
  //  Properties
  //
  //---------------------------------------------------
  const props = defineProps({
    title: {
      type: String,
      default: null,
    },
  });

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
  const { tm } = useI18n();
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
  const value = defineModel({ type: Object as PropType<TouchSettings>, default: () => {} });

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

  //---------------------------------------------------
  //
  //  Expose
  //
  //---------------------------------------------------
  // defineExpose({});
</script>

<template>
  <div class="settings-touch">
    <div class="title">
      <h2>{{ props.title }}</h2>
      <div>
        <span>MIDI CC</span>
        <span>{{ value.ccNumber }}</span>
      </div>
    </div>

    <div class="inputs">
      <div class="group">
        <label for="touch-ccNumber">Parameter</label>
        <select id="touch-ccNumber" v-model="value.ccNumber">
          <option v-for="(param, pdx) in tm('configurator.values.cc')" :key="`ccnumber-${pdx}`" :value="param.value">
            {{ param.label }}
          </option>
        </select>
      </div>
      <div class="group">
        <label for="touch-functionMode">Function Mode</label>
        <select id="touch-functionMode" v-model="value.functionMode">
          <option
            v-for="(param, pdx) in tm('configurator.values.touch.functionModes')"
            :key="`functionmode-${pdx}`"
            :value="param.value"
          >
            {{ param.label }}
          </option>
        </select>
      </div>
      <div class="group">
        <label for="touch-maxCCValue">CC Max</label>
        <input type="number" id="touch-maxCCValue" v-model="value.maxCCValue" />
      </div>
      <div class="group">
        <label for="touch-minCCValue">CC Min</label>
        <input type="number" id="touch-minCCValue" v-model="value.minCCValue" />
      </div>
    </div>
  </div>
</template>

<style lang="scss">
  .settings-touch {
    color: var(--color-text-primary);
    & > .title {
      display: flex;
      justify-content: space-between;
      align-items: flex-end;
      padding: var(--baseunit) calc(var(--baseunit) * 1.5) var(--baseunit);

      h2 {
        text-transform: uppercase;
        font-weight: 400;
        letter-spacing: 2px;
        color: inherit;
      }

      & > div {
        opacity: 0.7;
        display: inline-flex;
        align-items: center;
        justify-content: center;
        gap: calc(var(--baseunit) * 2);
        & > span {
          font-weight: 600;
          &:last-of-type {
            color: var(--color-text-primary);
          }
        }
      }
    }
    & > .inputs {
      font-weight: 600;
      padding: 0 var(--baseunit);
      & > .group {
        display: flex;
        justify-content: space-between;
        padding: var(--baseunit) calc(var(--baseunit) * 0.5);
        border-top: 1px solid var(--color-borders);
        margin-bottom: 0;

        &:first-child {
          border-top: none;
        }
        &:last-of-type {
          border-bottom: none;
        }

        & > label {
          letter-spacing: 1px;
          text-transform: uppercase;
          font-size: 1.3rem;
          opacity: 0.7;
        }

        & > select {
          appearance: none;
          background: transparent;
          border: 0;
          text-align: right;
          font-family: inherit;
          font-weight: inherit;
          color: var(--color-text-primary);
          outline: 0;
          max-width: 50%;
          width: 100%;
          overflow: hidden;
          white-space: nowrap;
          text-overflow: ellipsis;

          & > option {
            background: var(--color-bg-primary);
          }
        }

        input[type='number'] {
          appearance: none;
          background: transparent;
          border: 0;
          text-align: right;
          font-family: inherit;
          font-weight: inherit;
          color: var(--color-text-primary);
          outline: 0;
          max-width: 50%;
          width: 100%;
          overflow: hidden;
          white-space: nowrap;
          text-overflow: ellipsis;
        }

        & > div.number-with-unit {
          position: relative;
          display: inline-flex;
          justify-content: flex-end;
          align-items: flex-end;

          & > input[type='number'] {
            max-width: 100%;
            padding-right: 20px;
          }
          & > span {
            position: absolute;
            pointer-events: none;
            opacity: 0.6;
          }
        }
      }
    }

    &.lever-1 {
      color: var(--color-lever1);
    }
    &.lever-2 {
      color: var(--color-lever2);
    }
  }
</style>

<i18n />
