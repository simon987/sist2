cmake_minimum_required(VERSION 3.7)
set(CMAKE_C_STANDARD 11)

project(sist2 C)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMakeModules")

option(SIST_DEBUG "Build a debug executable" on)

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
        src/web/auth_basic.h src/web/auth_basic.c
        src/index/elastic.c src/index/elastic.h
        src/util.c src/util.h
        src/ctx.h src/types.h src/parsing/font.c src/parsing/font.h
        src/parsing/arc.c src/parsing/arc.h
        src/parsing/doc.c src/parsing/doc.h
        src/log.c src/log.h
        src/parsing/cbr.h src/parsing/cbr.c

        # argparse
        argparse/argparse.h argparse/argparse.c

        # cJSON
        cJSON/cJSON.h cJSON/cJSON.c

        # LMDB
        lmdb/libraries/liblmdb/lmdb.h lmdb/libraries/liblmdb/mdb.c
        lmdb/libraries/liblmdb/midl.h lmdb/libraries/liblmdb/midl.c
        src/cli.c src/cli.h

        # utf8.h
        utf8.h/utf8.h
)

find_package(PkgConfig REQUIRED)
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/local/lib/pkgconfig/")

find_package(Freetype REQUIRED)

pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GOBJECT REQUIRED gobject-2.0)
pkg_check_modules(UUID REQUIRED uuid)

add_definitions(${UUID_CFLAGS_OTHER})
add_definitions(${GLIB_CFLAGS_OTHER})
add_definitions(${GOBJECT_CFLAGS_OTHER})
add_definitions(${FREETYPE_CFLAGS_OTHER})

list(REMOVE_ITEM GLIB_LIBRARIES pcre)
list(REMOVE_ITEM GOBJECT_LIBRARIES pcre)
list(REMOVE_ITEM UUID_LIBRARIES pcre)

target_include_directories(
        sist2 PUBLIC
        ${GOBJECT_INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/lib/ffmpeg/
        ${FREETYPE_INCLUDE_DIRS}
        ${UUID_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/
        ${PROJECT_SOURCE_DIR}/lmdb/libraries/liblmdb/
        ${PROJECT_SOURCE_DIR}/lib/onion/src/
        ${PROJECT_SOURCE_DIR}/lib/mupdf/include/
        ${PROJECT_SOURCE_DIR}/include/
        /usr/include/libxml2/
        ${PROJECT_SOURCE_DIR}/lib/tesseract/include/
)
target_link_directories(
        sist2 PUBLIC
        ${UUID_LIBRARY_DIRS}
)

target_compile_options(
        sist2
        PRIVATE
        -fPIC
)

if (SIST_DEBUG)
    target_compile_options(
            sist2
            PRIVATE
            -g
            -fstack-protector
            -fno-omit-frame-pointer
            -fsanitize=address
    )
    target_link_options(
            sist2
            PRIVATE
            -fsanitize=address
    )
    set_target_properties(
            sist2
            PROPERTIES
            OUTPUT_NAME sist2_debug
    )
else ()
    target_compile_options(
            sist2
            PRIVATE
            -Ofast
            -fno-stack-protector
            -fomit-frame-pointer
    )
endif ()

TARGET_LINK_LIBRARIES(
        sist2

        ${GLIB_LIBRARIES}
        ${GOBJECT_LIBRARIES}
        ${UUID_LIBRARIES}

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

        m
        bz2
#        ${PROJECT_SOURCE_DIR}/lib/libmagic.a
        magic
        ${PROJECT_SOURCE_DIR}/lib/libharfbuzz.a
        ${PROJECT_SOURCE_DIR}/lib/libopenjp2.a
        freetype
        archive

        xml2

        ${PROJECT_SOURCE_DIR}/lib/libtesseract.a
        ${PROJECT_SOURCE_DIR}/lib/liblept.a
        ${PROJECT_SOURCE_DIR}/lib/libtiff.a
        ${PROJECT_SOURCE_DIR}/lib/libpng16.a
        stdc++

        # curl
        ${PROJECT_SOURCE_DIR}/lib/libcurl.a
        ${PROJECT_SOURCE_DIR}/lib/libcrypto.a
        ${PROJECT_SOURCE_DIR}/lib/libssl.a
        dl

        pcre
)

add_custom_target(
        before_sist2
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/before_build.sh
)

add_dependencies(sist2 before_sist2)
