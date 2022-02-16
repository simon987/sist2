import Vue from "vue"
import VueRouter, {RouteConfig} from "vue-router"
import StatsPage from "../views/StatsPage.vue"
import Configuration from "../views/Configuration.vue"
import SearchPage from "@/views/SearchPage.vue";
import FilePage from "@/views/FilePage.vue";

Vue.use(VueRouter)

const routes: Array<RouteConfig> = [
    {
        path: "/",
        name: "SearchPage",
        component: SearchPage
    },
    {
        path: "/stats",
        name: "Stats",
        component: StatsPage
    },
    {
        path: "/config",
        name: "Configuration",
        component: Configuration
    },
    {
        path: "/file",
        name: "File",
        component: FilePage
    }
]

const router = new VueRouter({
    mode: "hash",
    base: process.env.BASE_URL,
    routes,
    scrollBehavior (to, from, savedPosition) {
        // return desired position
    }
})

export default router
