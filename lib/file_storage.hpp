#pragma once

#include <cerrno>
#include <condition_variable>
#include <cstdio>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include <aws/s3/S3Client.h>
#include <aws/s3/model/CompletedMultipartUpload.h>

#include <simple-web-server/server_http.hpp>

#include "aws.hpp"
#include "logger.hpp"

struct FileStorage {
    // File implementations do not need to be thread-safe. All calls must be synchronized by the
    // caller.
    struct File {
        virtual bool write(const void* data, size_t len) = 0;
        virtual bool close() = 0;
    };

    virtual std::string downloadURL(const std::string& path) = 0;

    // When createFile returns a non-null pointer, callers must call close when they're done with it.
    virtual std::shared_ptr<File> createFile(const std::string& path) = 0;
};

// FileStorageForURI takes a string such as "s3:my-bucket" or "file:my-directory" and returns a
// FileStorage instance for it.
std::shared_ptr<FileStorage> FileStorageForURI(Logger logger, const std::string& uri);

// LocalFileStorage is a FileStorage implementation intended primarily for testing. It writes files
// to a directory on your local disk and runs an HTTP server that exposes them via loopback address.
class LocalFileStorage : public FileStorage, private SimpleWeb::Server<SimpleWeb::HTTP> {
public:
    LocalFileStorage(Logger logger, std::string directory);
    virtual ~LocalFileStorage();

    class File : public FileStorage::File {
    public:
        explicit File(Logger logger, std::FILE* f) : _logger{std::move(logger)}, _f{f} {}
        virtual ~File() {}

        virtual bool write(const void* data, size_t len) override;
        virtual bool close() override;

    private:
        const Logger _logger;
        std::FILE* _f;
    };

    virtual std::string downloadURL(const std::string& path) override;
    virtual std::shared_ptr<FileStorage::File> createFile(const std::string& path) override;

private:
    const Logger _logger;
    const std::string _directory;
    int _serverPort = 0;
    std::thread _serverThread;
};

class S3FileStorage : public FileStorage {
public:
    // If you provide your own s3Client (e.g. for testing), make sure you call InitAWS first.
    explicit S3FileStorage(Logger logger, std::string bucket, std::shared_ptr<Aws::S3::S3Client> s3Client = nullptr)
        : _logger{std::move(logger)}, _bucket{std::move(bucket)}, _s3Client{std::move(s3Client)}
    {
        if (!_s3Client) {
            InitAWS();
            _s3Client = std::make_shared<Aws::S3::S3Client>();
        }
    }

    virtual ~S3FileStorage() {}

    class File : public FileStorage::File {
    public:
        File(Logger logger, std::shared_ptr<Aws::S3::S3Client> s3Client, std::string bucket, std::string key, std::string uploadId)
            : _logger{std::move(logger)}, _s3Client{std::move(s3Client)}, _bucket{std::move(bucket)}, _key{std::move(key)}, _uploadId{std::move(uploadId)} {}

        virtual ~File() {}

        virtual bool write(const void* data, size_t len) override;
        virtual bool close() override;

    private:
        const Logger _logger;
        const std::shared_ptr<Aws::S3::S3Client> _s3Client;
        const std::string _bucket;
        const std::string _key;
        const std::string _uploadId;

        std::vector<uint8_t> _buffer;
        uint64_t _nextPartNumber = 1;
        Aws::S3::Model::CompletedMultipartUpload _completedUpload;

        bool _uploadPart();
    };

    virtual std::string downloadURL(const std::string& path) override;
    virtual std::shared_ptr<FileStorage::File> createFile(const std::string& path) override;

private:
    const Logger _logger;
    const std::string _bucket;
    std::shared_ptr<Aws::S3::S3Client> _s3Client;
};

// AsyncFile creates, writes, and closes a file asynchronously.
class AsyncFile {
public:
    AsyncFile(FileStorage* storage, std::string path);
    ~AsyncFile();

    void write(std::shared_ptr<std::vector<uint8_t>> data);
    void close();

    // Blocks until the file has been fully written and closed.
    void wait() const;

    // Returns true if the file has been fully written and closed.
    bool isComplete() const { return _isComplete; }

    // Returns true if no errors have been encounted.
    bool isHealthy() const { return _isHealthy; }

private:
    std::thread _thread;
    mutable std::mutex _mutex;
    std::condition_variable _cv;
    mutable std::condition_variable _waiterCV;

    std::queue<std::shared_ptr<std::vector<uint8_t>>> _writes;
    bool _isClosed = false;
    std::atomic<bool> _isComplete{false};
    std::atomic<bool> _isHealthy{true};
};
