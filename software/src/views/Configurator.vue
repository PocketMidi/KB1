<script setup lang="ts">
  //---------------------------------------------------
  //
  //  Imports
  //
  //---------------------------------------------------
  import LeverSettings from '@/components/LeverSettings.vue';
  import LeverPushSettings from '@/components/LeverPushSettings.vue';
  import TouchSettings from '@/components/TouchSettings.vue';
  import ScaleSettings from '@/components/ScaleSettings.vue';
  import Bluetooth from '@/utils/Bluetooth.ts';
  import { ConfiguratorViewState, Settings } from '@/types/interfaces.ts';
  import { computed, onBeforeUnmount, onMounted, ref, toRaw } from 'vue';
  import { useI18n } from 'vue-i18n';

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
  const { t } = useI18n();
  const viewState = ref(ConfiguratorViewState.DISCONNECTED);
  const settings = ref<Settings>({});

  //---------------------------------------------------
  //
  //  Computed Properties
  //
  //---------------------------------------------------
  const hasSettings = computed(() => {
    const setts = settings.value;
    return setts.lever1 &&
      setts.lever2 &&
      setts.leverPush1 &&
      setts.leverPush2 &&
      setts.touch &&
      setts.scale;
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
  onMounted(async () => {
    const isConnected = Bluetooth.connected;
    if (isConnected) {
      viewState.value = ConfiguratorViewState.READING_SETTINGS;
      settings.value = await Bluetooth.getSettings();
      viewState.value = ConfiguratorViewState.CONNECTED;
    }
  });
  // onBeforeUpdate(() => {});
  // onUpdated(() => {});
  // onActivated(() => {});
  // onDeactivated(() => {});
  onBeforeUnmount(() => {
    viewState.value = ConfiguratorViewState.DISCONNECTED;
  });
  // onUnmounted(() => {});

  //---------------------------------------------------
  //
  //  Methods
  //
  //---------------------------------------------------
  async function connect() {
    viewState.value = ConfiguratorViewState.CONNECTING;
    const isConnected = await Bluetooth.connect();
    if (isConnected) {
      viewState.value = ConfiguratorViewState.READING_SETTINGS;
      settings.value = await Bluetooth.getSettings();
      viewState.value = ConfiguratorViewState.CONNECTED;
    }
  }

  /*
  function disconnect() {
    Bluetooth.disconnect();
    viewState.value = ConfiguratorViewState.DISCONNECTED;
  }
  */

  async function handleSave() {
    console.log('attempting to write to bluetooth');
    viewState.value = ConfiguratorViewState.WRITING_SETTINGS;
    const isDone = await Bluetooth.writeSettings(toRaw(settings.value));
    if (isDone) {
      console.log('done');
      viewState.value = ConfiguratorViewState.CONNECTED;
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
  <div id="configurator">
    <div v-if="hasSettings" class="settings">
      <LeverSettings
        :title="t('configurator.lever.title')"
        :lever="1"
        v-model="settings.lever1"
      />
      <LeverPushSettings
        :title="t('configurator.leverpush.title')"
        :lever="1"
        v-model="settings.leverPush1"
      />
      <LeverSettings
        :title="t('configurator.lever.title')"
        :lever="2"
        v-model="settings.lever2"
      />
      <LeverPushSettings
        :title="t('configurator.leverpush.title')"
        :lever="2"
        v-model="settings.leverPush2"
      />
      <TouchSettings
        :title="t('configurator.touch.title')"
        v-model="settings.touch"
      />
      <ScaleSettings
        :title="t('configurator.scale.title')"
        v-model="settings.scale"
      />
    </div>
    <button @click="handleSave">Save</button>

    <template v-if="viewState === ConfiguratorViewState.DISCONNECTED">
      <div class="overlay">
        <button @click="connect">Connect</button>
      </div>
    </template>
    <template v-else-if="![ConfiguratorViewState.DISCONNECTED, ConfiguratorViewState.CONNECTED].includes(viewState)">
      <div class="overlay">
        {{ t(`configurator.viewstates.${viewState}`) }}
      </div>
    </template>
  </div>
</template>

<style lang="scss">
  #configurator {
    position: relative;

    & > div.overlay {
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: inherit;
      display: flex;
      justify-content: center;
      align-items: center;
      z-index: 2;

      &:before {
        content: '';
        position: absolute;
        inset: 0;
        width: 100%;
        height: 100%;
        background: var(--color-bg-primary);
        z-index: -1;
        opacity: 0.95;
      }
    }

    & > div.settings {
      height: inherit;
      width: 100%;
      overflow: hidden;
      overflow-y: scroll;
      padding-bottom: 64px;

      & > div {
        position: relative;
        margin-bottom: calc((var(--baseunit) * 3.5) + 1px);
        &:after {
          bottom: calc(var(--baseunit) * -2);
          left: 0;
          content: '';
          position: absolute;
          width: 100%;
          height: 1px;
          background: #111;
        }
        &:last-of-type {
          &:after {
            display: none;
          }
        }
      }
    }

    & > button {
      position: absolute;
      bottom: 4px;
      left: 50%;
      transform: translate(-50%, 0);
      background: var(--color-lever1);
      border: 5px solid var(--color-bg-primary);
      opacity: 0.7;
      border-radius: 100%;
      width: 48px;
      height: 48px;
      padding: 0;
      min-width: auto;
      text-transform: uppercase;
      font-size: 1rem;
    }
  }
</style>

<i18n />
