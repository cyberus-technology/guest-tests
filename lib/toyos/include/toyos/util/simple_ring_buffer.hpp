/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <stdint.h>

#include <vector>

namespace cbl
{

/// A simple ring buffer supporting a single thread.
///
/// This is an add-only buffer and elements can only be removed by
/// flushing the whole buffer.
/// Adding elements to a full buffer overwrites the oldest elements.
template <class T>
class simple_ring_buffer
{
public:
    /// \brief Create a new buffer with the given number of elements.
    ///
    /// The elements in the buffer are initialized using their default
    /// constructor. Accessing any element which has not been explicitly
    /// added to the buffer will return such a default element.
    ///
    /// \param max_entries_ maximum number of elements in the buffer
    explicit simple_ring_buffer(size_t max_entries_) : max_entries(max_entries_)
    {
        ASSERT(max_entries > 0, "invalid number of entries");
        queue.resize(max_entries);
    }

    /// \brief Add an element to the buffer.
    ///
    /// If the buffer is already full, the oldest element is overwritten.
    ///
    /// \param msg message to add
    void add(const T& msg)
    {
        if (num_elems == max_entries) {
            queue.at(start) = msg;
            start = (start + 1) % max_entries;
        } else {
            size_t pos {(start + num_elems) % max_entries};
            queue.at(pos) = msg;
            num_elems++;
        }
    }

    /// \brief Accesses the buffer at the provided index.
    ///
    /// The element at index 0 is always the oldest in the buffer.
    /// When an element is added to a full buffer, the element at position
    /// 0 is overwritten and accessing element at index 0 returns the
    /// one formerly being at position 1.
    ///
    /// Access wraps if the index is greater than the capacity of the buffer.
    ///
    /// \param i Index to access.
    /// \return Reference to the specified item.
    T& at(size_t i)
    {
        size_t real_idx {(start + i) % max_entries};
        return queue.at(real_idx);
    }

    /// \brief Reset the buffer to its initial state.
    void flush()
    {
        std::fill(queue.begin(), queue.end(), T());
        num_elems = 0;
        start = 0;
    }

private:
    size_t max_entries;

    std::vector<T> queue;

    size_t num_elems {0};
    size_t start {0};
};
} // namespace cbl
