<script setup lang="ts">
  //---------------------------------------------------
  //
  //  Imports
  //
  //---------------------------------------------------
  import { LeverPushSettings } from '@/types/interfaces.ts';
  import { computed, PropType } from 'vue';
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
    lever: {
      type: Number,
      default: 0,
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

  //---------------------------------------------------
  //
  //  Computed Properties
  //
  //---------------------------------------------------
  const isValidMidiCC = computed(() => {
    const cc = value.value.ccNumber || 0;
    return cc >= 0 && cc <= 127;
  });

  //---------------------------------------------------
  //
  //  Watch Properties
  //
  //---------------------------------------------------
  // watch(value, (newval, oldval) => { console.log(newval, oldval); });
  const value = defineModel({ type: Object as PropType<LeverPushSettings>, default: () => {} });

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
  <div :class="`settings-leverpush lever-${props.lever}`">
    <div class="title">
      <h2>{{ props.title }} {{ props.lever }}</h2>
      <div v-if="isValidMidiCC">
        <span>MIDI CC</span>
        <span>{{ value.ccNumber }}</span>
      </div>
    </div>

    <div class="inputs">
      <div class="group">
        <label :for="`push-ccNumber-${props.lever}`">Parameter</label>
        <select :id="`push-ccNumber-${props.lever}`" v-model="value.ccNumber">
          <option v-for="(param, pdx) in tm('configurator.values.cc')" :key="`ccnumber-${pdx}`" :value="param.value">
            {{ param.label }}
          </option>
        </select>
      </div>
      <div class="group">
        <label :for="`push-functionMode-${props.lever}`">Function Mode</label>
        <select :id="`push-functionMode-${props.lever}`" v-model="value.functionMode">
          <option
            v-for="(param, pdx) in tm('configurator.values.leverpush.functionModes')"
            :key="`functionmode-${pdx}`"
            :value="param.value"
          >
            {{ param.label }}
          </option>
        </select>
      </div>
      <div class="group">
        <label :for="`push-maxCCValue-${props.lever}`">CC Max</label>
        <input type="number" :id="`push-maxCCValue-${props.lever}`" v-model="value.maxCCValue" />
      </div>
      <div class="group">
        <label :for="`push-minCCValue-${props.lever}`">CC Min</label>
        <input type="number" :id="`push-minCCValue-${props.lever}`" v-model="value.minCCValue" />
      </div>
      <div class="group">
        <label :for="`push-onsetType-${props.lever}`">Attack Type</label>
        <select :id="`push-onsetType-${props.lever}`" v-model="value.onsetType">
          <option
            v-for="(param, pdx) in tm('configurator.values.interpolations')"
            :key="`onsettype-${pdx}`"
            :value="param.value"
          >
            {{ param.label }}
          </option>
        </select>
      </div>
      <div class="group">
        <label :for="`push-onsetTime-${props.lever}`">Attack Time</label>
        <div class="number-with-unit">
          <input type="number" :id="`push-onsetTime-${props.lever}`" v-model="value.onsetTime" />
          <span>ms</span>
        </div>
      </div>
      <div class="group">
        <label :for="`push-offsetType-${props.lever}`">Decay Type</label>
        <select :id="`push-offsetType-${props.lever}`" v-model="value.offsetType">
          <option
            v-for="(param, pdx) in tm('configurator.values.interpolations')"
            :key="`offsettype-${pdx}`"
            :value="param.value"
          >
            {{ param.label }}
          </option>
        </select>
      </div>
      <div class="group">
        <label :for="`push-offsetTime-${props.lever}`">Decay Time</label>
        <div class="number-with-unit">
          <input type="number" :id="`push-offsetTime-${props.lever}`" v-model="value.offsetTime" />
          <span>ms</span>
        </div>
      </div>
    </div>
  </div>
</template>

<style lang="scss">
  .settings-leverpush {
    & > .title {
      display: flex;
      justify-content: space-between;
      align-items: flex-end;
      padding: var(--baseunit) calc(var(--baseunit) * 1.5) var(--baseunit);

      h2 {
        text-transform: uppercase;
        font-weight: 400;
        letter-spacing: 2px;
        color: color-mix(in hsl, currentColor, white 30%);
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
          max-width: 65%;
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
          max-width: 65%;
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
