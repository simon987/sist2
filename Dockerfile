FROM simon987/sist2-build as build
MAINTAINER simon987 <me@simon987.net>

WORKDIR /build/
COPY . .
RUN cmake -DSIST_PLATFORM=x64_linux -DSIST_DEBUG=off -DBUILD_TESTS=off -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake .
RUN make -j$(nproc)
RUN strip sist2 || mv sist2_debug sist2

FROM --platform="linux/amd64" ubuntu:21.10

RUN apt update && apt install -y curl libasan5 libmagic1 && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /usr/share/tessdata && \
    cd /usr/share/tessdata/ && \
    curl -o /usr/share/tessdata/hin.traineddata https://raw.githubusercontent.com/tesseract-ocr/tessdata/master/hin.traineddata &&\
    curl -o /usr/share/tessdata/jpn.traineddata https://raw.githubusercontent.com/tesseract-ocr/tessdata/master/jpn.traineddata &&\
    curl -o /usr/share/tessdata/eng.traineddata https://raw.githubusercontent.com/tesseract-ocr/tessdata/master/eng.traineddata &&\
    curl -o /usr/share/tessdata/fra.traineddata https://raw.githubusercontent.com/tesseract-ocr/tessdata/master/fra.traineddata &&\
    curl -o /usr/share/tessdata/rus.traineddata https://raw.githubusercontent.com/tesseract-ocr/tessdata/master/rus.traineddata &&\
    curl -o /usr/share/tessdata/spa.traineddata https://raw.githubusercontent.com/tesseract-ocr/tessdata/master/spa.traineddata

ENTRYPOINT ["/root/sist2"]

ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8

COPY --from=build /build/sist2 /root/sist2
