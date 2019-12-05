![GitHub](https://img.shields.io/github/license/simon987/sist2.svg)
[![CodeFactor](https://www.codefactor.io/repository/github/simon987/sist2/badge?s=05daa325188aac4eae32c786f3d9cf4e0593f822)](https://www.codefactor.io/repository/github/simon987/sist2)

# sist2

sist2 (Simple incremental search tool)

*Warning: sist2 is in early development*

## Features

* Fast, low memory usage, multi-threaded
* Portable (all its features are packaged in a single executable)
* Extracts text from common file types\*
* Generates thumbnails\*
* Incremental scanning
* Automatic tagging from file attributes via [user scripts](scripting/README.md)


\* See [format support](#format-support)

## Getting Started

1. Have an [Elasticsearch](https://www.elastic.co/downloads/elasticsearch) instance running
1. 
    1. Download the [latest sist2 release](https://github.com/simon987/sist2/releases) *
    1. *(or)* `docker pull simon987/sist2:latest`
   

\* *Windows users*: **sist2** runs under [WSL](https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux)    
\* *Mac users*: See [#1](https://github.com/simon987/sist2/issues/1)


## Example usage

![demo](demo.gif)

See help page `sist2 --help` for more details.

**Scan a directory**
```bash
sist2 scan ~/Documents -o ./orig_idx/
sist2 scan --threads 4 --content-size 16384 /mnt/Pictures
sist2 scan --incremental ./orig_idx/ -o ./updated_idx/ ~/Documents
```

**Push index to Elasticsearch or file**
```bash
sist2 index --force-reset ./my_idx
sist2 index --print ./my_idx > raw_documents.ndjson
```

**Start web interface**
```bash
sist2 web --bind 0.0.0.0 --port 4321 ./my_idx1 ./my_idx2 ./my_idx3
```

### Use sist2 with docker

**scan**
```bash
docker run -it \
    -v /path/to/files/:/files \
    -v $PWD/out/:/out \
    simon987/sist2 scan -t 4 /files -o /out/my_idx1
```
**index**
```bash
docker run -it --network host\
    -v $PWD/out/:/out \
    simon987/sist2 index /out/my_idx1
```

**web**
```bash
docker run --rm --network host -d --name sist2\
    -v $PWD/out/my_idx:/idx \
    -v $PWD/my/files:/files
    simon987/sist2 web --bind 0.0.0.0 /idx
docker stop sist2
```


## Format support

File type | Library | Content | Thumbnail | Metadata
:---|:---|:---|:---|:---
pdf,xps,cbz,fb2,epub | MuPDF | yes | yes, `png` | title |
`audio/*` | ffmpeg | - | yes, `jpeg` | ID3 tags |
`video/*` | ffmpeg | - | yes, `jpeg` | title, comment, artist |
`image/*` | ffmpeg | - | yes, `jpeg` | `EXIF:Artist`, `EXIF:ImageDescription` |
ttf,ttc,cff,woff,fnt,otf | Freetype2 | - | yes, `bmp` | Name & style |
`text/plain` | *(none)* | yes | no | - |
tar, zip, rar, 7z, ar ...  | Libarchive | *planned* | - | no |
docx, xlsx, pptx |  | *planned* | no | *planned* |




## Build from source

You can compile **sist2** by yourself if you don't want to use the pre-compiled
binaries.

1. Install compile-time dependencies

    *(Debian)*
    ```bash
    apt install git cmake pkg-config libglib2.0-dev\
        libssl-dev uuid-dev libavformat-dev libswscale-dev \
        python3 libmagic-dev libfreetype6-dev libcurl-dev \
        libbz2-dev yasm libharfbuzz-dev ragel
   ```
    *(FreeBSD)*
    ```bash
   pkg install cmake gcc yasm gmake bash ffmpeg e2fsprogs-uuid\
        autotools ragel
   ```

2. Build
    ```bash
    git clone --recurse-submodules https://github.com/simon987/sist2
    ./scripts/get_static_libs.sh
    cmake .
    make
    ```
