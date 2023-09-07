/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <chrono>
#include <memory>

#include <config.hpp>
#include <toyos/util/cast_helpers.hpp>
#include <toyos/util/interval.hpp>

/**
 * Generic device driver adapter.
 *
 * It provides abstractions for a DMA pool,
 * delay functions, and mutexes.
 */
class device_driver_adapter
{
public:
    virtual ~device_driver_adapter() {}

    /// Generic mutex interface
    class mutex_interface
    {
    public:
        mutex_interface() {}
        virtual ~mutex_interface() {}

        virtual void acquire() = 0; ///< Acquire the mutex
        virtual void release() = 0; ///< Release the mutex

        /// RAII-style mutex guard
        class mutex_guard
        {
        public:
            explicit mutex_guard(mutex_interface& mtx) : mtx_(mtx) { mtx_.acquire(); }
            ~mutex_guard() { mtx_.release(); }

        private:
            mutex_interface& mtx_;
        };

        mutex_guard guard() { return mutex_guard {*this}; }
    };

    /// DMA buffer descriptor
    struct dma_buffer_t
    {
        uintptr_t dma_addr {0};   ///< Address to program into the hardware
        void* lin_addr {nullptr}; ///< Address to access by the driver software
        size_t pages {0};         ///< Number of pages contained in the region

        /**
         * Translate a DMA address to a linear address.
         *
         * \param addr The DMA address to be translated.
         * \return The linear address corresponding to the DMA address, or nullptr, if the address is not contained.
         */
        void* lin_address(uintptr_t addr) const
        {
            if (not dma_range().contains(addr)) {
                return nullptr;
            }

            return num_to_ptr<void*>(uintptr_t(lin_addr) + (addr - dma_addr));
        }

        /**
         * Translate a linear address to a DMA address.
         *
         * \param addr The linear address to be translated.
         * \return The DMA address corresponding to the linear address, or 0, if the address is not contained.
         */
        uintptr_t dma_address(void* addr) const
        {
            if (not lin_range().contains(uintptr_t(addr))) {
                return 0;
            }

            auto off = uintptr_t(addr) - uintptr_t(lin_addr);
            return dma_addr + off;
        }

        /**
         * Translate an object's linear address to DMA space.
         *
         * \param obj The object of which the address should be translated.
         * \return The DMA address corresponding to the object, or 0, if the object is not in the region.
         */
        template <typename T>
        uintptr_t dma_address(const T& obj) const
        {
            return dma_address(uintptr_t(&obj));
        }

        /**
         * Translate a pointer value to a DMA address.
         *
         * This is used for example in ring pointers.
         *
         * \param ptr The unsigned pointer value to translate.
         * \return The DMA address corresponding to the pointer value, or 0, if it is not in the region.
         */
        uintptr_t dma_address(uintptr_t ptr) const { return dma_address(num_to_ptr<void>(ptr)); }

    private:
        cbl::interval dma_range() const { return cbl::interval::from_size(dma_addr, pages * PAGE_SIZE); }
        cbl::interval lin_range() const { return cbl::interval::from_size(uintptr_t(lin_addr), pages * PAGE_SIZE); }
    };

    /**
     * Allocate a DMA buffer.
     *
     * \param count Number of pages requested.
     * \return A DMA buffer descriptor describing the buffer.
     */
    virtual dma_buffer_t allocate_pages(size_t count) = 0;

    /// Hand back buffer to the DMA pool.
    virtual void free_buffer(const dma_buffer_t& buffer) = 0;

    /// Allocate a mutex object, and return a reference to it.
    virtual std::unique_ptr<mutex_interface> get_mutex() = 0;

    /// Sleep for a certain amount of time.
    template <typename Rep, typename Period>
    void delay(std::chrono::duration<Rep, Period> duration)
    {
        udelay(std::chrono::duration_cast<std::chrono::microseconds>(duration));
    }

protected:
    virtual void udelay(std::chrono::microseconds duration) = 0;
};

/**
 * Dummy adapter with no functionality.
 *
 * This adapter can be used to instantiate a device driver
 * for discovery purposes, when none of the adapter functionality
 * is needed.
 */
class dummy_driver_adapter final : public device_driver_adapter
{
public:
    dummy_driver_adapter() {}

    /// Nop-implementation of the mutex interface.
    class dummy_mutex final : public mutex_interface
    {
    public:
        dummy_mutex() : mutex_interface() {}
        virtual void acquire() override {} ///< Nop acquire.
        virtual void release() override {} ///< Nop release.
    };

    /// Dummy allocator.
    virtual dma_buffer_t allocate_pages(size_t) override { return {}; }
    /// Dummy free.
    virtual void free_buffer(const dma_buffer_t&) override {}

    /// Dummy mutex retrieval.
    virtual std::unique_ptr<mutex_interface> get_mutex() override { return std::make_unique<dummy_mutex>(); }

protected:
    /// Dummy delay
    virtual void udelay(std::chrono::microseconds) override {};
};
