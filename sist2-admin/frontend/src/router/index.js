import Vue from 'vue'
import VueRouter from 'vue-router'
import Home from '../views/Home.vue'
import Job from "@/views/Job";
import Tasks from "@/views/Tasks";
import Frontend from "@/views/Frontend";
import Tail from "@/views/Tail";
import SearchBackend from "@/views/SearchBackend.vue";

Vue.use(VueRouter);

const routes = [
  {
    path: "/",
    name: "Home",
    component: Home
  },
  {
    path: "/job/:name",
    name: "Job",
    component: Job
  },
  {
    path: "/task/",
    name: "Tasks",
    component: Tasks
  },
  {
    path: "/frontend/:name",
    name: "Frontend",
    component: Frontend
  },
  {
    path: "/searchBackend/:name",
    name: "SearchBackend",
    component: SearchBackend
  },
  {
    path: "/log/:taskId",
    name: "Tail",
    component: Tail
  },
]

const router = new VueRouter({
  mode: "hash",
  base: process.env.BASE_URL,
  routes
})

export default router
