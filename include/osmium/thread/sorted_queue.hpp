#ifndef OSMIUM_THREAD_SORTED_QUEUE_HPP
#define OSMIUM_THREAD_SORTED_QUEUE_HPP

/*

This file is part of Osmium (http://osmcode.org/osmium).

Copyright 2013 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>

namespace osmium {

    namespace thread {

        /**
         * This implements a sorted queue. It is a bit like a priority
         * queue. We have n worker threads pushing items into the queue
         * and one thread pulling them out again "in order". The order
         * is defined by the monotonically increasing "num" parameter
         * to the push() method. The wait_and_pop() and try_pop() methods
         * will only give out the next numbered item. This way several
         * workers can work in their own time on different pieces of
         * some incoming data, but it all gets serialized properly again
         * after the workers have done their work.
         */
        template <typename T>
        class SortedQueue {

            typedef typename std::deque<T>::size_type size_type;

            mutable std::mutex m_mutex;
            std::deque<T> m_queue;
            std::condition_variable m_data_available;

            size_type m_offset;

            // this method expects that we already have the lock
            bool empty_intern() const {
                return m_queue.front() == T();
            }

        public:

            SortedQueue() :
                m_mutex(),
                m_queue(1),
                m_data_available(),
                m_offset(0) {
            }

            /**
             * Push an item into the queue.
             *
             * @param value The item to push into the queue.
             * @param num Number to describe ordering for the items.
             *            It must increase monotonically.
             */
            void push(T value, size_type num) {
                std::lock_guard<std::mutex> lock(m_mutex);

                num -= m_offset;
                if (m_queue.size() <= num + 1) {
                    m_queue.resize(num + 2);
                }
                m_queue[num] = std::move(value);

                m_data_available.notify_one();
            }

            /**
             * Wait until the next item becomes available and make it
             * available through value.
             */
            void wait_and_pop(T& value) {
                std::unique_lock<std::mutex> lock(m_mutex);

                m_data_available.wait(lock, [this] {
                    return !empty_intern();
                });
                value=std::move(m_queue.front());
                m_queue.pop_front();
                ++m_offset;
            }

            /**
             * Get next item if it is available and return true. Or
             * return false otherwise.
             */
            bool try_pop(T& value) {
                std::lock_guard<std::mutex> lock(m_mutex);

                if (empty_intern()) {
                    return false;
                }
                value=std::move(m_queue.front());
                m_queue.pop_front();
                ++m_offset;
                return true;
            }

            /**
             * The queue is empty. This means try_pop() would fail if called.
             * It does not mean that there is nothing on the queue. Because
             * the queue is sorted, it could mean that the next item in the
             * queue is not available, but other items are.
             */
            bool empty() const {
                std::lock_guard<std::mutex> lock(m_mutex);

                return empty_intern();
            }

            /**
             * Returns the number of items in the queue, regardless of whether
             * they can be accessed. If this is =0 it
             * implies empty()==true, but not the other way around.
             */
            size_t size() const {
                std::lock_guard<std::mutex> lock(m_mutex);
                return m_queue.size();
            }

        }; // class SortedQueue

    } // namespace thread

} // namespace osmium

#endif // OSMIUM_THREAD_SORTED_QUEUE_HPP
