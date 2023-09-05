<template>
    <b-list-group class="mt-3">
        <DocListItem v-for="doc in docs" :key="sid(doc)" :doc="doc"></DocListItem>
    </b-list-group>
</template>

<script>
import {sid} from "@/util";
import DocListItem from "@/components/DocListItem.vue";
import Vue from "vue";

export default Vue.extend({
    name: "DocList",
    components: {DocListItem},
    props: ["docs", "append"],
    mounted() {
        window.addEventListener("scroll", () => {
            const threshold = 400;
            const app = document.getElementById("app");

            if ((window.innerHeight + window.scrollY) >= app.offsetHeight - threshold) {
                this.append();
            }
        });
    },
    methods: {
        sid: sid
    }
});
</script>

<style>

.theme-black .list-group-item {
    background: #212121;
    color: #e0e0e0;

    border-bottom: none;
    border-left: none;
    border-right: none;
    border-radius: 0;
    padding: .25rem 0.5rem;
}

.theme-black .list-group-item:first-child {
    border-top: none;
}

</style>