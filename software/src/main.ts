import { createApp } from 'vue';
import { createPinia } from 'pinia';
import { createI18n } from 'vue-i18n';
import router from './router.ts';
import App from './App.vue';
import en from './locales/en.js';

const app = createApp(App);
app.use(createPinia());
const i18n = createI18n({
  legacy: false,
  fallbackWarn: false,
  missingWarn: false,
  warnHtmlMessage: false,
  messages: {
    en,
  },
});

app.use(i18n);
app.use(router);
app.mount('body');
