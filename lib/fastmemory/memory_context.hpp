#pragma once

#include "dvpapi_cuda.h"
#include "buffers.hpp"

#include <memory>
#include <mutex>

#define DEFAULT_MEMORY_MAP_SIZE (1920*1080*4*4)

namespace FastMemory {

    extern uint32_t BufferAddrAlignment;
    extern uint32_t BufferGpuStrideAlignment;
    extern uint32_t SemaphoreAddrAlignment;
    extern uint32_t SemaphoreAllocSize;
    extern uint32_t SemaphorePayloadOffset;
    extern uint32_t SemaphorePayloadSize;

// A context automatically creates a CUDA context and sets up DVP, if available
    struct MemoryContext {

        explicit MemoryContext(bool tryDvp = true, size_t memoryMapSize = DEFAULT_MEMORY_MAP_SIZE);
        MemoryContext(const MemoryContext&) = delete;
        MemoryContext(const MemoryContext&&) = delete;
        ~MemoryContext();

        SharedBufferPair allocateBuffer(size_t size);

        bool initDvp(bool tryDvp, size_t memoryMapSize);
        bool initCuda();

    protected:
        static bool initializeMemoryLocking(size_t size);
        static std::mutex memLockMtx;
        static size_t maxMappedMemory;

        static std::once_flag dvpConstantsOnceFlag;

        bool dvpInitialized{false};
        bool cudaInitialized{false};

        CUcontext cuCtx;

    };
}