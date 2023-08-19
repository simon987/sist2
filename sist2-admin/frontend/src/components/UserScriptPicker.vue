<template>
    <b-progress v-if="loading" striped animated value="100"></b-progress>

    <b-row v-else>
        <b-col cols="6">
            <h5>Selected scripts</h5>
            <b-list-group>
                <b-list-group-item v-for="script in selectedScripts" :key="script"
                                   button
                                   @click="onRemoveScript(script)"
                                   class="d-flex justify-content-between align-items-center">
                    {{ script }}
                    <b-button-group>
                        <b-button variant="light" @click.stop="moveUpScript(script)">↑</b-button>
                        <b-button variant="light" @click.stop="moveDownScript(script)">↓</b-button>
                    </b-button-group>
                </b-list-group-item>
            </b-list-group>
        </b-col>
        <b-col cols="6">
            <h5>Available scripts</h5>
            <b-list-group>
                <b-list-group-item v-for="script in availableScripts" :key="script" button
                                   @click="onSelectScript(script)">
                    {{ script }}
                </b-list-group-item>
            </b-list-group>
        </b-col>
    </b-row>

  <!--    <b-checkbox-group v-else :options="scripts" stacked :checked="selectedScripts"-->
  <!--                      @input="$emit('change', $event)"></b-checkbox-group>-->
</template>

<script>
import Sist2AdminApi from "@/Sist2AdminApi";

export default {
    name: "UserScriptPicker",
    props: ["selectedScripts"],
    data() {
        return {
            loading: true,
            scripts: []
        }
    },
    computed: {
        availableScripts() {
            return this.scripts.filter(script => !this.selectedScripts.includes(script))
        }
    },
    mounted() {
        Sist2AdminApi.getUserScripts().then(resp => {
            this.scripts = resp.data.map(script => script.name);
            this.loading = false;
        });
    },
    methods: {
        onSelectScript(name) {
            this.selectedScripts.push(name);
            this.$emit("change", this.selectedScripts)
        },
        onRemoveScript(name) {
            this.selectedScripts.splice(this.selectedScripts.indexOf(name), 1);
            this.$emit("change", this.selectedScripts);
        },
        moveUpScript(name) {
            const index = this.selectedScripts.indexOf(name);
            if (index > 0) {
                this.selectedScripts.splice(index, 1);
                this.selectedScripts.splice(index - 1, 0, name);
            }
            this.$emit("change", this.selectedScripts);
        },
        moveDownScript(name) {
            const index = this.selectedScripts.indexOf(name);
            if (index < this.selectedScripts.length - 1) {
                this.selectedScripts.splice(index, 1);
                this.selectedScripts.splice(index + 1, 0, name);
            }
            this.$emit("change", this.selectedScripts);
        }
    }
}
</script>

<style scoped>
</style>