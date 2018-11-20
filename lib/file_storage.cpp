#include "file_storage.hpp"

#include <cstdlib>
#include <fstream>
#include <unordered_map>

#include "aws.hpp"

#include <aws/core/utils/stream/PreallocatedStreamBuf.h>
#include <aws/s3/model/CreateMultipartUploadRequest.h>
#include <aws/s3/model/UploadPartRequest.h>
#include <aws/s3/model/CompleteMultipartUploadRequest.h>

std::shared_ptr<FileStorage> FileStorageForURI(Logger logger, const std::string& uri) {
    auto colon = uri.find(':');
    if (colon == std::string::npos) {
        return nullptr;
    }
    auto scheme = uri.substr(0, colon);
    auto authorityPart = colon + 1;
    if (authorityPart + 1 < uri.size() && uri[authorityPart] == '/' && uri[authorityPart+1] == '/') {
        authorityPart += 2;
    }
    if (scheme == "file") {
        return std::make_shared<LocalFileStorage>(logger, uri.substr(authorityPart));
    }
    if (scheme == "s3") {
        return std::make_shared<S3FileStorage>(logger, uri.substr(authorityPart));
    }
    return nullptr;
}

static std::unordered_map<std::string, std::string> _mimeTypes = { // NOLINT (cert-err58-cpp)
    {"ts", "video/MP2T"},
};

LocalFileStorage::LocalFileStorage(Logger logger, std::string directory) : _logger{std::move(logger)}, _directory{std::move(directory)} {
    io_service = std::make_shared<asio::io_service>();
    config.address = "127.0.0.1";
    config.port = 0;
    default_resource["GET"] = [this](std::shared_ptr<Response> response, std::shared_ptr<Request> request) {
        if (request->path.find("..") != std::string::npos) {
            response->write(SimpleWeb::StatusCode::client_error_not_found, "not found");
            return;
        }

        std::ifstream f{_directory + "/" + request->path, std::ios::in | std::ios::binary};
        if (!f.is_open()) {
            response->write(SimpleWeb::StatusCode::client_error_not_found, "not found");
            return;
        }

        SimpleWeb::CaseInsensitiveMultimap headers;

        std::string ext;
        auto period = request->path.rfind('.');
        if (period != std::string::npos) {
            ext = request->path.substr(period + 1);
        }
        if (_mimeTypes.count(ext) > 0) {
            headers.emplace("Content-Type", _mimeTypes[ext]);
        } else {
            headers.emplace("Content-Type", "application/octet-stream");
        }

        response->write(f, headers);
    };
    start();
    _serverPort = acceptor->local_endpoint().port();
    logger.with("directory", _directory, "port", _serverPort).info("local file storage server started");
    _serverThread = std::thread([this] {
        io_service->run();
    });
}

LocalFileStorage::~LocalFileStorage() {
    stop();
    _serverThread.join();
}

bool LocalFileStorage::File::write(const void* data, size_t len) {
    if (std::fwrite(data, len, 1, _f) != 1) {
        _logger.error("unable to write to file");
        return false;
    }
    return true;
}

bool LocalFileStorage::File::close() {
    if (std::fclose(_f)) {
        _logger.error("unable to close file");
        return false;
    }
    return true;
}

std::string LocalFileStorage::downloadURL(const std::string& path) {
    return "http://127.0.0.1:" + std::to_string(_serverPort) + "/" + path;
}

std::shared_ptr<FileStorage::File> LocalFileStorage::createFile(const std::string& path) {
    // TODO: eventually we'll be able to use the c++17 filesystem library
    auto filePath = _directory + "/" + path;
    auto parentDirectory = _directory;
    auto lastSlash = path.rfind('/');
    if (lastSlash != std::string::npos) {
        parentDirectory = _directory + "/" + path.substr(0, lastSlash);
    }
    system(("mkdir -p " + parentDirectory).c_str()); // NOLINT(cert-env33-c)
    auto f = std::fopen(filePath.c_str(), "wb");
    auto logger = _logger.with("directory", _directory, "path", path);
    if (!f) {
        logger.with("errno", errno).error("unable to create file");
        return nullptr;
    }
    return std::make_shared<File>(logger, f);
}

