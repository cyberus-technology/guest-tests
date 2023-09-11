/*
  SPDX-License-Identifier: MIT

  Copyright (c) 2022-2023 Cyberus Technology GmbH

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <assert.h>

#include <array>
#include <memory>
#include <optional>

#include "lock_free_queue_def.h"

namespace cbl
{

   /**
 *  This is the base for a lock free static size queue that is meant
 *  to be used by a single producer and a single consumer.
 *  Cannot be created directly, use static_lock_free_queue,
 *  lock_free_queue_producer or lock_free_queue_consumer.
 *  \tparam T        Type of the stored elements
 *  \tparam MAX_SIZE Maximum length of the queue buffer in number of elements
 */
   template<class T, size_t MAX_SIZE>
   class lock_free_queue_base
   {
   public:
      static_assert(offsetof(lock_less_queue_t, meta_data) == 0);
      static_assert(offsetof(lock_less_queue_t, read_position) == 64);
      static_assert(offsetof(lock_less_queue_t, write_position) == 128);

      /**
     * The storage container for the queue which can check
     * the whether the underlying has a compatible format.
     */
      struct queue_storage : public lock_less_queue_t
      {
         alignas(LFQ_CACHE_LINE_SIZE) std::array<T, MAX_SIZE + 1> elements;

         /** Initializes the queue's meta data
         * \param num_elems Number of elements that are maximally hold in the
         *                  queue. Must be lower equal MAX_SIZE. If larger,
         *                  MAX_SIZE will be used.
         */
         void initialize(uint64_t num_elems = MAX_SIZE)
         {
            read_position = 0;
            write_position = 0;
            meta_data.version = LFQ_API_VERSION;
            meta_data.entry_num = num_elems < MAX_SIZE ? num_elems : MAX_SIZE;
            meta_data.entry_size = sizeof(T);
         }

         /**
         * Verifies that the queue storage has a compatible format
         * \return true if compatible, false otherwise
         */
         bool verify() const
         {
            return meta_data.version == LFQ_API_VERSION and meta_data.entry_size == sizeof(T);
         }
      };

   private:
      size_t next_slot(size_t old) const
      {
         return (old + 1) % (q.meta_data.entry_num + 1);
      }

      uint64_t atomic_load(const uint64_t& atom) const
      {
         uint64_t ret{ 0 };
         __atomic_load(&atom, &ret, __ATOMIC_SEQ_CST);
         return ret;
      }

      void atomic_store(uint64_t& atom, uint64_t val) const
      {
         __atomic_store(&atom, &val, __ATOMIC_SEQ_CST);
      }

      queue_storage& q;

   protected:
      lock_free_queue_base(queue_storage& storage)
         : q(storage) {}

   public:
      /**
     *  Returns true iff the ring buffer contains no items.
     */
      bool empty() const
      {
         return atomic_load(q.read_position) == atomic_load(q.write_position);
      }

      /**
     *  Returns true iff the ring buffer is full.
     */
      bool full() const
      {
         return atomic_load(q.read_position) == next_slot(atomic_load(q.write_position));
      }

      /**
     *  Adds a new element to the ringbuffer.
     *  \return True iff there was enough space to insert the element.
     */
      bool push(const T& elem)
      {
         if (full()) {
            return false;
         }

         size_t old_slot{ atomic_load(q.write_position) };
         q.elements[old_slot] = elem;
         atomic_store(q.write_position, next_slot(old_slot));

         return true;
      }

      /**
     *  Accesses the first element of the buffer.
     *  The caller is responsible to check empty() before calling front()
     */
      T& front()
      {
         assert(not empty());
         return q.elements[atomic_load(q.read_position)];
      }

      /**
     *  Removes the first element of the buffer.
     *  \return True iff there was an element to remove.
     */
      bool pop()
      {
         if (empty()) {
            return false;
         }

         size_t old_slot{ atomic_load(q.read_position) };
         atomic_store(q.read_position, next_slot(old_slot));

         return true;
      }
   };

   /**
 * A lock free queue implementation that manages a fixed-size storage
 * internally. This variant is meant to be used within a single
 * context.
 * \tparam T        \see lock_free_queue_base
 * \tparam MAX_SIZE \see lock_free_queue_base
 */
   template<class T, size_t MAX_SIZE>
   class static_lock_free_queue : public lock_free_queue_base<T, MAX_SIZE>
   {
   public:
      static_lock_free_queue()
         : lock_free_queue_base<T, MAX_SIZE>(storage)
      {
         storage.initialize();
      }

   private:
      using storage_t = typename lock_free_queue_base<T, MAX_SIZE>::queue_storage;
      storage_t storage;
   };

   /**
 * A lock free queue implementation for producers only. This variant is meant
 * to be used cross-context and the storage is managed externally.
 * Only push() and full() are accessible in this variant.
 * \tparam T        \see lock_free_queue_base
 * \tparam MAX_SIZE \see lock_free_queue_base
 */
   template<class T, size_t MAX_SIZE>
   class lock_free_queue_producer : protected lock_free_queue_base<T, MAX_SIZE>
   {
   public:
      using queue_storage = typename lock_free_queue_base<T, MAX_SIZE>::queue_storage;

   private:
      lock_free_queue_producer(queue_storage& storage)
         : lock_free_queue_base<T, MAX_SIZE>(storage) {}

   public:
      /**
     * Creates a new producer queue and initializes the storage container
     * \param storage storage to be used for the queue
     * \param num_entries Maximum entries the queue can hold
     * \return a new producer queue
     */
      static std::unique_ptr<lock_free_queue_producer> create(queue_storage& storage, uint64_t num_elems = MAX_SIZE)
      {
         if (num_elems > MAX_SIZE) {
            return nullptr;
         }

         storage.initialize(num_elems);

         return create_from_existing_storage(storage);
      }

      /**
     * Creates a new producer queue from an existing already initialized
     * storage container. The number of elements the queue can hold at once is
     * defined by the given storage.
     * \param storage storage to be used for the queue
     * \return a new producer queue
     */
      static std::unique_ptr<lock_free_queue_producer> create_from_existing_storage(queue_storage& storage)
      {
         if (not storage.verify()) {
            return nullptr;
         }

         return std::unique_ptr<lock_free_queue_producer>(new lock_free_queue_producer(storage));
      }

      bool empty() const
      {
         return lock_free_queue_base<T, MAX_SIZE>::empty();
      }

      bool push(const T& elem)
      {
         return lock_free_queue_base<T, MAX_SIZE>::push(elem);
      }

      bool full() const
      {
         return lock_free_queue_base<T, MAX_SIZE>::full();
      }
   };

   /**
 * A lock free queue implementation for consumers only. This variant is meant
 * to be used cross-context where the storage is managed externally. The number
 * of entries the queue supports is defined by the queue storage that is
 * defined by the producer.
 * Only pop(), front() and empty() are accessible in this variant.
 * \tparam T        \see lock_free_queue_base
 * \tparam MAX_SIZE \see lock_free_queue_base
 */
   template<class T, size_t MAX_SIZE>
   class lock_free_queue_consumer : protected lock_free_queue_base<T, MAX_SIZE>
   {
   public:
      using queue_storage = typename lock_free_queue_base<T, MAX_SIZE>::queue_storage;

   private:
      lock_free_queue_consumer(queue_storage& storage)
         : lock_free_queue_base<T, MAX_SIZE>(storage) {}

   public:
      /**
     * Creates a new consumer queue and checks whether the storage container has a compatible format
     * \param storage storage to be used for the queue
     * \return a new consumer queue if the storage is compatible, nullptr otherwise
     */
      static std::unique_ptr<lock_free_queue_consumer> create(queue_storage& storage)
      {
         if (storage.verify()) {
            return std::unique_ptr<lock_free_queue_consumer>(new lock_free_queue_consumer(storage));
         }

         return {};
      }

      bool empty() const
      {
         return lock_free_queue_base<T, MAX_SIZE>::empty();
      }

      T& front()
      {
         return lock_free_queue_base<T, MAX_SIZE>::front();
      }

      /**
     *  Removes the first element of the buffer.
     *  \return True iff there was an element to remove.
     */
      bool pop()
      {
         return lock_free_queue_base<T, MAX_SIZE>::pop();
      }
   };

   /**
 *  Helper function to pop the front element of the queue and return its value.
 *  \return nothing if the queue is empty
 *  \return the front element if the queue is non-empty
 */
   template<class T, size_t MAX_SIZE>
   std::optional<T> get_and_pop(cbl::static_lock_free_queue<T, MAX_SIZE>& queue)
   {
      if (queue.empty()) {
         return {};
      }
      T val{ queue.front() };

      [[maybe_unused]] bool success{ queue.pop() };
      assert(success);

      return val;
   }

   /**
 *  The same helper as above, but for the consumer queue.
 *  \return nothing if the queue is empty
 *  \return the front element if the queue is non-empty
 */
   template<class T, size_t MAX_SIZE>
   std::optional<T> get_and_pop(cbl::lock_free_queue_consumer<T, MAX_SIZE>& queue)
   {
      if (queue.empty()) {
         return {};
      }
      T val{ queue.front() };

      [[maybe_unused]] bool success{ queue.pop() };
      assert(success);

      return val;
   }

}  // namespace cbl
