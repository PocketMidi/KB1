import { createRouter, createWebHistory } from 'vue-router';
import Index from '@/views/Index.vue';
import Control from '@/views/Control.vue';
import Overlay from '@/views/Overlay.vue';

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      name: 'index',
      component: Index,
    },
    {
      path: '/control',
      name: 'control',
      component: Control,
    },
    {
      path: '/overlay',
      name: 'overlay',
      component: Overlay,
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
