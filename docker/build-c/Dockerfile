FROM ubuntu:trusty

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update
RUN apt-get install -y \
    build-essential \
    valgrind \
 && apt-get clean

RUN useradd -m swanson

USER swanson
ENV HOME /home/swanson
WORKDIR /home/swanson
CMD ["/bin/bash"]
