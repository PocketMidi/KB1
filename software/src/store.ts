import { defineStore } from 'pinia';
import { useStorage } from '@vueuse/core';

const STORE_NAME = 'configstore';

export const useStore = defineStore('configstore', {
  state: () => ({
    configuration: useStorage(STORE_NAME, {
      server: {
        host: '127.0.0.1',
        port: 5566,
      },
      actions: `[
  {
    "type": "",
    "action": "",
    "radiogroup": "",
    "image": "",
    "color": "",
    "label": "",
    "value": ""
  }
]`,
    }),
  }),
  getters: {
    config: (state) => state.configuration,
    server: (state) => state.configuration.server,
    actions: (state) => state.configuration.actions,
  },
  actions: {
    update(partialConfig) {
      // noinspection JSConstantReassignment
      this.configuration = {
        ...this.configuration,
        ...partialConfig,
      };
    },
  },
});
