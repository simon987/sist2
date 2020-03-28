
```bash
vcpkg install libarchive pthread tesseract libxml2 ffmpeg

cmake -DCMAKE_TOOLCHAIN_FILE=/usr/share/vcpkg/scripts/buildsystems/vcpkg.cmake .
make -j 4
```