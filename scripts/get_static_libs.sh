#!/usr/bin/env bash

THREADS=$(nproc)

cd lib

cd mupdf
make USE_SYSTEM_HARFBUZZ=yes USE_SYSTEM_OPENJPEG=yes HAVE_X11=no HAVE_GLUT=no -j $THREADS
cd ..

mv mupdf/build/release/libmupdf.a .
mv mupdf/build/release/libmupdf-third.a .

# openjp2
cd openjpeg
#cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-O3 -march=native -DNDEBUG"
cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-O3"
make -j $THREADS
cd ..
mv openjpeg/bin/libopenjp2.a .

# harfbuzz
cd harfbuzz
./autogen.sh
./configure --disable-shared --enable-static
make -j $THREADS
cd ..
mv harfbuzz/src/.libs/libharfbuzz.a .

# ffmpeg
cd ffmpeg
./configure --disable-shared --enable-static --disable-ffmpeg --disable-ffplay \
 --disable-ffprobe --disable-doc\
 --disable-manpages --disable-postproc --disable-avfilter \
 --disable-alsa --disable-lzma --disable-xlib --disable-debug\
 --disable-vdpau --disable-vaapi --disable-sdl2 --disable-network
make -j $THREADS
cd ..

mv ffmpeg/libavcodec/libavcodec.a .
mv ffmpeg/libavformat/libavformat.a .
mv ffmpeg/libavutil/libavutil.a .
mv ffmpeg/libswresample/libswresample.a .
mv ffmpeg/libswscale/libswscale.a .

# onion
cd onion
mkdir build 2> /dev/null
cd build
cmake -DONION_USE_SSL=false -DONION_USE_PAM=false -DONION_USE_PNG=false -DONION_USE_JPEG=false \
-DONION_USE_JPEG=false -DONION_USE_XML2=false -DONION_USE_SYSTEMD=false -DONION_USE_SQLITE3=false \
-DONION_USE_REDIS=false -DONION_USE_GC=false -DONION_USE_TESTS=false -DONION_EXAMPLES=false \
-DONION_USE_BINDINGS_CPP=false ..
make -j $THREADS
cd ../..

mv onion/build/src/onion/libonion_static.a .

#bzip2
cd bzip2-1.0.6
make -j $THREADS
cd ..
mv bzip2-1.0.6/libbz2.a .

# magic
cd libmagic
./autogen.sh
./configure --enable-static --disable-shared
make -j $THREADS
cd ..
mv libmagic/src/.libs/libmagic.a .

# tesseract
cd tesseract
mkdir build
cd build
cmake -DSTATIC=on -DBUILD_TRAINING_TOOLS=off ..
make -j $THREADS
cd ../..
mv tesseract/build/libtesseract.a .

# leptonica
cd leptonica
./autogen.sh
./configure --without-zlib --without-jpeg --without-giflib \
  --without-giflib --without-libwebp --without-libwebpmux --without-libopenjpeg \
  --enable-static --disable-shared
make -j $THREADS
cd ..
mv leptonica/src/.libs/liblept.a .
