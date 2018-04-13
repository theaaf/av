#pragma once

#include <unordered_map>

#include "file_storage.hpp"

struct TestFileStorage : FileStorage {
    virtual ~TestFileStorage() {}

    struct File : FileStorage::File {
        virtual ~File() {}

        virtual bool write(const void* data, size_t len) override {
            contents.resize(contents.size() + len);
            std::memcpy(&contents[contents.size() - len], data, len);
            return true;
        }

        virtual bool close() override {
            isClosed = true;
            return true;
        }

        std::vector<uint8_t> contents;
        bool isClosed = false;
    };

    virtual std::shared_ptr<FileStorage::File> createFile(const std::string& path) override {
        auto f = std::make_shared<File>();
        files[path] = f;
        return f;
    }

    std::unordered_map<std::string, std::shared_ptr<File>> files;
};
