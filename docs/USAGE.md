# Usage

*More examples (specifically with docker/compose) are in progress*

* [scan](#scan)
    * [options](#scan-options)
    * [examples](#scan-examples)
    * [index format](#index-format)
* [index](#index)
    * [options](#index-options)
    * [examples](#index-examples)
* [web](#web)
    * [options](#web-options)
    * [examples](#web-examples)
    * [rewrite_url](#rewrite_url)
    * [link to specific indices](#link-to-specific-indices)
* [exec-script](#exec-script)
* [tagging](#tagging)
* [sidecar files](#sidecar-files)

```
Usage: sist2 scan [OPTION]... PATH
   or: sist2 index [OPTION]... INDEX
   or: sist2 web [OPTION]... INDEX...
   or: sist2 exec-script [OPTION]... INDEX
Lightning-fast file system indexer and search tool.

    -h, --help                    show this help message and exit
    -v, --version                 Show version and exit
    --verbose                     Turn on logging
    --very-verbose                Turn on debug messages

Scan options
    -t, --threads=<int>           Number of threads. DEFAULT=1
    -q, --quality=<flt>           Thumbnail quality, on a scale of 1.0 to 31.0, 1.0 being the best. DEFAULT=5
    --size=<int>                  Thumbnail size, in pixels. Use negative value to disable. DEFAULT=500
    --content-size=<int>          Number of bytes to be extracted from text documents. Use negative value to disable. DEFAULT=32768
    --incremental=<str>           Reuse an existing index and only scan modified files.
    -o, --output=<str>            Output directory. DEFAULT=index.sist2/
    --rewrite-url=<str>           Serve files from this url instead of from disk.
    --name=<str>                  Index display name. DEFAULT: (name of the directory)
    --depth=<int>                 Scan up to DEPTH subdirectories deep. Use 0 to only scan files in PATH. DEFAULT: -1
    --archive=<str>               Archive file mode (skip|list|shallow|recurse). skip: Don't parse, list: only get file names as text, shallow: Don't parse archives inside archives. DEFAULT: recurse
    --ocr=<str>                   Tesseract language (use tesseract --list-langs to see which are installed on your machine)
    -e, --exclude=<str>           Files that match this regex will not be scanned
    --fast                        Only index file names & mime type
    --treemap-threshold=<str>     Relative size threshold for treemap (see USAGE.md). DEFAULT: 0.0005
    --mem-buffer=<int>            Maximum memory buffer size per thread in MB for files inside archives (see USAGE.md). DEFAULT: 2000

Index options
    -t, --threads=<int>           Number of threads. DEFAULT=1
    --es-url=<str>                Elasticsearch url with port. DEFAULT=http://localhost:9200
    --es-index=<str>              Elasticsearch index name. DEFAULT=sist2
    -p, --print                   Just print JSON documents to stdout.
    --script-file=<str>           Path to user script.
    --async-script                Execute user script asynchronously.
    --batch-size=<int>            Index batch size. DEFAULT: 100
    -f, --force-reset             Reset Elasticsearch mappings and settings. (You must use this option the first time you use the index command)

Web options
    --es-url=<str>                Elasticsearch url. DEFAULT=http://localhost:9200
    --es-index=<str>              Elasticsearch index name. DEFAULT=sist2
    --bind=<str>                  Listen on this address. DEFAULT=localhost:4090
    --auth=<str>                  Basic auth in user:password format
    --tag-auth=<str>              Basic auth in user:password format for tagging

Exec-script options
    --es-url=<str>                Elasticsearch url. DEFAULT=http://localhost:9200
    --es-index=<str>              Elasticsearch index name. DEFAULT=sist2
    --script-file=<str>           Path to user script.
    --async-script                Execute user script asynchronously.
Made by simon987 <me@simon987.net>. Released under GPL-3.0
```

## Scan

### Scan options

* `-t, --threads` 
      Number of threads for file parsing. **Do not set a number higher than `$(nproc)` or `$(Get-WmiObject Win32_ComputerSystem).NumberOfLogicalProcessors` in Windows!**
* `-q, --quality` 
    Thumbnail quality, on a scale of 1.0 to 31.0, 1.0 being the best. *Does not affect PDF thumbnails quality*
* `--size` 
    Thumbnail size in pixels.
* `--content-size` 
    Number of bytes of text to be extracted from the content of files (plain text and PDFs).
    Repeated whitespace and special characters do not count toward this limit.
* `--incremental`
    Specify an existing index. Information about files in this index that were not modified (based on *mtime* attribute)
    will be copied to the new index and will not be parsed again.
* `-o, --output` Output directory. 
* `--rewrite-url` Set the `rewrite_url` option for the web module (See [rewrite_url](#rewrite_url)) 
* `--name` Set the `name` option for the web module
* `--depth` Maximum scan dept. Set to 0 only scan files directly in the root directory, set to -1 for infinite depth
* `--archive` Archive file mode.
    * skip: Don't parse
    * list: Only get file names as text
    * shallow: Don't parse archives inside archives.
    * recurse: Scan archives recursively (default)
* `--ocr` See [OCR](../README.md#OCR)
* `-e, --exclude` Regex pattern to exclude files. A file is excluded if the pattern matches any 
    part of the full absolute path.
    
    Examples: 
    * `-e ".*\.ttf"`: Ignore ttf files
    * `-e ".*\.(ttf|rar)"`: Ignore ttf and rar files
    * `-e "^/mnt/backups/"`: Ignore all files in the `/mnt/backups/` directory
    * `-e "^/mnt/Data[12]/"`: Ignore all files in the `/mnt/Data1/` and `/mnt/Data2/` directory
    * `-e "(^/usr/)|(^/var/)|(^/media/DRIVE-A/tmp/)|(^/media/DRIVE-B/Trash/)"` Exclude the
     `/usr`, `/var`, `/media/DRIVE-A/tmp`, `/media/DRIVE-B/Trash` directories
* `--fast` Only index file names and mime type
* `--treemap-threshold` Directories smaller than (`treemap-threshold` * `<total size of the index>`)
    will not be considered for the disk utilisation visualization; their size will be added to
    the parent directory. If the parent directory is still smaller than the threshold, it will also be "merged upwards"
    and so on.
    
    In effect, smaller `treemap-threshold` values will yield a more detailed 
    (but also a more cluttered and harder to read) visualization. 
    
* `--mem-buffer` Maximum memory buffer size in MB (per thread) for files inside archives. Media files 
    larger than this number will be read sequentially and no *seek* operations will be supported.

    To check if a media file can be parsed without *seek*, execute `cat file.mp4 | ffprobe -`

### Scan examples

Simple scan
```bash
sist2 scan ~/Documents

sist2 scan \
    --threads 4 --content-size 16000000 --quality 1.0 --archive shallow \
    --name "My Documents" --rewrite-url "http://nas.domain.local/My Documents/" \
    ~/Documents -o ./documents.idx/
```

Incremental scan
```
sist2 scan --incremental ./orig_idx/ -o ./updated_idx/ ~/Documents
```

### Index format

A typical `binary` type index structure looks like this:
```
documents.idx/
├── descriptor.json
├── _index_139965416830720
├── _index_139965425223424
├── _index_139965433616128
├── _index_139965442008832
├── _index_139965442008832
├── treemap.csv
├── agg_mime.csv
├── agg_date.csv
├── add_size.csv
├── thumbs/
|   ├── data.mdb
|   └── lock.mdb
├── tags/
|   ├── data.mdb
|   └── lock.mdb
└── meta/
    ├── data.mdb
    └── lock.mdb
```

The `_index_*` files contain the raw binary index data and are not meant to be
read by other applications. The format is generally compatible across different 
sist2 versions.

The `thumbs/` folder is a [LMDB](https://en.wikipedia.org/wiki/Lightning_Memory-Mapped_Database)
database containing the thumbnails.

The `descriptor.json` file contains general information about the index. The 
following fields are safe to modify manually: `root`, `name`, [rewrite_url](#rewrite_url) and `timestamp`.

The `.csv` are pre-computed aggregations necessary for the stats page.


*Advanced usage*

Instead of using the `scan` module, you can also import an index generated
by a third party application. The 'external' index must have the following format:

```
my_index/
├── descriptor.json
├── _index_0
└── thumbs/
|   ├── data.mdb
|   └── lock.mdb
└── meta/
    └── <empty>
```

*descriptor.json*:
```json
{
    "uuid": "<valid UUID4>",
    "version": "_external_v1",
    "root": "(optional)",
    "name": "<name>",
    "rewrite_url": "(optional)",
    "type": "json",
    "timestamp": 1578971024
}
```

*_index_0*: NDJSON format (One json object per line)

```json
{
  "_id": "unique uuid for the file",
  "index": "index uuid4 (same one as descriptor.json!)",
  "mime": "application/x-cbz",
  "size": 14341204,
  "mtime": 1578882996,
  "extension": "cbz",
  "name": "my_book",
  "path": "path/to/books",
  "content": "text contents of the book",
  "title": "Title of the book",
  "tag": ["genre.fiction", "author.someguy", "etc..."],
  "_keyword": [
    {"k": "ISBN", "v": "ABCD34789231"}
  ],
  "_text": [
    {"k": "other", "v": "This will be indexed as text"}
  ]
}
```

You can find the full list of supported fields [here](../src/io/serialize.c#L90)

The `_keyword.*` items will be indexed and searchable as **keyword** fields (only full matches allowed).
The `_text.*` items will be indexed and searchable as **text** fields (fuzzy searching allowed)


*thumbs/*:

LMDB key-value store. Keys are **binary** 128-bit UUID4s (`_id` field)
and values are raw image bytes.

Importing an external `binary` type index is technically possible but
it is currently unsupported and has no guaranties of back/forward compatibility.


## Index
### Index options
 * `--es-url` 
 Elasticsearch url and port. If you are using docker, make sure that both containers are on the
 same network.
 * `--es-index` 
    Elasticsearch index name. DEFAULT=sist2
 * `-p, --print` 
    Print index in JSON format to stdout.
 * `--script-file` 
    Path to user script. See [Scripting](scripting.md).
 * `--async-script` 
    Use `wait_for_completion=false` elasticsearch option while executing user script.
     (See [Elasticsearch documentation](https://www.elastic.co/guide/en/elasticsearch/reference/current/tasks.html))
 * `--batch-size=<int>` 
    Index batch size. Indexing is generally faster with larger batches, but payloads that
    are too large will fail and additional overhead for retrying with smaller sizes may slow
    down the process.
 * `-f, --force-reset` 
    Reset Elasticsearch mappings and settings.
    **(You must use this option the first time you use the index command)**.
    
### Index examples

**Push to elasticsearch**
```bash
sist2 index --force-reset --batch-size 1000 --es-url http://localhost:9200 ./my_index/
sist2 index ./my_index/
```

**Save index in JSON format**
```bash
sist2 index --print ./my_index/ > my_index.ndjson
```

**Inspect contents of an index**
```bash
sist2 index --print ./my_index/ | jq | less
```

## Web

### Web options
 * `--es-url=<str>` Elasticsearch url.
 * `--es-index` 
    Elasticsearch index name. DEFAULT=sist2
 * `--bind=<str>` Listen on this address.
 * `--auth=<str>` Basic auth in user:password format
 * `--tag-auth=<str>` Basic auth in user:password format. Works the same way as the 
    `--auth` argument, but authentication is only applied the `/tag/` endpoint.
 
### Web examples

**Single index**
```bash
sist2 web --auth admin:hunter2 --bind 0.0.0.0:8888 my_index
```

**Multiple indices**
```bash
# Indices will be displayed in this order in the web interface
sist2 web index1 index2 index3 index4
```

### rewrite_url

When the `rewrite_url` field is not empty, the web module ignores the `root`
field and will return a HTTP redirect to `<rewrite_url><path>/<name><extension>`
instead of serving the file from disk. 
Both the `root` and `rewrite_url` fields are safe to manually modify from the 
`descriptor.json` file.

### Link to specific indices

To link to specific indices, you can add a list of comma-separated index name to 
the URL: `?i=<name>,<name>`. By default, indices with `"(nsfw)"` in their name are
not displayed.

## exec-script

The `exec-script` command is used to execute a user script for an index that has already been imported to Elasticsearch with the `index` command. Note that the documents will not be reset to their default state before each execution as the `index` command does: if you make undesired changes to the documents by accident, you will need to run `index` again to revert to the original state.


# Tagging

### Manual tagging

You can modify tags of individual documents directly from the 
 `web` interface. Note that you can setup authentication for this feature
 with the `--tag-auth` option (See [web options](#web-options))

![manual_tag](manual_tag.png)

Tags that are manually added are saved both in the 
 index folder (in `/tags/`) and in Elasticsearch*. When re-`index`ing, 
 they are read from the index and automatically applied.
 
You can safely copy the `/tags/` database to another index.

See [Automatic tagging](#automatic-tagging) for information about tag 
 hierarchies and tag colors.

\* *It can take a few seconds to take effect in new search queries, and the page needs 
    to be reloaded for the tags tab to update*


### Automatic tagging

See [scripting](scripting.md) documentation.

# Sidecar files

When scanning, sist2 will read metadata from `.s2meta` JSON files and overwrite the 
original document's metadata. Sidecar metadata files will also work inside archives.
Sidecar files themselves are not saved in the index.

This feature is useful to leverage third-party applications such as speech-to-text or
OCR to add additional metadata to a file.

**Example**

```
~/Documents/
├── Video.mp4
└── Video.mp4.s2meta
```

The sidecar file must have exactly the same file path and the `.s2meta` suffix.

`Video.mp4.s2meta`:
```json
{
  "content": "This sidecar file will overwrite some metadata fields of Video.mp4",
  "author": "Some author",
  "duration": 12345,
  "bitrate": 67890,
  "some_arbitrary_field": [1,2,3]
}
```

```
sist2 scan ~/Documents -o ./docs.idx
sist2 index ./docs.idx
```

*NOTE*: It is technically possible to overwrite the `tag` value using sidecar files, however,
it is not currently possible to restore both manual tags and sidecar tags without user scripts
while reindexing.
