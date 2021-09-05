import '@babel/polyfill'
import 'mutationobserver-shim'
import Vue from 'vue'
import './plugins/bootstrap-vue'
import App from './App.vue'
import router from './router'
import store from './store'
import VueI18n from "vue-i18n";
import messages from "@/i18n/messages";


import VueRouter from "vue-router";

Vue.config.productionTip = false;

Vue.use(VueI18n);
Vue.use(VueRouter);

const i18n = new VueI18n({
    locale: "en",
    messages: messages
});

new Vue({
    router,
    store,
    i18n,
    render: h => h(App)
}).$mount("#app");
