<template>
    <b-list-group-item action :to="`/userScript/${script.name}`">

        <div class="d-flex w-100 justify-content-between">
            <h5 class="mb-1">
                {{ script.name }}
            </h5>
        </div>
    </b-list-group-item>
</template>

<script>

export default {
    name: "UserScriptListItem",
    props: ["script"],
}
</script>