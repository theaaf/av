#pragma once

#include "dvpapi_cuda.h"
#include <sys/mman.h>
#include <vector>
#include <iostream>
#include <memory>

#define DEFAULT_SYS_BUFFER_ALIGNMENT 4096

namespace FastMemory {

    struct MemoryLock {

        MemoryLock(void* ptr, size_t size) : ptr(ptr), size(size) {
            if (mlock(ptr, size)) {
                throw std::runtime_error("MemoryLock(): mlock failed");
            }
        }
        MemoryLock(const MemoryLock&) = delete;
        MemoryLock(const MemoryLock&&) = delete;

        ~MemoryLock() {
            munlock(ptr, size);
        }

    private:
        void* const ptr;
        const size_t size;
    };

    struct CudaBuffer {
        
        explicit CudaBuffer(size_t size) : ptr(CudaBufferPtr(size)) {}
        CudaBuffer(const CudaBuffer&) = delete;
        CudaBuffer(const CudaBuffer&&) = delete;

        ~CudaBuffer() {
            if (CUDA_SUCCESS != cuMemFree(ptr)) {
                std::cerr << "~CudaBuffer(): cuMemFree failed" << std::endl;
            }
        }

        const CUdeviceptr ptr;

    private:
        static CUdeviceptr CudaBufferPtr(size_t size) {
            CUdeviceptr ptr;
            if (CUDA_SUCCESS != cuMemAlloc(&ptr, size)) {
                throw std::runtime_error("CudaBuffer(): cuMemAlloc failed");
            }
            return ptr;
        }
    };

    struct SystemBuffer {

        explicit SystemBuffer(size_t size) : ptr(malloc(size)), size(size) {}
        SystemBuffer(const SystemBuffer&) = delete;
        SystemBuffer(const SystemBuffer&&) = delete;

        virtual ~SystemBuffer() {
            free(ptr);
        }

        void* const ptr;
        const size_t size;
    };

    struct AlignedSystemBuffer : SystemBuffer {

        AlignedSystemBuffer(size_t size, size_t alignment) : SystemBuffer(size + alignment - 1), ptr(align(alignment)), size(size) {}
        AlignedSystemBuffer(const AlignedSystemBuffer&) = delete;
        AlignedSystemBuffer(const AlignedSystemBuffer&&) = delete;
        virtual ~AlignedSystemBuffer() {}

        void* const ptr;
        const size_t size;
    
    private:
        void* align(size_t alignment) {
            uintptr_t temp = (uintptr_t) SystemBuffer::ptr;
            temp += alignment - 1;
            temp &= ~((uint64_t) alignment - 1);
            return (void*) temp;
        }
    };

// stores pointer to memory on host and device
    struct BufferPair {

        virtual ~BufferPair() {}

        virtual void transferHtoD() = 0;
        virtual void transferDtoH() = 0;
        virtual void acquire() {};
        virtual void release() {};

        virtual uint8_t* const getSystemPtr() const = 0;
        virtual const CUdeviceptr getDevicePtr() const = 0;

        constexpr size_t getSize() const {
            return size;
        }

    protected:
        size_t size;
    };

// allocates memory on host and device
// buffers bound together for duration of lifetime
    struct SlowBufferPair : BufferPair {

        explicit SlowBufferPair(size_t size) : cudaBuffer(size), systemBuffer(size) {
            BufferPair::size = size;
        }

        SlowBufferPair(const SlowBufferPair&) = delete;
        SlowBufferPair(const SlowBufferPair&&) = delete;
        virtual ~SlowBufferPair() {}

        virtual uint8_t* const getSystemPtr() const {
            return static_cast<uint8_t*>(systemBuffer.ptr);
        }

        virtual const CUdeviceptr getDevicePtr() const {
            return cudaBuffer.ptr;
        }

        virtual void transferHtoD();
        virtual void transferDtoH();
        
    private:
        CudaBuffer cudaBuffer;
        SystemBuffer systemBuffer;
    };

// Implementation incomplete until DVP
    struct FastBufferPair : BufferPair {

        explicit FastBufferPair(size_t size, size_t alignment = DEFAULT_SYS_BUFFER_ALIGNMENT);

        FastBufferPair(const FastBufferPair&) = delete;
        FastBufferPair(const FastBufferPair&&) = delete;

        virtual ~FastBufferPair() {
            if (DVP_STATUS_OK != dvpFreeBuffer(cudaBufferHandle)) {
                std::cerr << "~FastBufferPair(): dvpFreeBuffer(cudaBuffer) failed";
            }
            if (DVP_STATUS_OK != dvpUnbindFromCUDACtx(systemBufferHandle)) {
                std::cerr << "~FastBufferPair(): dvpUnbindFromCUDACtx(systemBuffer) failed";
            }
            if (DVP_STATUS_OK != dvpDestroyBuffer(systemBufferHandle)) {
                std::cerr << "~FastBufferPair(): dvpDestroyBuffer(systemBuffer) failed";
            }
        }

        virtual uint8_t* const getSystemPtr() const {
            return static_cast<uint8_t*>(alignedSystemBuffer.ptr);
        }

        virtual const CUdeviceptr getDevicePtr() const {
            return cudaBuffer.ptr;
        }

        virtual void transferHtoD();
        virtual void transferDtoH();
        virtual void acquire();
        virtual void release();

    private:

        CudaBuffer cudaBuffer;
        AlignedSystemBuffer alignedSystemBuffer;
        MemoryLock alignedBufferLock;

        DVPBufferHandle cudaBufferHandle;
        DVPBufferHandle systemBufferHandle;

    };

    using SharedBufferPair = std::shared_ptr<BufferPair>;
}