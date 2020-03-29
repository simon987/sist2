cmake_minimum_required(VERSION 3.15)

project(scan C)
set(CMAKE_C_STANDARD 11)

add_library(
        scan
        libscan/util.c libscan/util.h
        libscan/scan.h
        libscan/macros.h

        libscan/text/text.c libscan/text/text.h
        libscan/arc/arc.c libscan/arc/arc.h
        libscan/ebook/ebook.c libscan/ebook/ebook.h
        libscan/cbr/cbr.c libscan/cbr/cbr.h
        libscan/ooxml/ooxml.c libscan/ooxml/ooxml.h
        libscan/media/media.c libscan/media/media.h
        libscan/font/font.c libscan/font/font.h

        third-party/utf8.h
)

set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib)
target_link_directories(scan PRIVATE BEFORE /usr/share/vcpkg/installed/x64-linux/lib/)

find_package(LibArchive REQUIRED)
find_package(BZip2 REQUIRED)
find_package(lz4 REQUIRED)

find_package(Threads REQUIRED)
find_package(Tesseract CONFIG REQUIRED)
find_package(harfbuzz CONFIG REQUIRED)
find_package(OpenJPEG CONFIG REQUIRED)
find_package(JPEG REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(FFMPEG REQUIRED)
#find_package(OpenSSL REQUIRED)
find_package(LibLZMA REQUIRED)
find_package(ZLIB REQUIRED)


include(ExternalProject)
find_program(MAKE_EXE NAMES gmake nmake make)
ExternalProject_Add(
        mupdf
        # TODO: use master branch ?
        URL https://mupdf.com/downloads/archive/mupdf-1.16.1-source.tar.xz

        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        TEST_COMMAND ""
        CONFIGURE_COMMAND ""
        INSTALL_COMMAND ""

        PREFIX "third-party/ext_mupdf"
        BINARY_DIR "third-party/ext_mupdf/src/mupdf"

        BUILD_COMMAND CFLAGS=-fPIC HAVE_CURL=no HAVE_GLUT=no ${MAKE_EXE} -j 4 --silent
        && ar d build/release/libmupdf-third.a jutils.o jdinput.o jdmarker.o jdmaster.o
)
SET(MUPDF_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/third-party/ext_mupdf/src/mupdf/build/release/)
SET(MUPDF_INC_DIR ${CMAKE_CURRENT_BINARY_DIR}/third-party/ext_mupdf/src/mupdf/include/)


target_compile_options(
        scan
        PRIVATE
        -Werror
        -g
)

add_dependencies(
        scan
        mupdf
)

SET(CMAKE_C_LINK_EXECUTABLE "g++ <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

string(REGEX REPLACE "-lvdpau" "" FFMPEG_LIBRARIES "${FFMPEG_LIBRARIES}")
string(REGEX REPLACE "-lX11" "" FFMPEG_LIBRARIES "${FFMPEG_LIBRARIES}")

target_link_libraries(
        scan

        ${LibArchive_LIBRARIES}
        ZLIB::ZLIB
        BZip2::BZip2
        lz4::lz4
        zstd
        lzo2
        LibLZMA::LibLZMA

        freetype

        #        OpenSSL::SSL OpenSSL::Crypto

        stdc++

        -Wl,--whole-archive
        m
        -Wl,--no-whole-archive

        "${MUPDF_LIB_DIR}/libmupdf.a"
        "${MUPDF_LIB_DIR}/libmupdf-third.a"

        ${JPEG_LIBRARIES}
        ${Tesseract_LIBRARIES}
        ${LIBXML2_LIBRARIES}
        ${FFMPEG_LIBRARIES}

        ${CMAKE_THREAD_LIBS_INIT}

        uuid
)

target_include_directories(
        scan
        BEFORE
        PUBLIC
        ${MUPDF_INC_DIR}
        ${JPEG_INCLUDE_DIR}
        ${LIBXML2_INCLUDE_DIR}
        ${FFMPEG_INCLUDE_DIR}
)
