import { createRouter, createWebHistory } from 'vue-router';
import Index from '@/views/Index.vue';
import Configurator from '@/views/Configurator.vue';
import CCFaders from '@/views/CCFaders.vue';

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      name: 'index',
      component: Index,
    },
    {
      path: '/configurator',
      name: 'configurator',
      component: Configurator,
    },
    {
      path: '/cc-faders',
      name: 'cc-faders',
      component: CCFaders,
    },
  ],
  scrollBehavior(to, from, savedPosition) {
    if (savedPosition) {
      return savedPosition;
    }
    return { top: 0 };
  },
});

export default router;
