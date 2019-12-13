#!/usr/bin/env bash
cd lib

cd mupdf
USE_SYSTEM_HARFBUZZ=yes USE_SYSTEM_OPENJPEG=yes HAVE_X11=no HAVE_GLUT=no make -j 4
cd ..

mv mupdf/build/release/libmupdf.a .
mv mupdf/build/release/libmupdf-third.a .

# openjp2
cd openjpeg
#cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-O3 -march=native -DNDEBUG"
cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-O3"
make -j 4
cd ..
mv openjpeg/bin/libopenjp2.a .

# harfbuzz
cd harfbuzz
./autogen.sh
./configure --disable-shared --enable-static
make -j 4
cd ..
mv harfbuzz/src/.libs/libharfbuzz.a .

# ffmpeg
cd ffmpeg
./configure --disable-shared --enable-static --disable-ffmpeg --disable-ffplay \
 --disable-ffprobe --disable-doc\
 --disable-manpages --disable-postproc --disable-avfilter \
 --disable-alsa --disable-lzma --disable-xlib --disable-debug\
 --disable-vdpau --disable-vaapi --disable-sdl2 --disable-network
make -j 4
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
make -j 4
cd ../..

mv onion/build/src/onion/libonion_static.a .

#bzip2
cd bzip2-1.0.6
make -j 4
cd ..
mv bzip2-1.0.6/libbz2.a .

# magic
cd libmagic
./autogen.sh
./configure --enable-static --disable-shared
make -j 4
cd ..
mv libmagic/src/.libs/libmagic.a .

# libarchive
git clone https://github.com/libarchive/libarchive
cd libarchive/build
./autogen.sh
cd ..
./configure --without-nettle --without-expat --without-xml2 --without-openssl
make -j 4
cd ..
mv libarchive/.libs/libarchive.a .

# lz4
git clone https://github.com/lz4/lz4
cd lz4
make -j 4
cd ..
mv lz4/lib/liblz4.a .

# lzma
wget https://newcontinuum.dl.sourceforge.net/project/lzmautils/xz-5.2.3.tar.gz
tar -xzf xz-5.2.3.tar.gz
rm xz-5.2.3.tar.gz
cd xz-5.2.3
./autogen.sh
./configure
make -j 4
cd ..
mv xz-5.2.3/src/liblzma/.libs/liblzma.a .

# zstd
git clone https://github.com/facebook/zstd
cd zstd
make -j 4
cd ..
mv zstd/lib/libzstd.a .

cd ..
