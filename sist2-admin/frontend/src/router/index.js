import Vue from 'vue'
import VueRouter from 'vue-router'
import Home from '../views/Home.vue'
import Job from "@/views/Job";
import Tasks from "@/views/Tasks";
import Frontend from "@/views/Frontend";
import Tail from "@/views/Tail";
import SearchBackend from "@/views/SearchBackend.vue";
import UserScript from "@/views/UserScript.vue";

Vue.use(VueRouter);

const routes = [
  {
    path: "/task",
    name: "Tasks",
    component: Tasks
  },
  {
    path: "/:tab?",
    name: "Home",
    component: Home
  },
  {
    path: "/job/:name",
    name: "Job",
    component: Job
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
    path: "/userScript/:name",
    name: "UserScript",
    component: UserScript
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
