/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*- */

// Copyright (c) 2019 Maxim Egorushkin. MIT License. See the full licence in file LICENSE.

#include "atomic_queue/atomic_queue.h"

#include <thread>
#include <cstdint>
#include <iostream>

int main() {
    int constexpr PRODUCERS = 1;
    int constexpr CONSUMERS = 2;
    unsigned constexpr CAPACITY = 1024;
    unsigned constexpr N = 1000000;

    using Element = uint32_t;
    Element constexpr NIL = static_cast<Element>(-1);

    using Queue = atomic_queue::AtomicQueueB<Element, std::allocator<Element>, NIL>;

    // Create a queue shared between producers and consumers.
    Queue q{CAPACITY};

    // Start consumers.
    uint64_t results[CONSUMERS];
    std::thread consumers[CONSUMERS];
    for(int i = 0; i < CONSUMERS; ++i)
        consumers[i] = std::thread([&q, &r = results[i]]() {
            uint64_t sum = 0;
            while(Element n = q.pop()) // Stop when 0 is received.
                sum += n;
            r = sum;
        });

    // Start producers.
    std::thread producers[PRODUCERS];
    for(int i = 0; i < PRODUCERS; ++i)
        producers[i] = std::thread([&q]() {
            for(Element n = N; n; --n)
                q.push(n);
        });

    // Wait till producers complete and terminate.
    for(auto& t : producers)
        t.join();

    // Stop consumers.
    for(int i = CONSUMERS; i--;)
        q.push(0);
    // Wait till consumers complete and terminate.
    for(auto& t : consumers)
        t.join();

    // Verify the results.
    uint64_t result = 0;
    for(auto& r : results) {
        result += r;
        if(!r)
            std::cerr << "WARNING: consumer " << (&r - results) << " received no messages.\n";
    }
    uint64_t constexpr expected_result = (N + 1) / 2. * N * PRODUCERS;
    if(int64_t result_diff = result - expected_result) {
        std::cerr << "ERROR: unexpected result difference " << result_diff << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
