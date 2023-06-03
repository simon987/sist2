<template>
    <b-list-group-item action :to="`/searchBackend/${backend.name}`">

        <div class="d-flex w-100 justify-content-between">
            <h5 class="mb-1">
                {{ backend.name }}
            </h5>

            <div>
                <b-badge v-if="backend.backend_type === 'sqlite'" variant="info">SQLite</b-badge>
                <b-badge v-else variant="info">Elasticsearch</b-badge>
            </div>
        </div>

    </b-list-group-item>
</template>

<script>

export default {
    name: "SearchBackendListItem",
    props: ["backend"],
}
</script>