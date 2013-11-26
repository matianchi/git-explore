#ifndef OSMIUM_THREAD_CHECKED_TASK_HPP
#define OSMIUM_THREAD_CHECKED_TASK_HPP

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

#include <chrono>
#include <future>
#include <thread>
#include <utility>

namespace osmium {

    namespace thread {

        template <class T>
        class CheckedTask {

            std::thread m_thread {};
            std::future<void> m_future {};

        public:

            CheckedTask(T&& task) {
                std::packaged_task<void()> pack_task(task);
                m_future = pack_task.get_future();
                m_thread = std::thread(std::move(pack_task));
            }

            ~CheckedTask() {
                close();
            }

            CheckedTask(const CheckedTask&) = delete;
            CheckedTask& operator=(const CheckedTask&) = delete;

            /**
             * Check task for exceptions.
             *
             * If an exception happened in the task, re-throw it in this
             * thread. This will not do anything if there was no exception.
             */
            void check_for_exception() {
                if (m_future.valid() && m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    m_future.get();
                }
            }

            /**
             * Close the task. This will raise in this thread any exception the
             * task generated in the other thread. Calling this function is
             * optional, because the destructor will also call this function.
             * But because it can throw an exception, it is better to call it
             * explicitly.
             */
            void close() {
                // If an exception happened in the task, re-throw
                // it in this thread. This will block if the task
                // isn't finished.
                if (m_future.valid()) {
                    m_future.get();
                }

                // Make sure task is done.
                if (m_thread.joinable()) {
                    m_thread.join();
                }
            }

        }; // class CheckedTask

    } // namespace thread

} // namespace osmium

#endif // OSMIUM_THREAD_CHECKED_TASK_HPP
