# av

This repo is home of some miscellanous AV utilities and libraries.

## Building

Dev environment requirements: 
- pkg-config

Builds are done using [Bazel](https://bazel.build/). For example: `bazel build ingest-server`

You can also perform a Docker build: `docker build . -t av`

## Testing

Run the tests with Bazel: `bazel test lib:test`
