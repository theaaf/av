# av

This repo is home of the servers that directly route and manipulate audio / video.

## Building

Builds are done using [Bazel](https://bazel.build/). For example: `bazel build //main:ingest-server`

## Testing

To run the tests, you need to run a local Minio server: `docker-compose up minio`

Then run the tests with Bazel: `bazel test lib:test`
