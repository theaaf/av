FROM ubuntu:18.04

RUN apt-get update
RUN apt-get install -y curl gnupg2

RUN echo 'deb [arch=amd64] http://storage.googleapis.com/bazel-apt stable jdk1.8' | tee /etc/apt/sources.list.d/bazel.list
RUN curl https://bazel.build/bazel-release.pub.gpg | apt-key add -

RUN apt-get update
RUN apt-get install -y bazel git openjdk-8-jdk libcurl4-openssl-dev make pkg-config g++-8
ENV CC=gcc-8
ENV CXX=g++-8

WORKDIR /tmp/av

# go ahead and compile the big dependencies so devs can test other changes reasonably quickly
COPY .bazelrc build.bzl BUILD WORKSPACE ./
COPY third-party third-party
RUN bazel build @openssl//:libssl.a @ffmpeg//:libavformat.a @aws//:aws @clang_linux//:clang_check

COPY lib lib
RUN bazel test //lib:test
RUN bazel test //lib/h26x:test

COPY ingest-server ingest-server
RUN bazel build ingest-server

COPY archiver archiver
RUN bazel build archiver

COPY analysis analysis
RUN bazel run //analysis:clang-tidy

FROM ubuntu:18.04

RUN apt-get update && apt-get install -y \
        libcurl4-openssl-dev libxext6 libgl1-mesa-dev openssh-server \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/av/bin

COPY --from=0 /tmp/av/bazel-bin/archiver/archiver .
RUN /opt/av/bin/archiver --help > /dev/null

COPY --from=0 /tmp/av/bazel-bin/ingest-server/ingest-server .
RUN /opt/av/bin/ingest-server --help > /dev/null

ENV PATH "/opt/av/bin:$PATH"

RUN useradd -m tunnel
RUN mkdir /home/tunnel/.ssh
COPY docker-entry-point.sh .
ENTRYPOINT [ "docker-entry-point.sh" ]