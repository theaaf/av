FROM ubuntu:16.04

RUN apt-get update
RUN apt-get install -y curl

RUN echo 'deb [arch=amd64] http://storage.googleapis.com/bazel-apt stable jdk1.8' | tee /etc/apt/sources.list.d/bazel.list
RUN curl https://bazel.build/bazel-release.pub.gpg | apt-key add -

RUN apt-get update
RUN apt-get install -y bazel git openjdk-8-jdk libcurl4-openssl-dev make

WORKDIR /tmp/av

# go ahead and compile the big dependencies so devs can test other changes reasonably quickly
COPY .bazelrc build.bzl BUILD WORKSPACE ./
COPY third-party third-party
RUN bazel build @openssl//:libssl.a @ffmpeg//:libavformat.a @aws//:aws

COPY . .

RUN bazel test //lib:test
RUN bazel test //lib/h264:test

RUN bazel build //main:ingest-server

FROM ubuntu:16.04

RUN apt-get update && apt-get install -y \
        libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/av/bin

COPY --from=0 /tmp/av/bazel-bin/main/ingest-server .
RUN /opt/av/bin/ingest-server --help > /dev/null

ENV PATH "/opt/av/bin:$PATH"
