cmake_minimum_required(VERSION 3.15)

project(scan C)
set(CMAKE_C_STANDARD 11)

find_package(LibArchive REQUIRED)
find_package(Threads REQUIRED)
find_package(Tesseract CONFIG REQUIRED)
find_package(harfbuzz CONFIG REQUIRED)
find_package(OpenJPEG CONFIG REQUIRED)
find_package(JPEG REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(FFMPEG REQUIRED)


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

        -static
        -static-libgcc
        -static-libstdc++

        -Wl,--whole-archive
        m
        -Wl,--no-whole-archive

        "${CMAKE_SOURCE_DIR}/third-party/ext_mupdf/src/mupdf/build/release/libmupdf.a"
        "${CMAKE_SOURCE_DIR}/third-party/ext_mupdf/src/mupdf/build/release/libmupdf-third.a"

        ${JPEG_LIBRARIES}
        ${LibArchive_LIBRARIES}
        ${Tesseract_LIBRARIES}
        ${LIBXML2_LIBRARIES}
        ${FFMPEG_LIBRARIES}

        ${CMAKE_THREAD_LIBS_INIT}

        # TODO: Looks like I don't need to explicitly link to libuuid?
)

target_include_directories(
        scan
        BEFORE
        PRIVATE
        "${CMAKE_SOURCE_DIR}/third-party/ext_mupdf/src/mupdf/include/"
        ${JPEG_INCLUDE_DIR}
        ${LIBXML2_INCLUDE_DIR}
        ${FFMPEG_INCLUDE_DIR}
)
