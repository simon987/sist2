cmake_minimum_required(VERSION 3.7)

project(sist2)
set(CMAKE_C_STANDARD 11)

option(SIST_DEBUG "Build a debug executable" on)
option(SIST_FAST "Enable more optimisation flags" off)
option(SIST_FAKE_STORE "Disable IO operations of LMDB stores for debugging purposes" 0)

add_compile_definitions(
        "SIST_PLATFORM=${SIST_PLATFORM}"
)

if (SIST_DEBUG)
    add_compile_definitions(
            "SIST_DEBUG=${SIST_DEBUG}"
    )
endif()

add_subdirectory(third-party/libscan)
set(ARGPARSE_SHARED off)
add_subdirectory(third-party/argparse)

add_executable(sist2
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
        src/parsing/sidecar.c src/parsing/sidecar.h
        src/mempool/mempool.c src/mempool/mempool.h

        src/auth0/auth0_c_api.h src/auth0/auth0_c_api.cpp

        # argparse
        third-party/argparse/argparse.h third-party/argparse/argparse.c
        )
set_target_properties(sist2 PROPERTIES LINKER_LANGUAGE C)

target_link_directories(sist2 PRIVATE BEFORE ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib)

find_package(PkgConfig REQUIRED)

pkg_search_module(GLIB REQUIRED glib-2.0)

find_package(lmdb CONFIG REQUIRED)
find_package(cJSON CONFIG REQUIRED)
find_package(unofficial-mongoose CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_library(MAGIC_LIB
        NAMES libmagic.so.1 magic
        PATHS /usr/lib/x86_64-linux-gnu/ /usr/lib/aarch64-linux-gnu/
)


target_include_directories(
        sist2 PUBLIC
        ${CMAKE_SOURCE_DIR}/third-party/onion/src/
        ${CMAKE_SOURCE_DIR}/third-party/utf8.h/
        ${CMAKE_SOURCE_DIR}/third-party/libscan/
        ${CMAKE_SOURCE_DIR}/
        ${GLIB_INCLUDE_DIRS}
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
            -static-libasan
    )
    set_target_properties(
            sist2
            PROPERTIES
            OUTPUT_NAME sist2_debug
    )
elseif (SIST_FAST)
    target_compile_options(
            sist2
            PRIVATE

            -Ofast
            -march=native
            -fno-stack-protector
            -fomit-frame-pointer
            -freciprocal-math
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
        ${GLIB_LDFLAGS}
        unofficial::mongoose::mongoose
        CURL::libcurl

        pthread

        c

        scan

        ${MAGIC_LIB}
)

add_custom_target(
        before_sist2
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/before_build.sh
)

add_dependencies(sist2 before_sist2)
