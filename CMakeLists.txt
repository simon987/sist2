cmake_minimum_required(VERSION 3.7)
set(CMAKE_C_STANDARD 11)

project(sist2 C)

option(SIST_DEBUG "Build a debug executable" on)

set(BUILD_TESTS on)
add_subdirectory(third-party/libscan)
set(ARGPARSE_SHARED off)
add_subdirectory(third-party/argparse)

add_executable(sist2

        # argparse
        third-party/argparse/argparse.h third-party/argparse/argparse.c

        src/main.c
        src/sist.h
        src/io/walk.h src/io/walk.c
        src/io/store.h src/io/store.c
        src/tpool.h src/tpool.c
        src/parsing/parse.h src/parsing/parse.c
        src/io/serialize.h src/io/serialize.c
        src/parsing/mime.h src/parsing/mime.c src/parsing/mime_generated.c
        src/index/web.c src/index/web.h
        src/web/serve.c src/web/serve.h
        src/index/elastic.c src/index/elastic.h
        src/util.c src/util.h
        src/ctx.h src/types.h
        src/log.c src/log.h
        src/cli.c src/cli.h
        src/stats.c src/stats.h src/ctx.c
        src/parsing/sidecar.c src/parsing/sidecar.h)

target_link_directories(sist2 PRIVATE BEFORE ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib)

find_package(lmdb CONFIG REQUIRED)
find_package(cJSON CONFIG REQUIRED)
find_package(unofficial-glib CONFIG REQUIRED)
find_package(unofficial-mongoose CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)

#find_package(OpenSSL REQUIRED)


target_include_directories(
        sist2 PUBLIC
        ${CMAKE_SOURCE_DIR}/third-party/onion/src/
        ${CMAKE_SOURCE_DIR}/third-party/utf8.h/
        ${CMAKE_SOURCE_DIR}/third-party/libscan/
        ${CMAKE_SOURCE_DIR}/
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
            -fno-inline
#            -O2
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

add_dependencies(
        sist2
        scan
        argparse
)

target_link_libraries(
        sist2

        z
        lmdb
        cjson
        argparse
        unofficial::glib::glib
        unofficial::mongoose::mongoose
        CURL::libcurl

        pthread
        magic

        c

        scan
)

add_custom_target(
        before_sist2
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/before_build.sh
)

add_dependencies(sist2 before_sist2)
