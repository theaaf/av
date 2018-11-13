#include "buffers.hpp"
#include "memory_context.hpp"

#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <thread>

#define MAX_TEST_BUFFER_SIZE 4097

TEST(Buffers, allocating_and_releasing){
    FastMemory::MemoryContext ctx;

    for (int i = 0; i < 25; i++){
        FastMemory::CudaBuffer buffer1(1920*1080*4);
        FastMemory::CudaBuffer buffer2(1920*1080*4);

        FastMemory::SystemBuffer system1(1920*1080*4);
        FastMemory::SystemBuffer system2(1920*1080*4);
        FastMemory::SystemBuffer system3(1920*1080*4);

        FastMemory::SlowBufferPair slow(1920*1080*4);
        FastMemory::FastBufferPair fast(1920*1080*4);

        FastMemory::SlowBufferPair slow2(1920*1080*4);
        FastMemory::FastBufferPair fast2(31920*1080*4);
    }
}

#define THREAD_TEST_COUNT 5
#define THREAD_TEST_LOOPS 5

auto slowCopyTest = []() {
    FastMemory::MemoryContext ctx;

    std::vector<uint8_t> reference(MAX_TEST_BUFFER_SIZE);
    FastMemory::SlowBufferPair slow1(MAX_TEST_BUFFER_SIZE);
    auto sys1 = (uint8_t*) slow1.getSystemPtr();

    for (int j = 0; j < THREAD_TEST_LOOPS; j++) {

        for (int i = 0; i < MAX_TEST_BUFFER_SIZE; i++) {
            sys1[i] = i % 200;
            reference[i] = sys1[i];
        }

        slow1.transferHtoD();
        
        for (int i = 0; i < MAX_TEST_BUFFER_SIZE; i++) {
            sys1[i] = 255;
            ASSERT_NE(sys1[i], reference[i]);
        }

        slow1.transferDtoH();

        for (int i = 0; i < MAX_TEST_BUFFER_SIZE; i++) {
            EXPECT_EQ(sys1[i], reference[i]);
        }
    }
};

TEST(Buffers, slow_buffer_threaded_stress){
    std::vector<std::thread> testThreads;
    for (int i = 0; i < THREAD_TEST_COUNT; i++) {
        testThreads.emplace_back(slowCopyTest);
    }
    for (auto& thread : testThreads) {
        thread.join();
    }
}