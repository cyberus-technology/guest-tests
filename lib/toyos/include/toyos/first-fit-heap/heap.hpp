/*
 * MIT License

 * Copyright (c) 2016 Thomas Prescher

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "heap_config.hpp"

class memory
{
public:
    virtual ~memory() {}

    virtual size_t base() const = 0;
    virtual size_t size() const = 0;
    virtual size_t end()  const = 0;
};

class fixed_memory : public memory
{
private:
    size_t base_;
    size_t size_;

public:
    fixed_memory(size_t base, size_t size)
        : base_(base)
        , size_(size)
    {
    }

    virtual size_t base() const { return base_; }
    virtual size_t size() const { return size_; };
    virtual size_t end()  const { return base_ + size_; }
};

static constexpr size_t HEAP_MIN_ALIGNMENT = 16;

template<size_t ALIGNMENT = HEAP_MIN_ALIGNMENT>
class first_fit_heap
{
private:
    static constexpr size_t min_alignment() { return HEAP_MIN_ALIGNMENT; }

    struct empty {};
    template <size_t, bool, class T>
    struct align_helper : public T {};

    template <size_t SIZE, class T>
    struct HEAP_PACKED align_helper<SIZE, true, T> : public T {
        char align_chars[SIZE - min_alignment()];
    };

    template <size_t MIN, size_t SIZE>
    using prepend_alignment_if_greater = align_helper<SIZE, (SIZE > MIN), empty>;

    class header_free;
    class header_used;

    class footer {
    public:
        size_t size() const { return s; }

        void size(size_t size) { s = size; }

        header_free* header() {
            return reinterpret_cast<header_free*>(reinterpret_cast<char *>(this) + sizeof(footer) - size() - sizeof(header_used));
        }

    private:
        size_t s;
    };

    class HEAP_PACKED header_used : private prepend_alignment_if_greater<16, ALIGNMENT>
    {
    private:
        enum {
            PREV_FREE_MASK = ~(1ul << (sizeof(size_t) * 8 - 1)),
            THIS_FREE_MASK = ~(1ul << (sizeof(size_t) * 8 - 2)),
            SIZE_MASK      = (~PREV_FREE_MASK) | (~THIS_FREE_MASK),
            CANARY_VALUE   = 0x1337133713371337ul,
        };

        size_t raw {0};
        volatile size_t canary {CANARY_VALUE};

    public:
        header_used(const size_t size_) { size(size_); }

        size_t size() const { return raw & ~SIZE_MASK; }

        void size(size_t s)
        {
            ASSERT_HEAP((s & ~SIZE_MASK) == s);
            raw &= SIZE_MASK;
            raw |= s & ~SIZE_MASK;
        }

        bool prev_free() const { return raw & ~PREV_FREE_MASK; }

        void prev_free(bool val)
        {
            raw &= PREV_FREE_MASK;
            raw |= (~PREV_FREE_MASK) * val;
        }

        bool is_free() const { return raw & ~THIS_FREE_MASK; }

        void is_free(bool val)
        {
            raw &= THIS_FREE_MASK;
            raw |= (~THIS_FREE_MASK) * val;
        }

        bool canary_alive() { return canary == CANARY_VALUE; }

        header_used *following_block(const memory &mem)
        {
            auto *following = reinterpret_cast<header_used *>(reinterpret_cast<char *>(this) + sizeof(header_used) + size());
            if (reinterpret_cast<size_t>(following) >= mem.end()) {
                return nullptr;
            }

            return following;
        }

        header_used *preceding_block(const memory& mem)
        {
            if (not prev_free()) {
                return nullptr;
            }

            footer *prev_footer = reinterpret_cast<footer *>(reinterpret_cast<char *>(this) - sizeof(footer));

            ASSERT_HEAP(reinterpret_cast<size_t>(prev_footer) > mem.base());

            return prev_footer->header();
        }

        void *data_ptr() { return this+1; }
    };

    class HEAP_PACKED header_free : public header_used
    {
    public:
        header_free(const size_t size) : header_used(size)
        {
            update_footer();
            this->is_free(true);
        }

        header_free *next() const { return next_; }
        void         next(header_free *val) { next_ = val; }

        footer *get_footer()
        {
            return reinterpret_cast<footer *>(reinterpret_cast<char *>(this) + sizeof(header_used) + this->size() - sizeof(footer));
        }

        void update_footer()
        {
            get_footer()->size(this->size());
        }

        header_free *next_ {nullptr};
    };

    class free_list_container
    {
    public:
        free_list_container(memory &mem_, header_free *root) : mem(mem_), list(root)
        {
            ASSERT_HEAP(ALIGNMENT != 0);
            ASSERT_HEAP(ALIGNMENT >= min_alignment());
            ASSERT_HEAP((ALIGNMENT & (ALIGNMENT - 1)) == 0);
            ASSERT_HEAP(sizeof(header_used) == ALIGNMENT);
            ASSERT_HEAP(mem.size() != 0);
            ASSERT_HEAP((mem.base() & (ALIGNMENT - 1)) == 0);
            ASSERT_HEAP((mem.base() + mem.size()) > mem.base());
        }

        class iterator
        {
        public:
            iterator(header_free *block_ = nullptr) : block(block_) {}

            iterator &operator++()
            {
                block = block->next();
                ASSERT_HEAP(block ? block->canary_alive() : true);
                return *this;
            }

            header_free *operator*() const { return block; }

            bool operator!=(const iterator &other) const { return block != other.block; }

            bool operator==(const iterator &other) const { return not operator!=(other); }

        private:
            header_free *block;
        };

        iterator begin() const { return iterator(list); }
        iterator end()   const { return iterator(); }

    private:
        iterator position_for(header_free *val)
        {
            for (auto elem : *this) {
                if (not elem->next() or (elem->next() > val)) {
                    return elem < val ? iterator(elem) : iterator();
                }
            }

            return end();
        }

        iterator insert_after(header_free *val, iterator other)
        {
            if (not val) {
                return {};
            }

            ASSERT_HEAP(val > *other);

            if (other == end()) {
                // insert at list head
                val->next(list);
                val->is_free(true);
                val->update_footer();
                list = val;
            } else {
                // insert block into chain
                auto *tmp = (*other)->next();
                val->next(tmp);
                val->is_free(true);
                val->update_footer();

                ASSERT_HEAP(val != *other);
                (*other)->next(val);
            }

            // update meta data of surrounding blocks
            auto       *following = val->following_block(mem);
            const auto *preceding = val->preceding_block(mem);

            if (following) {
                following->prev_free(true);
            }

            if (preceding and preceding->is_free()) {
                val->prev_free(true);
            }

            return {val};
        }

        iterator try_merge_back(iterator it)
        {
            auto *following = (*it)->following_block(mem);

            if (following and following->is_free()) {
                auto *following_free = static_cast<header_free*>(following);
                (*it)->next(following_free->next());
                (*it)->size((*it)->size() + following_free->size() + sizeof(header_used));
                (*it)->update_footer();
            }

            return it;
        }

        iterator try_merge_front(iterator it)
        {
            if (not (*it)->prev_free()) {
                return it;
            }

            auto *preceding = static_cast<header_free *>((*it)->preceding_block(mem));
            if (not preceding) {
                return it;
            }

            ASSERT_HEAP(preceding->is_free());
            return try_merge_back({preceding});
        }

        static constexpr size_t min_block_size() { return sizeof(header_free) - sizeof(header_used) + sizeof(footer); }

        size_t align(size_t size) const
        {
            size_t real_size {(size + ALIGNMENT - 1) & ~(ALIGNMENT - 1)};
            return HEAP_MAX(min_block_size(), real_size);
        }

        bool fits(header_free &block, size_t size) const
        {
            ASSERT_HEAP(size >= min_block_size());
            return block.size() >= size;
        }

        iterator first_free(size_t size, iterator &before) const
        {
            iterator before_ = end();

            for (auto elem : *this) {
                if (fits(*elem, size)) {
                    before = before_;
                    return {elem};
                }
                before_ = iterator(elem);
            }

            return {};
        }

    public:
        iterator insert(header_free *val)
        {
            auto pos  = position_for(val);
            auto elem = insert_after(val, pos);
            return try_merge_front(try_merge_back(elem));
        }


        header_used *alloc(size_t size)
        {
            size = HEAP_MAX(size, ALIGNMENT);
            size = align(size);

            iterator prev;
            auto it = first_free(size, prev);

            if (it == end()) {
                return nullptr;
            }

            auto  &block          = **it;
            size_t size_remaining = block.size() - size;

            if (size_remaining < (sizeof(header_free) + sizeof(footer))) {
                // remaining size cannot hold another block, use entire space
                size += size_remaining;
            } else {
                // split block into two
                block.size(size);
                block.update_footer();
                auto *new_block = new (block.following_block(mem)) header_free(size_remaining - sizeof(header_used));
                new_block->next(block.next());
                new_block->prev_free(true);
                block.next(new_block);
            }

            if (*prev) {
                (*prev)->next(block.next());
            } else {
                list = block.next();
            }

            auto *following = block.following_block(mem);
            if (following) {
                following->prev_free(false);
            }

            block.is_free(false);
            return &block;
        }

        bool ptr_in_range(void *p)
        {
            return reinterpret_cast<size_t>(p) >= mem.base() and reinterpret_cast<size_t>(p) < mem.end();
        }

    private:
        memory &mem;
        header_free *list;
    };

private:
    free_list_container free_list;

public:
    first_fit_heap(memory &mem_) : free_list(mem_, new(reinterpret_cast<void *>(mem_.base())) header_free(mem_.size() - sizeof(header_used)))
    {
    }

    void *alloc(size_t size)
    {
        auto *block = free_list.alloc(size);
        return block ? block->data_ptr() : nullptr;
    }

    void free(void *p)
    {
        header_free *header {reinterpret_cast<header_free *>(reinterpret_cast<char *>(p) - sizeof(header_used))};

        if (not p or not free_list.ptr_in_range(header)) {
            return;
        }

        ASSERT_HEAP(header->canary_alive());
        ASSERT_HEAP(not header->is_free());

        free_list.insert(header);
    }

    size_t num_blocks() const
    {
        size_t cnt {0};

        for (auto HEAP_UNUSED elem : free_list) {
            cnt++;
        }

        return cnt;
    }

    size_t free_mem() const
    {
        size_t size {0};

        for (auto  elem : free_list) {
            size += elem->size();
        }

        return size;
    }

    constexpr size_t alignment() const { return ALIGNMENT; }
};
