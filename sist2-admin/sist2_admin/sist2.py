import datetime
import json
import logging
import os.path
from datetime import datetime
from io import TextIOWrapper
from logging import FileHandler
from subprocess import Popen, PIPE
from tempfile import NamedTemporaryFile
from threading import Thread
from typing import List

from pydantic import BaseModel

from config import logger, LOG_FOLDER


class Sist2Version:
    def __init__(self, version: str):
        self._version = version

        self.major, self.minor, self.patch = [int(x) for x in version.split(".")]

    def __str__(self):
        return f"{self.major}.{self.minor}.{self.patch}"


class WebOptions(BaseModel):
    indices: List[str] = []
    es_url: str = "http://elasticsearch:9200"
    es_insecure_ssl: bool = False
    es_index: str = "sist2"
    bind: str = "0.0.0.0:4090"
    auth: str = None
    tag_auth: str = None
    tagline: str = "Lightning-fast file system indexer and search tool"
    dev: bool = False
    lang: str = "en"
    auth0_audience: str = None
    auth0_domain: str = None
    auth0_client_id: str = None
    auth0_public_key: str = None
    auth0_public_key_file: str = None

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def args(self):
        args = ["web", f"--es-url={self.es_url}", f"--bind={self.bind}",
                f"--tagline={self.tagline}", f"--lang={self.lang}"]

        if self.auth0_audience:
            args.append(f"--auth0-audience={self.auth0_audience}")
        if self.auth0_domain:
            args.append(f"--auth0-domain={self.auth0_domain}")
        if self.auth0_client_id:
            args.append(f"--auth0-client-id={self.auth0_client_id}")
        if self.auth0_public_key_file:
            args.append(f"--auth0-public-key-file={self.auth0_public_key_file}")
        if self.es_insecure_ssl:
            args.append(f"--es-insecure-ssl")
        if self.auth:
            args.append(f"--auth={self.auth}")
        if self.tag_auth:
            args.append(f"--tag-auth={self.tag_auth}")
        if self.dev:
            args.append(f"--dev")

        args.extend(self.indices)

        return args


class IndexOptions(BaseModel):
    path: str = None
    threads: int = 1
    es_url: str = "http://elasticsearch:9200"
    es_insecure_ssl: bool = False
    es_index: str = "sist2"
    incremental_index: bool = True
    script: str = ""
    script_file: str = None
    batch_size: int = 70

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def args(self):

        args = ["index", self.path, f"--threads={self.threads}", f"--es-url={self.es_url}",
                f"--es-index={self.es_index}", f"--batch-size={self.batch_size}"]

        if self.script_file:
            args.append(f"--script-file={self.script_file}")
        if self.es_insecure_ssl:
            args.append(f"--es-insecure-ssl")
        if self.incremental_index:
            args.append(f"--incremental-index")

        return args


ARCHIVE_SKIP = "skip"
ARCHIVE_LIST = "list"
ARCHIVE_SHALLOW = "shallow"
ARCHIVE_RECURSE = "recurse"


class ScanOptions(BaseModel):
    path: str
    threads: int = 1
    thumbnail_quality: int = 2
    thumbnail_size: int = 552
    thumbnail_count: int = 1
    content_size: int = 32768
    depth: int = -1
    archive: str = ARCHIVE_RECURSE
    archive_passphrase: str = None
    ocr_lang: str = None
    ocr_images: bool = False
    ocr_ebooks: bool = False
    exclude: str = None
    fast: bool = False
    treemap_threshold: float = 0.0005
    mem_buffer: int = 2000
    read_subtitles: bool = False
    fast_epub: bool = False
    checksums: bool = False
    incremental: bool = True
    optimize_index: bool = False
    output: str = None
    name: str = None
    rewrite_url: str = None
    list_file: str = None

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def args(self):
        args = ["scan", self.path, f"--threads={self.threads}", f"--thumbnail-quality={self.thumbnail_quality}",
                f"--thumbnail-count={self.thumbnail_count}", f"--thumbnail-size={self.thumbnail_size}",
                f"--content-size={self.content_size}", f"--output={self.output}", f"--depth={self.depth}",
                f"--archive={self.archive}", f"--mem-buffer={self.mem_buffer}"]

        if self.incremental:
            args.append(f"--incremental")
        if self.optimize_index:
            args.append(f"--optimize-index")
        if self.rewrite_url:
            args.append(f"--rewrite-url={self.rewrite_url}")
        if self.name:
            args.append(f"--name={self.name}")
        if self.archive_passphrase:
            args.append(f"--archive-passphrase={self.archive_passphrase}")
        if self.ocr_lang:
            args.append(f"--ocr-lang={self.ocr_lang}")
        if self.ocr_ebooks:
            args.append(f"--ocr-ebooks")
        if self.ocr_images:
            args.append(f"--ocr-images")
        if self.exclude:
            args.append(f"--exclude={self.exclude}")
        if self.fast:
            args.append(f"--fast")
        if self.treemap_threshold:
            args.append(f"--treemap-threshold={self.treemap_threshold}")
        if self.read_subtitles:
            args.append(f"--read-subtitles")
        if self.fast_epub:
            args.append(f"--fast-epub")
        if self.checksums:
            args.append(f"--checksums")
        if self.list_file:
            args.append(f"--list_file={self.list_file}")

        return args


