# av [![codebuild](https://codebuild.us-east-1.amazonaws.com/badges?uuid=eyJlbmNyeXB0ZWREYXRhIjoiU0ppc3lyL1hkUDkvaHVuRm54MGdkUU9xUXFuSjVyaktVdnJ6clR2bk5JQUNvd0xVTGN6VFBXK2RnR1REejlMZHAyZGVTTmVPK3BZMG5WTUprMjc4elZBPSIsIml2UGFyYW1ldGVyU3BlYyI6IlQ0aWRlUlhvYTltT0VVY3EiLCJtYXRlcmlhbFNldFNlcmlhbCI6MX0%3D&branch=master)](https://console.aws.amazon.com/codebuild/home?region=us-east-1#/projects)

This repo is home of the servers that directly route and manipulate audio / video.

## Building

Builds are done using [Bazel](https://bazel.build/). For example: `bazel build //main:ingest-server`

## Testing

To run the tests, you need to run a local Minio server: `docker-compose up minio`

Then run the tests with Bazel: `bazel test lib:test`
