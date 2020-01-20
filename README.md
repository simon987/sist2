![GitHub](https://img.shields.io/github/license/simon987/sist2.svg)
[![CodeFactor](https://www.codefactor.io/repository/github/simon987/sist2/badge?s=05daa325188aac4eae32c786f3d9cf4e0593f822)](https://www.codefactor.io/repository/github/simon987/sist2)
[![Development snapshots](https://ci.simon987.net/app/rest/builds/buildType(Sist2_Build)/statusIcon)](https://files.simon987.net/artifacts/Sist2/Build/)

# sist2

sist2 (Simple incremental search tool)

*Warning: sist2 is in early development*

## Features

* Fast, low memory usage, multi-threaded
* Portable (all its features are packaged in a single executable)
* Extracts text from common file types \*
* Generates thumbnails \*
* Incremental scanning
* Automatic tagging from file attributes via [user scripts](scripting/README.md)
* Recursive scan inside archive files \*\*
* OCR support with tesseract \*\*\*


\* See [format support](#format-support)    
\*\* See [Archive files](#archive-files)    
\*\*\* See [OCR](#ocr)    

## Getting Started

1. Have an [Elasticsearch](https://www.elastic.co/downloads/elasticsearch) instance running
1. 
    1. Download the [latest sist2 release](https://github.com/simon987/sist2/releases) *
    1. *(or)* Download an [development snapshot](https://files.simon987.net/artifacts/Sist2/Build/) *(Not recommended!)*
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
pdf,xps,cbz,fb2,epub | MuPDF | text+ocr | yes, `png` | title |
`audio/*` | ffmpeg | - | yes, `jpeg` | ID3 tags |
`video/*` | ffmpeg | - | yes, `jpeg` | title, comment, artist |
`image/*` | ffmpeg | - | yes, `jpeg` | `EXIF:Artist`, `EXIF:ImageDescription` |
ttf,ttc,cff,woff,fnt,otf | Freetype2 | - | yes, `bmp` | Name & style |
`text/plain` | *(none)* | yes | no | - |
tar, zip, rar, 7z, ar ...  | Libarchive | yes\* | - | no |
docx, xlsx, pptx | libOPC | yes | no | no |

\* *See [Archive files](#archive-files)*
 
### Archive files
**sist2** will scan files stored into archive files (zip, tar, 7z...) as if
they were directly in the file system. Recursive (archives inside archives)
scan is also supported.

**Limitations**:
* Parsing media files with formats that require
*seek* (e.g. `.gif`, `.mp4` w/ fragmented metadata etc.) is not supported.
* Archive files are scanned sequentially, by a single thread. On systems where
**sist2** is not I/O bound, scans might be faster when larger archives are split
 into smaller parts.

To check if a media file can be parsed without *seek*, execute `cat file.mp4 | ffprobe -`
 
 
### OCR

You can enable OCR support for pdf,xps,cbz,fb2,epub file types with the
`--ocr <lang>` option. Download the language data files with your
package manager (`apt install tesseract-ocr-eng`) or directly [from Github](https://github.com/tesseract-ocr/tesseract/wiki/Data-Files).

The `simon987/sist2` github image comes with common languages 
(hin, jpn, eng, fra, rus, spa) pre-installed.

Examples
```bash
sist2 scan --ocr jpn ~/Books/Manga/
sist2 scan --ocr eng ~/Books/Textbooks/
```


## Build from source

You can compile **sist2** by yourself if you don't want to use the pre-compiled
binaries.

1. Install compile-time dependencies

    *(Debian)*
    ```bash
    apt install git cmake pkg-config libglib2.0-dev \
        libssl-dev uuid-dev python3 libmagic-dev libfreetype6-dev \
        libcurl-dev libbz2-dev yasm libharfbuzz-dev ragel \
        libarchive-dev libtiff5 libpng16-16 libpango1.0-dev
   ```

2. Build
    ```bash
    git clone --recurse-submodules https://github.com/simon987/sist2
    ./scripts/get_static_libs.sh
    cmake .
    make
    ```
