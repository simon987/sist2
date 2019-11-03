#!/usr/bin/env bash
cd lib

# mupdf
cd mupdf
HAVE_X11=no HAVE_GLUT=no gmake -j 4
cd ..

mv mupdf/build/release/libmupdf.a .
mv mupdf/build/release/libmupdf-third.a .

# openjp2
cd openjpeg
#cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-O3 -march=native -DNDEBUG"
cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-O3"
gmake -j 4
cd ..
mv openjpeg/bin/libopenjp2.a .

# harfbuzz
cd harfbuzz
./autogen.sh
./configure --disable-shared --enable-static
gmake -j 4
cd ..
mv harfbuzz/src/.libs/libharfbuzz.a .

# ffmpeg
cd ffmpeg
./configure --disable-shared --enable-static --disable-ffmpeg --disable-ffplay \
 --disable-ffprobe --disable-doc\
 --disable-manpages --disable-postproc --disable-avfilter \
 --disable-alsa --disable-lzma --disable-xlib --disable-debug\
 --disable-vdpau --disable-vaapi --disable-sdl2 --disable-network
gmake -j 4
cd ..

mv ffmpeg/libavcodec/libavcodec.a .
mv ffmpeg/libavformat/libavformat.a .
mv ffmpeg/libavutil/libavutil.a .
mv ffmpeg/libswresample/libswresample.a .
mv ffmpeg/libswscale/libswscale.a .

#bzip2
git clone https://github.com/enthought/bzip2-1.0.6
cd bzip2-1.0.6
make -j 4
cd ..
mv bzip2-1.0.6/libbz2.a .

# magic
git clone https://github.com/threatstack/libmagic
cd libmagic
./autogen.sh
./configure --enable-static --disable-shared
make -j 4
cd ..
mv libmagic/src/.libs/libmagic.a .

cd ..