bool S3FileStorage::File::write(const void* data, size_t len) {
    _buffer.resize(_buffer.size() + len);
    std::memcpy(&_buffer[_buffer.size()-len], data, len);
    if (_buffer.size() > 5 * 1024 * 1024) {
        return _uploadPart();
    }
    return true;
}

bool S3FileStorage::File::close() {
    if (!_buffer.empty() && !_uploadPart()) {
        return false;
    }

    Aws::S3::Model::CompleteMultipartUploadRequest request;
    request.SetBucket(_bucket.c_str());
    request.SetKey(_key.c_str());
    request.SetUploadId(_uploadId.c_str());
    request.SetMultipartUpload(_completedUpload);

    auto outcome = _s3Client->CompleteMultipartUpload(request);
    if (!outcome.IsSuccess()) {
        _logger.error("unable to complete multipart upload: {}: {}", outcome.GetError().GetExceptionName(), outcome.GetError().GetMessage());
        return false;
    }

    _logger.info("completed multipart upload");
    return true;
}

bool S3FileStorage::File::_uploadPart() {
    Aws::S3::Model::UploadPartRequest request;
    request.SetBucket(_bucket.c_str());
    request.SetKey(_key.c_str());
    request.SetPartNumber(_nextPartNumber);
    request.SetUploadId(_uploadId.c_str());

    Aws::Utils::Array<uint8_t> array(_buffer.data(), _buffer.size());
    Aws::Utils::Stream::PreallocatedStreamBuf streamBuf(&array, array.GetLength());
    request.SetBody(std::make_shared<Aws::IOStream>(&streamBuf));

    auto outcome = _s3Client->UploadPart(request);
    if (!outcome.IsSuccess()) {
        _logger.error("unable to upload part: {}: {}", outcome.GetError().GetExceptionName(), outcome.GetError().GetMessage());
        return false;
    }

    Aws::S3::Model::CompletedPart completedPart;
    completedPart.SetPartNumber(_nextPartNumber);
    completedPart.SetETag(outcome.GetResult().GetETag());
    _completedUpload.AddParts(completedPart);
    ++_nextPartNumber;
    _buffer.clear();
    return true;
}

std::string S3FileStorage::downloadURL(const std::string& path) {
    return "https://s3.amazonaws.com/" + _bucket + "/" + path;
}

std::shared_ptr<FileStorage::File> S3FileStorage::createFile(const std::string& path) {
    InitAWS();

    Aws::S3::Model::CreateMultipartUploadRequest request;
    request.SetBucket(_bucket.c_str());
    request.SetKey(path.c_str());

    auto outcome = _s3Client->CreateMultipartUpload(request);
    if (!outcome.IsSuccess()) {
        _logger.error("unable to create multipart upload: {}: {}", outcome.GetError().GetExceptionName(), outcome.GetError().GetMessage());
        return nullptr;
    }

    std::string uploadId{outcome.GetResult().GetUploadId()};
    auto logger = _logger.with("upload_id", uploadId);
    logger.with("key", path).info("created new multipart upload");
    return std::make_shared<File>(logger, _s3Client, _bucket, path, uploadId);
}

AsyncFile::AsyncFile(FileStorage* storage, const std::string& path) {
    _thread = std::thread([this, storage, path] {
        auto f = storage->createFile(path);

        std::unique_lock<std::mutex> l{_mutex};
        while (true) {
            while (!_isClosed && _writes.empty()) {
                _cv.wait(l);
            }
            if (_writes.empty()) {
                break;
            }
            auto next = _writes.front();
            _writes.pop();
            l.unlock();

            if (!f->write(next->data(), next->size())) {
                _isHealthy = false;
            }

            l.lock();
        }

        if (!f->close()) {
            _isHealthy = false;
        }
        _isComplete = true;
        _waiterCV.notify_all();
    });
}

AsyncFile::~AsyncFile() {
    _thread.join();
}

void AsyncFile::write(std::shared_ptr<std::vector<uint8_t>> data) {
    {
        std::lock_guard<std::mutex> l{_mutex};
        _writes.emplace(data);
    }
    _cv.notify_one();
}

void AsyncFile::close() {
    {
        std::lock_guard<std::mutex> l{_mutex};
        _isClosed = true;
    }
    _cv.notify_one();
}

void AsyncFile::wait() const {
    std::unique_lock<std::mutex> l{_mutex};
    while (!_isComplete) {
        _waiterCV.wait(l);
    }
}
