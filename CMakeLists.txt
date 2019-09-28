cmake_minimum_required(VERSION 3.7)
set(CMAKE_C_STANDARD 11)

project(sist2 C)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMakeModules")

add_executable(
        sist2
        src/main.c
        src/sist.h
        src/io/walk.h src/io/walk.c
        src/parsing/media.h src/parsing/media.c
        src/parsing/pdf.h src/parsing/pdf.c
        src/io/store.h src/io/store.c
        src/tpool.h src/tpool.c
        src/parsing/parse.h src/parsing/parse.c
        src/io/serialize.h src/io/serialize.c
        src/parsing/mime.h src/parsing/mime.c src/parsing/mime_generated.c
        src/parsing/text.h src/parsing/text.c
        src/index/web.c src/index/web.h
        src/web/serve.c src/web/serve.h
        src/index/elastic.c src/index/elastic.h
        src/util.c src/util.h
        src/ctx.h src/types.h src/parsing/font.c src/parsing/font.h

        # argparse
        argparse/argparse.h argparse/argparse.c

        # cJSON
        cJSON/cJSON.h cJSON/cJSON.c

        # LMDB
        lmdb/libraries/liblmdb/lmdb.h lmdb/libraries/liblmdb/mdb.c
        lmdb/libraries/liblmdb/midl.h lmdb/libraries/liblmdb/midl.c
)

find_package(PkgConfig REQUIRED)
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/local/lib/pkgconfig/")

find_package(LibMagic REQUIRED)
find_package(FFmpeg REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Freetype REQUIRED)

pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GOBJECT REQUIRED gobject-2.0)
pkg_check_modules(UUID REQUIRED uuid)

include_directories(${LIBMAGIC_INCLUDE_DIRS})
link_directories(${LIBMAGIC_LIBRARY_DIRS})
add_definitions(${LIBMAGIC_CFLAGS_OTHER})

link_directories(${UUID_LIBRARY_DIRS})
include_directories(${UUID_INCLUDE_DIRS})
add_definitions(${UUID_CFLAGS_OTHER})

include_directories(${GLIB_INCLUDE_DIRS})
link_directories(${GLIB_LIBRARY_DIRS})
add_definitions(${GLIB_CFLAGS_OTHER})

include_directories(${GOBJECT_INCLUDE_DIRS})
link_directories(${GOBJECT_LIBRARY_DIRS})
add_definitions(${GOBJECT_CFLAGS_OTHER})

link_directories(${FFMPEG_LIBRARY_DIRS})
include_directories(${FFMPEG_INCLUDE_DIRS})

include_directories(${OPENSSL_INCLUDE_DIR})
link_directories(${OPENSSL_CRYPTO_LIBRARY})

list(REMOVE_ITEM GLIB_LIBRARIES pcre)
list(REMOVE_ITEM GOBJECT_LIBRARIES pcre)
list(REMOVE_ITEM UUID_LIBRARIES pcre)

include_directories(${FREETYPE_INCLUDE_DIRS})
add_definitions(${FREETYPE_CFLAGS_OTHER})

include_directories(
        ${PROJECT_SOURCE_DIR}/
        ${PROJECT_SOURCE_DIR}/lmdb/libraries/liblmdb/
        ${PROJECT_SOURCE_DIR}/lib/onion/src/
        ${PROJECT_SOURCE_DIR}/lib/mupdf/include/
)

target_compile_options(sist2
        PRIVATE
        -Ofast
#        -march=native
        -fno-stack-protector
        -fomit-frame-pointer
        )

TARGET_LINK_LIBRARIES(
        sist2

        ${GOBJECT_LIBRARIES}
        ${UUID_LIBRARIES}
        ${GLIB_LIBRARIES}

        # ffmpeg
        ${PROJECT_SOURCE_DIR}/lib/libavcodec.a
        ${PROJECT_SOURCE_DIR}/lib/libavformat.a
        ${PROJECT_SOURCE_DIR}/lib/libavutil.a
        ${PROJECT_SOURCE_DIR}/lib/libswscale.a
        ${PROJECT_SOURCE_DIR}/lib/libswresample.a

        # mupdf
        ${PROJECT_SOURCE_DIR}/lib/libmupdf.a
        ${PROJECT_SOURCE_DIR}/lib/libmupdf-third.a

        # onion
        ${PROJECT_SOURCE_DIR}/lib/libonion_static.a

        pthread
        curl
        m
        bz2
        magic
)

add_custom_target(
        before_sist2
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/before_build.sh
)
add_dependencies(sist2 before_sist2)
