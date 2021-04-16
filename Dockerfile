FROM centos:7

RUN yum update -y \
    && yum install -y tbb tbb-devel cmake boost boost-devel mariadb-devel make gcc-c++

RUN mkdir -p /replay
COPY ./query-playback /replay

RUN mkdir -p /replay/build_dir
RUN cd /replay/build_dir \
    && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .. \
    && make

WORKDIR /replay/build_dir
