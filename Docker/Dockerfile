FROM ubuntu:19.10
MAINTAINER simon987 <me@simon987.net>

RUN apt update
RUN apt install -y libglib2.0-0 libcurl4 libmagic1 libharfbuzz-bin libopenjp2-7 libarchive13 liblzma5 libzstd1 liblz4-1 curl

RUN mkdir -p /usr/share/tessdata && \
    cd /usr/share/tessdata/ && \
    curl -o /usr/share/tessdata/hin.traineddata https://raw.githubusercontent.com/tesseract-ocr/tessdata/master/hin.traineddata &&\
    curl -o /usr/share/tessdata/jpn.traineddata https://raw.githubusercontent.com/tesseract-ocr/tessdata/master/jpn.traineddata &&\
    curl -o /usr/share/tessdata/eng.traineddata https://raw.githubusercontent.com/tesseract-ocr/tessdata/master/eng.traineddata &&\
    curl -o /usr/share/tessdata/fra.traineddata https://raw.githubusercontent.com/tesseract-ocr/tessdata/master/fra.traineddata &&\
    curl -o /usr/share/tessdata/rus.traineddata https://raw.githubusercontent.com/tesseract-ocr/tessdata/master/rus.traineddata &&\
    curl -o /usr/share/tessdata/spa.traineddata https://raw.githubusercontent.com/tesseract-ocr/tessdata/master/spa.traineddata && ls -lh

ADD sist2 /root/sist2

ENTRYPOINT ["/root/sist2"]
