![GitHub](https://img.shields.io/github/license/simon987/sist2.svg)
[![CodeFactor](https://www.codefactor.io/repository/github/simon987/sist2/badge?s=05daa325188aac4eae32c786f3d9cf4e0593f822)](https://www.codefactor.io/repository/github/simon987/sist2)

# sist2

sist2 (Simple incremental search tool)

## Features



## Example usage

```bash
sist2 scan [OPTION]... PATH

# Examples
sist2 scan ~/Documents -o ./orig_idx/
sist2 scan --threads 4 --content-size 16384 /mnt/Pictures
sist2 scan -i ./orig_idx/ -o ./updated_idx/ ~/Documents
```

```bash
sist2 index [OPTION]... INDEX

# Examples
sist2 index --force-reset ./my_idx
sist2 index --print ./my_idx > raw_documents.ndjson
```

```bash
sist2 web [OPTION]... INDEX...

# Examples
sist2 web --bind 0.0.0.0 --port 4321 ./my_idx1 ./my_idx2 ./my_idx3
```

## Format support

File type | Library | Content | Thumbnail | Metadata
:---|:---|:---|:---|:---
pdf,xps,cbz,cbr,fb2,epub | MuPDF | yes | yes, `png` | *planned* |
`audio/*` | libav | - | yes, `jpeg` | ID3 tags |
`video/*` | libav | - | yes, `jpeg` | *planned* |
`image/*` | libav | - | yes, `jpeg` | *planned* |
ttf,ttc,cff,woff,fnt,otf | Freetype2 | - | yes, `bmp` | Name & style |
`text/plain` | *(none)* | yes | no | - |
docx, xlsx, pptx |  | *planned* | no | *planned* |




## Installation

1. Download runtime dependencies

    `apt install curl bzip2`
    
1. Download binary
    
    Get [the latest release](https://github.com/simon987/sist2/releases) from GitHub

1. (Optional) Add to search path
    
   `mv sist2 /usr/bin/`

## Build from source

1. Install compile-time dependencies

    *(Debian)*
    ```bash
    apt install git cmake pkg-config libglib2.0-dev\
        libssl-dev uuid-dev libavformat-dev libswscale-dev \
        python3 libmagic-dev libfreetype6-dev libcurl-dev \
        libbz2-dev yasm
    ```
    *(Archlinux)*
    ```bash
    pacman -S git ffmpeg pkg-config cmake openssl curl \
        bzip2 yasm libutil-linux
    ```
2. Build
    ```bash
    git clone --recurse-submodules https://github.com/simon987/sist2
    ./scripts/get_static_libs.sh
    cmake .
    make
    ```