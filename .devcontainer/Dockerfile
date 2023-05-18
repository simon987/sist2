FROM simon987/sist2-build

RUN curl -fsSL https://deb.nodesource.com/setup_16.x | bash
RUN apt update -y; apt install -y nodejs && rm -rf /var/lib/apt/lists/*

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8

