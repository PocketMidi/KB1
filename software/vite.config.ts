import { fileURLToPath, URL } from 'node:url';
import { resolve, dirname } from 'node:path'

import { defineConfig, type UserConfig } from 'vite';
import vue from '@vitejs/plugin-vue';
import svgLoader from 'vite-svg-loader';
import VueI18nPlugin from '@intlify/unplugin-vue-i18n/vite';

// https://vitejs.dev/config/
export default defineConfig((): UserConfig => {
  return {
    plugins: [
      vue(),
      svgLoader({
        svgoConfig: {
          multipass: true,
          plugins: [
            {
              name: 'preset-default',
              // @ts-ignore
              prefixIds: true,
              params: {
                overrides: {},
              },
            },
          ],
        }
      }),
      VueI18nPlugin({
        runtimeOnly: false,
        strictMessage: false,
        include: resolve(dirname('./src/locales/**')),
      }),
    ],
    css: {
      preprocessorOptions: {
        scss: {
          api: 'modern-compiler'
        }
      }
    },
    server: {},
    resolve: {
      alias: {
        '@': fileURLToPath(new URL('./src', import.meta.url))
      }
    }
  };
});
