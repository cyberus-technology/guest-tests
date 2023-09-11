/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "x86/x86asm.hpp"

#include <atomic>

// A simple spinlock that enforces a fair ordering of threads. This spinlock will not preempt a thread that has to wait,
// but uses busy waiting in the lock-function.
class spinlock : public uncopyable
{
   // The number that is currently allowed to enter the critical section.
   std::atomic<unsigned> current_ticket{ 0u };

   // The number the next thread that is trying to enter the critical section will get.
   std::atomic<unsigned> next_ticket{ 0u };

public:
   // Acquires the spinlock. If the lock is not available, i.e. another thread helds this lock, the calling thread
   // spins (busy-waiting) until it can acquire the lock.
   void lock()
   {
      const unsigned my_ticket{ next_ticket.fetch_add(1) };
      while (my_ticket != current_ticket.load()) {
         // As Hedron and SuperNOVA use only a single time-slice per CPU, we know that a thread on another CPU
         // currently holds the lock. Thus we busy-wait until someone unlocks this lock.
         cpu_pause();
      }
   }

   // Unlock the spinlock, allowing the next waiting thread to acquire it.
   void unlock()
   {
      current_ticket.store(current_ticket + 1);
   }

   // A guard for a spinlock that can handle a given nullptr. If the given pointer is not a nullptr, it locks the given
   // spinlock upon construction and unlocks it upon destruction.
   class optional_guard : public uncopyable
   {
      spinlock* const lock;

   public:
      explicit optional_guard(spinlock* lock_)
         : lock(lock_)
      {
         if (lock) {
            lock->lock();
         }
      }

      ~optional_guard()
      {
         if (lock) {
            lock->unlock();
         }
      }
   };

   // A guard for a spinlock that locks a given spinlock upon construction and unlocks it upon destruction.
   struct guard : public optional_guard
   {
      explicit guard(spinlock& lock_)
         : optional_guard(&lock_) {}
      ~guard() = default;
   };
};
