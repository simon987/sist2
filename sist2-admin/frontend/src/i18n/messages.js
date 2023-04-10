export default {
    en: {
        start: "Start",
        stop: "Stop",
        go: "Go",
        online: "online",
        offline: "offline",
        delete: "Delete",
        runNow: "Index now",
        create: "Create",
        test: "Test",

        jobTitle: "job configuration",
        tasks: "Tasks",
        runningTasks: "Running tasks",
        frontends: "Frontends",
        jobDisabled: "There is no valid index for this job",
        status: "Status",

        taskHistory: "Task history",
        taskName: "Task name",
        taskStarted: "Started",
        taskDuration: "Duration",
        taskStatus: "Status",
        logs: "Logs",
        kill: "Kill",
        killConfirmation: "SIGTERM signal sent to sist2 process",
        killConfirmationTitle: "Confirmation",
        follow: "Follow",
        wholeFile: "Whole file",
        logLevel: "Log level",
        logMode: "Follow mode",
        logFile: "Reading log file",

        jobs: "Jobs",
        newJobName: "New job name",
        newJobHelp: "Create a new job to get started!",
        newFrontendName: "New frontend name",
        scanned: "last scan",
        autoStart: "Start automatically",

        runJobConfirmationTitle: "Task queued",
        runJobConfirmation: "Check the Tasks page to monitor the status.",

        extraQueryArgs: "Extra query arguments when launching from sist2-admin",
        customUrl: "Custom URL when launching from sist2-admin",

        selectJobs: "Select jobs",
        webOptions: {
            title: "Web options",
            esUrl: "Elasticsearch URL",
            esIndex: "Elasticsearch index name",
            esInsecure: "Do not verify SSL connections to Elasticsearch.",
            lang: "UI Language",
            bind: "Listen address",
            tagline: "Tagline in navbar",
            auth: "Basic auth in user:password format",
            tagAuth: "Basic auth in user:password format for tagging",
            auth0Audience: "Auth0 audience",
            auth0Domain: "Auth0 domain",
            auth0ClientId: "Auth0 client ID",
            auth0PublicKey: "Auth0 public key",
        },
        scanOptions: {
            title: "Scanning options",
            path: "Path",
            threads: "Number of threads",
            memThrottle: "Total memory threshold in MiB for scan throttling",
            thumbnailQuality: "Thumbnail quality, on a scale of 2 to 32, 2 being the best",
            thumbnailCount: "Number of thumbnails to generate. Set a value > 1 to create video previews, set to 0 to disable thumbnails.",
            thumbnailSize: "Thumbnail size, in pixels",
            contentSize: "Number of bytes to be extracted from text documents. Set to 0 to disable",
            rewriteUrl: "Serve files from this url instead of from disk",
            depth: "Scan up to this many subdirectories deep",
            archive: "Archive file mode",
            archivePassphrase: "Passphrase for encrypted archive files",
            ocrLang: "Tesseract language",
            ocrLangAlert: "You must select at least one language",
            ocrEbooks: "Enable OCR'ing of ebook files",
            ocrImages: "Enable OCR'ing of image files",
            exclude: "Files that match this regex will not be scanned",
            excludePlaceholder: "Exclude",
            fast: "Only index file names & mime type",
            checksums: "Calculate file checksums when scanning",
            readSubtitles: "Read subtitles from media files",
            memBuffer: "Maximum memory buffer size per thread in MiB for files inside archives",
            treemapThreshold: "Relative size threshold for treemap",
            optimizeIndex: "Defragment index file after scan to reduce its file size."
        },
        indexOptions: {
            title: "Indexing options",
            threads: "Number of threads",
            esUrl: "Elasticsearch URL",
            esIndex: "Elasticsearch index name",
            esInsecure: "Do not verify SSL connections to Elasticsearch.",
            batchSize: "Index batch size",
            script: "User script"
        },
        jobOptions: {
            title: "Job options",
            cron: "Job schedule",
            scheduleEnabled: "Enable scheduled re-scan",
            noJobAvailable: "No jobs available.",
            desktopNotifications: "Desktop notifications"
        },
        frontendOptions: {
            title: "Frontend options",
            noJobSelectedWarning: "You must select at least one job to start this frontend"
        },
        notifications: {
            indexCompleted: "Task completed for [$JOB$]"
        }
    }
}