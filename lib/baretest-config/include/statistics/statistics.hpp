/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "trace.hpp"
#include "x86asm.hpp"

#include "algorithm"
#include "numeric"
#include "vector"

namespace statistics
{

/**
 * Simple Statistics class.
 * You can feed it some data and calculate different statistical values over it
 */
    template <typename T>
    class data
    {
    private:
        std::vector<T> data_;

    public:
        data() = default;

        /// reserves a number of entries in the underlying container
        void reserve(size_t num) { data_.reserve(num); }

        /// push a measurement
        void push(const T& v) { data_.push_back(v); }

        /// check whether or not data has been pushed
        bool has_data() const { return data_.size() > 0; }

        /// get the floor of the average of all measurements
        T avg() const
        {
            assert(has_data());

            auto sum = std::accumulate(std::begin(data_), std::end(data_), static_cast<T>(0));
            return sum / data_.size();
        }

        /// get the min value of all measurements
        T min() const
        {
            assert(has_data());

            return *std::min_element(std::begin(data_), std::end(data_));
        }

        /// get the max value of all measurements
        T max() const
        {
            assert(has_data());

            return *std::max_element(std::begin(data_), std::end(data_));
        }
    };

/**
 * Simple cycle counter helper.
 * This function can be used to measure the amount of processor cycles a given
 * function needs to execute. If desired, the measurement can be repeated
 * a given number of times.
 *
 * \param f function to execute
 * \param times number of repetitions
 * \return data set consisting of min/avg/max
 */
    template <typename FN>
    data<uint64_t> measure_cycles(FN f, size_t times = 1, size_t warmup_runs = 10)
    {
        ASSERT(times > 0, "cannot measure zero runs");

        data<uint64_t> benchmark_data;
        benchmark_data.reserve(times);

        for (size_t run {0}; run < warmup_runs; ++run) {
            f();
        }

        for (size_t run {0}; run < times; ++run) {
            auto start {rdtscp()};
            f();
            auto end {rdtscp()};

            benchmark_data.push(end - start);
        }

        return benchmark_data;
    };

/**
 * A simple cycle accumulator.
 * This class can be used for more complex test scenarios where
 * measure_cycles() is not enough. It should be used by calling
 * start() right before starting a benchmark() and stop() afterwards.
 * This sequence can be repeated any number of times.
 */
    class cycle_acc
    {
    public:
        void start() { last_start = rdtscp(); }

        void stop()
        {
            auto time {rdtscp() - last_start};
            res.push(time);
        }

        data<uint64_t> result() const { return res; }

    private:
        uint64_t last_start {0};
        data<uint64_t> res;
    };

} // namespace statistics
