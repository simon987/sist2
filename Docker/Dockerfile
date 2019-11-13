FROM ubuntu:19.10
MAINTAINER simon987 <me@simon987.net>

RUN apt update
RUN apt install -y libglib2.0-0 libcurl4 libmagic1 libharfbuzz-bin libopenjp2-7

ADD sist2 /root/sist2

ENTRYPOINT ["/root/sist2"]