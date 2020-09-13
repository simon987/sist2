![GitHub](https://img.shields.io/github/license/simon987/sist2.svg)
[![CodeFactor](https://www.codefactor.io/repository/github/simon987/sist2/badge?s=05daa325188aac4eae32c786f3d9cf4e0593f822)](https://www.codefactor.io/repository/github/simon987/sist2)
[![Development snapshots](https://ci.simon987.net/app/rest/builds/buildType(Sist2_Build)/statusIcon)](https://files.simon987.net/artifacts/Sist2/Build/)

**Demo**: [sist2.simon987.net](https://sist2.simon987.net/)

# sist2

sist2 (Simple incremental search tool)

*Warning: sist2 is in early development*

![sist2.png](docs/sist2.png)

## Features

* Fast, low memory usage, multi-threaded
* Mobile-friendly Web interface
* Portable (all its features are packaged in a single executable)
* Extracts text and metadata from common file types \*
* Generates thumbnails \*
* Incremental scanning
* Manual tagging from the UI and automatic tagging based on file attributes via [user scripts](docs/scripting.md)
* Recursive scan inside archive files \*\*
* OCR support with tesseract \*\*\*
* Stats page & disk utilisation visualization


\* See [format support](#format-support)    
\*\* See [Archive files](#archive-files)    
\*\*\* See [OCR](#ocr)    

![stats](docs/stats.png)


## Getting Started

1. Have an Elasticsearch (>= 6.X.X) instance running
    1. Download [from official website](https://www.elastic.co/downloads/elasticsearch)
    1. *(or)* Run using docker:
        ```bash
       docker run -d --name es1 --net sist2_net -p 9200:9200 \
            -e "discovery.type=single-node" elasticsearch:7.5.2
        ```
    1. *(or)* Run using docker-compose:
        ```yaml
          elasticsearch:
            image: docker.elastic.co/elasticsearch/elasticsearch:7.5.2
            environment:
              - discovery.type=single-node
              - "ES_JAVA_OPTS=-Xms1G -Xmx2G"
        ```
1. Download sist2 executable
    1. Download the [latest sist2 release](https://github.com/simon987/sist2/releases) *
    1. *(or)* Download a [development snapshot](https://files.simon987.net/artifacts/Sist2/Build/) *(Not recommended!)*
    1. *(or)* `docker pull simon987/sist2:latest`

1. See [Usage guide](docs/USAGE.md)
   

\* *Windows users*: **sist2** runs under [WSL](https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux)    


## Example usage

See [Usage guide](docs/USAGE.md) for more details

1. Scan a directory: `sist2 scan ~/Documents -o ./docs_idx`
1. Push index to Elasticsearch: `sist2 index ./docs_idx`
1. Start web interface: `sist2 web ./docs_idx`


## Format support

File type | Library | Content | Thumbnail | Metadata
:---|:---|:---|:---|:---
pdf,xps,fb2,epub | MuPDF | text+ocr | yes | title |
cbz,cbr | *(none)* | - | yes | - |
`audio/*` | ffmpeg | - | yes | ID3 tags |
`video/*` | ffmpeg | - | yes | title, comment, artist |
`image/*` | ffmpeg | - | yes | [Common EXIF tags](https://github.com/simon987/sist2/blob/efdde2734eca9b14a54f84568863b7ffd59bdba3/src/parsing/media.c#L190) |
raw, rw2, dng, cr2, crw, dcr, k25, kdc, mrw, pef, xf3, arw, sr2, srf, erf  | LibRaw | - | yes | Common EXIF tags |
ttf,ttc,cff,woff,fnt,otf | Freetype2 | - | yes, `bmp` | Name & style |
`text/plain` | *(none)* | yes | no | - |
html, xml | *(none)* | yes | no | - |
tar, zip, rar, 7z, ar ...  | Libarchive | yes\* | - | no |
docx, xlsx, pptx | *(none)* | yes | if embedded | creator, modified_by, title |
mobi, azw, azw3 | libmobi | yes | no | author, title |

\* *See [Archive files](#archive-files)*
 
### Archive files
**sist2** will scan files stored into archive files (zip, tar, 7z...) as if
they were directly in the file system. Recursive (archives inside archives)
scan is also supported.

**Limitations**:
* Support for parsing media files with formats that require *seek* (e.g. `.gif`, `.mp4` w/ fragmented metadata etc.) 
  is limitted (see `--mem-buffer` option)
* Archive files are scanned sequentially, by a single thread. On systems where
**sist2** is not I/O bound, scans might be faster when larger archives are split
 into smaller parts.
 
 
### OCR

You can enable OCR support for pdf,xps,fb2,epub file types with the
`--ocr <lang>` option. Download the language data files with your
package manager (`apt install tesseract-ocr-eng`) or directly [from Github](https://github.com/tesseract-ocr/tesseract/wiki/Data-Files).

The `simon987/sist2` image comes with common languages 
(hin, jpn, eng, fra, rus, spa) pre-installed.

Examples
```bash
sist2 scan --ocr jpn ~/Books/Manga/
sist2 scan --ocr eng ~/Books/Textbooks/
```


## Build from source

You can compile **sist2** by yourself if you don't want to use the pre-compiled
binaries (GCC 7+ required).

1. Install compile-time dependencies

   ```bash
   vcpkg install lmdb cjson glib libarchive[core,bzip2,libxml2,lz4,lzma,lzo] pthread tesseract libxml2 ffmpeg zstd gtest mongoose libuuid libmagic libraw curl[core,ssl] jbig2dec brotli libmupdf
   ```

2. Build
    ```bash
    git clone --recursive https://github.com/simon987/sist2/
    cmake -DCMAKE_TOOLCHAIN_FILE=<VCPKG_ROOT>/scripts/buildsystems/vcpkg.cmake .
    make
    ```