class Sist2Index:
    def __init__(self, path):
        self.path = path

        with open(os.path.join(path, "descriptor.json")) as f:
            self._descriptor = json.load(f)

    def to_json(self):
        return {
            "path": self.path,
            "version": self.version(),
            "timestamp": self.timestamp(),
            "name": self.name()
        }

    def version(self) -> Sist2Version:
        return Sist2Version(self._descriptor["version"])

    def timestamp(self) -> datetime:
        return datetime.fromtimestamp(self._descriptor["timestamp"])

    def name(self) -> str:
        return self._descriptor["name"]


class Sist2:

    def __init__(self, bin_path: str, data_directory: str):
        self._bin_path = bin_path
        self._data_dir = data_directory

    def index(self, options: IndexOptions, logs_cb):

        if options.script:
            with NamedTemporaryFile("w", prefix="sist2-admin", suffix=".painless", delete=False) as f:
                f.write(options.script)
            options.script_file = f.name
        else:
            options.script_file = None

        args = [
            self._bin_path,
            *options.args(),
            "--json-logs",
            "--very-verbose"
        ]
        proc = Popen(args, stdout=PIPE, stderr=PIPE)

        t_stderr = Thread(target=self._consume_logs_stderr, args=(logs_cb, proc))
        t_stderr.start()

        self._consume_logs_stdout(logs_cb, proc)

        t_stderr.join()

        return proc.returncode

    def scan(self, options: ScanOptions, logs_cb, set_pid_cb):

        if options.output is None:
            options.output = os.path.join(
                self._data_dir,
                f"scan-{options.name.replace('/', '_')}-{datetime.now()}.sist2"
            )

        args = [
            self._bin_path,
            *options.args(),
            "--json-logs",
            "--very-verbose"
        ]

        logs_cb({"sist2-admin": f"Starting sist2 command with args {args}"})

        proc = Popen(args, stdout=PIPE, stderr=PIPE)

        set_pid_cb(proc.pid)

        t_stderr = Thread(target=self._consume_logs_stderr, args=(logs_cb, proc))
        t_stderr.start()

        self._consume_logs_stdout(logs_cb, proc)

        t_stderr.join()

        return proc.returncode

    @staticmethod
    def _consume_logs_stderr(logs_cb, proc):
        pipe_wrapper = TextIOWrapper(proc.stderr, encoding="utf8", errors="ignore")
        try:
            for line in pipe_wrapper:
                if line.strip() == "":
                    continue
                logs_cb({"stderr": line})
        finally:
            proc.wait()
            pipe_wrapper.close()

    @staticmethod
    def _consume_logs_stdout(logs_cb, proc):
        pipe_wrapper = TextIOWrapper(proc.stdout, encoding="utf8", errors="ignore")
        for line in pipe_wrapper:
            try:
                if line.strip() == "":
                    continue
                log_object = json.loads(line)
                logs_cb(log_object)
            except Exception as e:
                try:
                    logs_cb({"sist2-admin": f"Could not decode log line: {line}; {e}"})
                except NameError:
                    pass

    def web(self, options: WebOptions, name: str):

        if options.auth0_public_key:
            with NamedTemporaryFile("w", prefix="sist2-admin", suffix=".txt", delete=False) as f:
                f.write(options.auth0_public_key)
            options.auth0_public_key_file = f.name
        else:
            options.auth0_public_key_file = None

        args = [
            self._bin_path,
            *options.args()
        ]

        web_logger = logging.Logger(name=f"sist2-frontend-{name}")
        web_logger.addHandler(FileHandler(os.path.join(LOG_FOLDER, f"frontend-{name}.log")))

        def logs_cb(message):
            web_logger.info(json.dumps(message))

        logger.info(f"Starting frontend {' '.join(args)}")

        proc = Popen(args, stdout=PIPE, stderr=PIPE)

        t_stderr = Thread(target=self._consume_logs_stderr, args=(logs_cb, proc))
        t_stderr.start()

        t_stdout = Thread(target=self._consume_logs_stdout, args=(logs_cb, proc))
        t_stdout.start()

        return proc.pid
