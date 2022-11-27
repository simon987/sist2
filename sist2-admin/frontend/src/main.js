import Vue from 'vue'

import { BootstrapVue, IconsPlugin } from 'bootstrap-vue'

import "bootstrap/dist/css/bootstrap.min.css"
import "bootstrap-vue/dist/bootstrap-vue.min.css"

Vue.use(BootstrapVue);
Vue.use(IconsPlugin);

import App from './App.vue';
import router from './router';
import store from './store';
import VueI18n from "vue-i18n";
import messages from "@/i18n/messages";

Vue.use(VueI18n);

const i18n = new VueI18n({
  locale: "en",
  messages: messages
});

Vue.config.productionTip = false

new Vue({
  router,
  store,
  i18n,
  render: h => h(App)
}).$mount('#app')
