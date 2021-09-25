export default {
    en: {
        searchBar: {
            simple: "Search",
            advanced: "Advanced search",
            fuzzy: "Fuzzy"
        },
        download: "Download",
        and: "and",
        page: "page",
        pages: "pages",
        mimeTypes: "Media types",
        tags: "Tags",
        help: {
            simpleSearch: "Simple search",
            advancedSearch: "Advanced search",
            help: "Help",
            term: "<TERM>",
            and: "AND operator",
            or: "OR operator",
            not: "negates a single term",
            quotes: "will match the enclosed sequence of terms in that specific order",
            prefix: "will match any term with a given prefix when used at the end of a word",
            parens: "used to group expressions",
            tildeTerm: "match a term with a given edit distance",
            tildePhrase: "match a phrase with a given number of allowed intervening unmatched words",
            example1:
                "For example: <code>\"fried eggs\" +(eggplant | potato) -frittata</code> will match the " +
                "phrase <i>fried eggs</i> and either <i>eggplant</i> or <i>potato</i>, but will ignore results " +
                "containing <i>frittata</i>.",
            defaultOperator:
                "When neither <code>+</code> or <code>|</code> is specified, the default operator is " +
                "<code>+</code> (and).",
            fuzzy:
                "When the <b>Fuzzy</b> option is checked, partial matches based on 3-grams are also returned.",
            moreInfoSimple: "For more information, see <a target=\"_blank\" " +
                "rel=\"noreferrer\" href=\"//www.elastic.co/guide/en/elasticsearch/reference/current/query-dsl-simple-query-string-query.html\">Elasticsearch documentation</a>",
            moreInfoAdvanced: "For documentation about the advanced search mode, see <a target=\"_blank\" rel=\"noreferrer\" href=\"//www.elastic.co/guide/en/elasticsearch/reference/current/query-dsl-query-string-query.html#query-string-syntax\">Elasticsearch documentation</a>"
        },
        config: "Configuration",
        configDescription: "Configuration is saved in real time for this browser.",
        configReset: "Reset configuration",
        searchOptions: "Search options",
        treemapOptions: "Treemap options",
        displayOptions: "Display options",
        opt: {
            lang: "Language",
            highlight: "Enable highlighting",
            fuzzy: "Set fuzzy search by default",
            searchInPath: "Enable matching query against document path",
            suggestPath: "Enable auto-complete in path filter bar",
            fragmentSize: "Highlight context size in characters",
            queryMode: "Search mode",
            displayMode: "Display",
            columns: "Column count",
            treemapType: "Treemap type",
            treemapTiling: "Treemap tiling",
            treemapColorGroupingDepth: "Treemap color grouping depth (flat)",
            treemapColor: "Treemap color (cascaded)",
            treemapSize: "Treemap size",
            theme: "Theme",
            lightboxLoadOnlyCurrent: "Do not preload full-size images for adjacent slides in image viewer.",
            slideDuration: "Slide duration",
            resultSize: "Number of results per page",
            tagOrOperator: "Use OR operator when specifying multiple tags.",
            hideDuplicates: "Hide duplicate results based on checksum"
        },
        queryMode: {
            simple: "Simple",
            advanced: "Advanced",
        },
        lang: {
            en: "English",
            fr: "Français"
        },
        displayMode: {
            grid: "Grid",
            list: "List",
        },
        columns: {
            auto: "Auto"
        },
        treemapType: {
            cascaded: "Cascaded",
            flat: "Flat (compact)"
        },
        treemapSize: {
            small: "Small",
            medium: "Medium",
            large: "Large",
            xLarge: "xLarge",
            xxLarge: "xxLarge",
            custom: "Custom",
        },
        treemapTiling: {
            binary: "Binary",
            squarify: "Squarify",
            slice: "Slice",
            dice: "Dice",
            sliceDice: "Slice & Dice",
        },
        theme: {
            light: "Light",
            black: "Black"
        },
        hit: "hit",
        hits: "hits",
        details: "Details",
        stats: "Stats",
        queryTime: "Query time",
        totalSize: "Total size",
        pathBar: {
            placeholder: "Filter path",
            modalTitle: "Select path"
        },
        debug: "Debug information",
        debugDescription: "Information useful for debugging. If you encounter bugs or have suggestions for" +
            " new features, please submit a new issue <a href='https://github.com/simon987/sist2/issues/new/choose'>here</a>.",
        tagline: "Tagline",
        toast: {
            esConnErrTitle: "Elasticsearch connection error",
            esConnErr: "sist2 web module encountered an error while connecting to Elasticsearch." +
                " See server logs for more information.",
            esQueryErrTitle: "Query error",
            esQueryErr: "Could not parse or execute query, please check the Advanced search documentation. " +
                "See server logs for more information.",
            dupeTagTitle: "Duplicate tag",
            dupeTag: "This tag already exists for this document."
        },
        saveTagModalTitle: "Add tag",
        saveTagPlaceholder: "Tag name",
        confirm: "Confirm",
        indexPickerPlaceholder: "Select indices",
        sort: {
            relevance: "Relevance",
            dateAsc: "Date (Older first)",
            dateDesc: "Date (Newer first)",
            sizeAsc: "Size (Smaller first)",
            sizeDesc: "Size (Larger first)",
            random: "Random",
        },
        d3: {
            mimeCount: "File count distribution by media type",
            mimeSize: "Size distribution by media type",
            dateHistogram: "File modification time distribution",
            sizeHistogram: "File size distribution",
        }
    },
    fr: {
        searchBar: {
            simple: "Recherche",
            advanced: "Recherche avancée",
            fuzzy: "Approximatif"
        },
        download: "Télécharger",
        and: "et",
        page: "page",
        pages: "pages",
        mimeTypes: "Types de médias",
        tags: "Tags",
        help: {
            simpleSearch: "Recherche simple",
            advancedSearch: "Recherche avancée",
            help: "Aide",
            term: "<TERME>",
            and: "opérator ET",
            or: "opérator OU",
            not: "exclut un terme",
            quotes: "recherche la séquence de termes dans cet ordre spécifique.",
            prefix: "lorsqu'utilisé à la fin d'un mot, recherche tous les termes avec le préfixe donné.",
            parens: "utilisé pour regrouper des expressions",
            tildeTerm: "recherche un terme avec une distance d'édition donnée",
            tildePhrase: "recherche une phrase avec un nombre donné de mots intermédiaires tolérés",
            example1:
                "Par exemple: <code>\"fried eggs\" +(eggplant | potato) -frittata</code> va rechercher la " +
                "phrase <i>fried eggs</i> et soit <i>eggplant</i> ou <i>potato</i>, mais vas exlure les résultats " +
                "qui contiennent <i>frittata</i>.",
            defaultOperator:
                "Lorsqu'aucun des opérateurs <code>+</code> ou <code>|</code> sont spécifiés, l'opérateur par défaut " +
                "est <code>+</code> (ET).",
            fuzzy:
                "Lorsque l'option <b>Approximatif</b> est activée, les résultats partiels basés sur les trigrammes sont" +
                " également inclus.",
            moreInfoSimple: "Pour plus d'information, voir <a target=\"_blank\" " +
                "rel=\"noreferrer\" href=\"//www.elastic.co/guide/en/elasticsearch/reference/current/query-dsl-simple-query-string-query.html\">documentation Elasticsearch</a>",
            moreInfoAdvanced: "Pour plus d'information sur la recherche avancée, voir <a target=\"_blank\" rel=\"noreferrer\" href=\"//www.elastic.co/guide/en/elasticsearch/reference/current/query-dsl-query-string-query.html#query-string-syntax\">documentation Elasticsearch</a>"
        },
        config: "Configuration",
        configDescription: "La configuration est enregistrée en temps réel pour ce navigateur.",
        configReset: "Réinitialiser la configuration",
        searchOptions: "Options de recherche",
        treemapOptions: "Options du Treemap",
        displayOptions: "Options d'affichage",
        opt: {
            lang: "Langue",
            highlight: "Activer le surlignage",
            fuzzy: "Activer la recherche approximative par défaut",
            searchInPath: "Activer la recherche dans le chemin des documents",
            suggestPath: "Activer l'autocomplétion dans la barre de filtre de chemin",
            fragmentSize: "Longueur du contexte de surlignage, en nombre de caractères",
            queryMode: "Mode de recherche",
            displayMode: "Affichage",
            columns: "Nombre de colonnes",
            treemapType: "Type de Treemap",
            treemapTiling: "Treemap tiling",
            treemapColorGroupingDepth: "Groupage de couleur du Treemap (plat)",
            treemapColor: "Couleur du Treemap (en cascade)",
            treemapSize: "Taille du Treemap",
            theme: "Thème",
            lightboxLoadOnlyCurrent: "Désactiver le chargement des diapositives adjacentes pour le visualiseur d'images",
            slideDuration: "Durée des diapositives",
            resultSize: "Nombre de résultats par page",
            tagOrOperator: "Utiliser l'opérateur OU lors de la spécification de plusieurs tags",
            hideDuplicates: "Masquer les résultats en double"
        },
        queryMode: {
            simple: "Simple",
            advanced: "Avancé",
        },
        lang: {
            en: "English",
            fr: "Français"
        },
        displayMode: {
            grid: "Grille",
            list: "Liste",
        },
        columns: {
            auto: "Auto"
        },
        treemapType: {
            cascaded: "En cascade",
            flat: "Plat (compact)"
        },
        treemapSize: {
            small: "Petit",
            medium: "Moyen",
            large: "Grand",
            xLarge: "xGrand",
            xxLarge: "xxGrand",
            custom: "Personnalisé",
        },
        treemapTiling: {
            binary: "Binary",
            squarify: "Squarify",
            slice: "Slice",
            dice: "Dice",
            sliceDice: "Slice & Dice",
        },
        theme: {
            light: "Clair",
            black: "Noir"
        },
        hit: "résultat",
        hits: "résultats",
        details: "Détails",
        stats: "Stats",
        queryTime: "Durée de la requête",
        totalSize: "Taille totale",
        pathBar: {
            placeholder: "Filtrer le chemin",
            modalTitle: "Sélectionner le chemin"
        },
        debug: "Information de débogage",
        debugDescription: "Informations utiles pour le débogage\n" +
            "Si vous rencontrez des bogues ou si vous avez des suggestions pour de nouvelles fonctionnalités," +
            " veuillez soumettre un nouvel Issue <a href='https://github.com/simon987/sist2/issues/new/choose'>ici</a>.",
        tagline: "Tagline",
        toast: {
            esConnErrTitle: "Erreur de connexion Elasticsearch",
            esConnErr: "Le module web a rencontré une erreur lors de la connexion à Elasticsearch." +
                " Consultez les journaux du serveur pour plus d'informations..",
            esQueryErrTitle: "Erreur de requête",
            esQueryErr: "Impossible d'analyser ou d'exécuter la requête, veuillez consulter la documentation sur la " +
                "recherche avancée. Voir les journaux du serveur pour plus d'informations.",
            dupeTagTitle: "Tag en double",
            dupeTag: "Ce tag existe déjà pour ce document."
        },
        saveTagModalTitle: "Ajouter un tag",
        saveTagPlaceholder: "Nom du tag",
        confirm: "Confirmer",
        indexPickerPlaceholder: "Sélectionner un index",
        sort: {
            relevance: "Pertinence",
            dateAsc: "Date (Plus ancient)",
            dateDesc: "Date (Plus récent)",
            sizeAsc: "Taille (Plus petit)",
            sizeDesc: "Taille (Plus grand)",
            random: "Aléatoire",
        },
        d3: {
            mimeCount: "Distribution du nombre de fichiers par type de média",
            mimeSize: "Distribution des tailles de fichiers par type de média",
            dateHistogram: "Distribution des dates de modification",
            sizeHistogram: "Distribution des tailles de fichier",
        }
    }
}