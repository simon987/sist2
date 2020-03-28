#!/usr/bin/env bash

THREADS=$(nproc)

cd lib

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

# openssl...
git clone --depth 1 -b OpenSSL_1_1_0-stable https://github.com/openssl/openssl
cd openssl
./config --prefix=$(pwd)/../ssl
make depend
make -j $THREADS
make install
cd ..
mv ./openssl/libcrypto.a ./openssl/libssl.a .

# curl
wget -nc https://curl.haxx.se/download/curl-7.68.0.tar.gz
tar -xzf curl-7.68.0.tar.gz
cd curl-7.68.0
./configure --disable-ldap --disable-ldaps --without-librtmp --disable-rtsp --disable-crypto-auth \
  --disable-smtp --without-libidn2 --without-nghttp2 --without-brotli --enable-static --disable-shared \
  --without-libpsl --with-ssl=$(pwd)/../ssl
make -j $THREADS
cd ..
mv curl-7.68.0/lib/.libs/libcurl.a .
