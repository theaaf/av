#include "memory_context.hpp"
#include <iostream>
#include <sys/resource.h>
#include <functional>

namespace FastMemory {

    size_t MemoryContext::maxMappedMemory{0};
    std::once_flag MemoryContext::dvpConstantsOnceFlag{};

    uint32_t BufferAddrAlignment = 0;
    uint32_t BufferGpuStrideAlignment = 0;
    uint32_t SemaphoreAddrAlignment = 0;
    uint32_t SemaphoreAllocSize = 0;
    uint32_t SemaphorePayloadOffset = 0;
    uint32_t SemaphorePayloadSize = 0;

    std::mutex MemoryContext::memLockMtx;

    bool MemoryContext::initializeMemoryLocking(size_t size) {
        std::lock_guard<std::mutex> l{memLockMtx};

        if (maxMappedMemory >= size) {
            return true;
        }
        struct rlimit limit;
        limit.rlim_cur = limit.rlim_max = size * 80;
        int err = setrlimit(RLIMIT_MEMLOCK, &limit);

        if(err) {
            return false;
        }
        
        maxMappedMemory = size;
        return true;
    }

    MemoryContext::MemoryContext(bool tryDvp, size_t memoryMapSize) {
        if (!initCuda()) {
            throw std::runtime_error("MemoryContext(): failed to initialize CUDA");
        }
        if (!initDvp(tryDvp, memoryMapSize)) {
            std::cerr << "MemoryContext(): DVP failed to initialize, falling back to standard CUDA transfers" << std::endl;;
        }
    }

    MemoryContext::~MemoryContext() {
        if (DVP_STATUS_OK != dvpCloseCUDAContext()) {
            std::cerr << "~MemoryContext(): dvpCloseCUDAContext failed" << std::endl;
        }
        cuCtxSynchronize();
        if (CUDA_SUCCESS != cuCtxDestroy(cuCtx)) {
            std::cerr << "~MemoryContext(): cuCtxDestroy failed" << std::endl;
        }
    }

    bool MemoryContext::initCuda() {
        if (cudaInitialized){
            return true;
        }

        if (CUDA_SUCCESS != cuInit(0)) {
            return false;
        }
        if(CUDA_SUCCESS != cuCtxCreate(&cuCtx, 0, 0)){
            return false;
        }
        if (CUDA_SUCCESS != cuCtxSetCurrent(cuCtx)) {
            return false;
        }

        cudaInitialized = true;
        return true;
    }

    bool MemoryContext::initDvp(bool tryDvp, size_t memoryMapSize) {
        if (dvpInitialized) {
            return true;
        }
        if (!tryDvp) {
            return false;
        }

        if (!initializeMemoryLocking(memoryMapSize)) {
            return false;
        }

        if (DVP_STATUS_OK != dvpInitCUDAContext(DVP_DEVICE_FLAGS_SHARE_APP_CONTEXT)) {
            return false;
        }

        std::call_once(dvpConstantsOnceFlag, []() {
            if (DVP_STATUS_OK != dvpGetRequiredConstantsCUDACtx(    &BufferAddrAlignment, &BufferGpuStrideAlignment,
                                                                    &SemaphoreAddrAlignment, &SemaphoreAllocSize,
                                                                    &SemaphorePayloadOffset, &SemaphorePayloadSize)
            ) {
                throw std::runtime_error("FastTransfer::initialize(): dvpGetRequiredConstantsCUDACtx failed");
            }
        });


        dvpInitialized = true;
        return true;
    }

    SharedBufferPair MemoryContext::allocateBuffer(size_t size) {
        SharedBufferPair bufferPair;
        if (dvpInitialized) {
            initializeMemoryLocking(size);
            bufferPair = std::make_shared<FastBufferPair>(size);
        } else {
            bufferPair = std::make_shared<SlowBufferPair>(size);
        }
        return bufferPair;
    }
}