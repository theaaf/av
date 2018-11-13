#include "buffers.hpp"
#include "memory_context.hpp"

namespace FastMemory {

    FastBufferPair::FastBufferPair(size_t size, size_t alignment)
        : cudaBuffer(size), alignedSystemBuffer(size, alignment), alignedBufferLock(alignedSystemBuffer.ptr, alignedSystemBuffer.size) {

        BufferPair::size = size;

        DVPSysmemBufferDesc sysMemBuffersDesc;
        sysMemBuffersDesc.width = 1;
        sysMemBuffersDesc.height = 1;
        sysMemBuffersDesc.stride = 1;
        sysMemBuffersDesc.format = DVP_BUFFER;
        sysMemBuffersDesc.type = DVP_BYTE;
        sysMemBuffersDesc.size = size;
        sysMemBuffersDesc.bufAddr = alignedSystemBuffer.ptr;

        if (DVP_STATUS_OK != dvpCreateBuffer(&sysMemBuffersDesc, &systemBufferHandle)) {
            throw std::runtime_error("FastTransfer::prepareTransfer(): dvpCreateBuffer failed");
        }
        if (DVP_STATUS_OK != dvpBindToCUDACtx(systemBufferHandle)) {
            throw std::runtime_error("FastTransfer::prepareTransfer(): dvpBindToCUDACTX failed");
        }
        if (DVP_STATUS_OK != dvpCreateGPUCUDADevicePtr(cudaBuffer.ptr, &cudaBufferHandle)){
            throw std::runtime_error("FastTransfer::prepareTransfer(): dvpCreateGPUCUDADevicePtr failed");
        }
    }

    void SlowBufferPair::transferHtoD() {
        if (CUDA_SUCCESS != cuMemcpyHtoD(cudaBuffer.ptr, systemBuffer.ptr, size)) {
            throw std::runtime_error("SlowBufferPair::transferHtoD(): cuMemcpyHtoD failed");
        }
    }
    
    void SlowBufferPair::transferDtoH() {
        if (CUDA_SUCCESS != cuMemcpyDtoH(systemBuffer.ptr, cudaBuffer.ptr, size)) {
            throw std::runtime_error("SlowBufferPair::transferDtoH(): cuMemcpyDtoH failed");
        }
    }

    // Stubbed until DVP
    void FastBufferPair::transferHtoD() {
        
    }

    // Stubbed until DVP
    void FastBufferPair::transferDtoH() {
        
    }

    // Stubbed until DVP
    void FastBufferPair::acquire() {
        
    }

    // Stubbed until DVP
    void FastBufferPair::release() {
        
    }
}