# av [![codebuild](https://codebuild.us-east-1.amazonaws.com/badges?uuid=eyJlbmNyeXB0ZWREYXRhIjoiU0ppc3lyL1hkUDkvaHVuRm54MGdkUU9xUXFuSjVyaktVdnJ6clR2bk5JQUNvd0xVTGN6VFBXK2RnR1REejlMZHAyZGVTTmVPK3BZMG5WTUprMjc4elZBPSIsIml2UGFyYW1ldGVyU3BlYyI6IlQ0aWRlUlhvYTltT0VVY3EiLCJtYXRlcmlhbFNldFNlcmlhbCI6MX0%3D&branch=master)](https://console.aws.amazon.com/codebuild/home?region=us-east-1#/projects)

This repo is home of the servers that directly route and manipulate audio / video.

## Building

Builds are done using [Bazel](https://bazel.build/). For example: `bazel build ingest-server`

You can also perform a Docker build: `docker build . -t av`

## Running

At the moment, the only application here is the ingest server. After building it, you can run it like so:

```
./bazel-bin/ingest-server/ingest-server
```

This will start up an ingest server that accepts RTMP connections and does nothing with the streams. For something more interesting, you can pass some flags:

```
./bazel-bin/ingest-server/ingest-server --archive-storage file:archive --segment-storage file:segments --encoding '{"video":{"bitrate":4000000,"height":720,"width":1280}}'
```

Now, archive files will be written to the ./archive directory, and segments will be written to ./segments.

## Testing

Run the tests with Bazel: `bazel test lib:test`
