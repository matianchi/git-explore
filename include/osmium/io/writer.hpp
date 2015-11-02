#ifndef OSMIUM_IO_WRITER_HPP
#define OSMIUM_IO_WRITER_HPP

/*

This file is part of Osmium (http://osmcode.org/libosmium).

Copyright 2013-2015 Jochen Topf <jochen@topf.org> and others (see README).

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

#include <cassert>
#include <future>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include <osmium/io/compression.hpp>
#include <osmium/io/detail/output_format.hpp>
#include <osmium/io/detail/queue_util.hpp>
#include <osmium/io/detail/read_write.hpp>
#include <osmium/io/detail/write_thread.hpp>
#include <osmium/io/error.hpp>
#include <osmium/io/file.hpp>
#include <osmium/io/header.hpp>
#include <osmium/io/overwrite.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/thread/util.hpp>

namespace osmium {

    namespace io {

        /**
         * This is the user-facing interface for writing OSM files. Instantiate
         * an object of this class with a file name or osmium::io::File object
         * and optionally the data for the header and then call operator() on
         * it to write Buffers or Items.
         *
         * The writer uses multithreading internally to do the actual encoding
         * of the data into the intended format, possible compress the data and
         * then write it out. But this is intentionally hidden from the user
         * of this class who can use it without knowing those details.
         *
         * If you are done call the close() method to finish up. Only if you
         * don't get an exception from the close() method, you can be sure
         * the data is written correctly (modulo operating system buffering).
         * The destructor of this class will also do the right thing if you
         * forget to call close(), but because the destructor can't throw you
         * will not get informed about any problems.
         *
         * The writer is usually used to write complete blocks of data stored
         * in osmium::memory::Buffers. But you can also write single
         * osmium::memory::Items. In this case the Writer uses an internal
         * Buffer.
         */
        class Writer {

            static constexpr size_t default_buffer_size = 10 * 1024 * 1024;

            osmium::io::File m_file;

            detail::future_string_queue_type m_output_queue;

            std::unique_ptr<osmium::io::detail::OutputFormat> m_output;

            osmium::memory::Buffer m_buffer;

            size_t m_buffer_size;

            std::future<bool> m_write_future;

            osmium::thread::thread_handler m_thread;

            enum class status {
                okay   = 0, // normal writing
                error  = 1, // some error occurred while writing
                closed = 2  // close() called successfully
            } m_status;

            // This function will run in a separate thread.
            static void write_thread(detail::future_string_queue_type& output_queue,
                                     std::unique_ptr<osmium::io::Compressor>&& compressor,
                                     std::promise<bool>&& write_promise) {
                detail::WriteThread write_thread{output_queue,
                                                 std::move(compressor),
                                                 std::move(write_promise)};
                write_thread();
            }

            void write(osmium::memory::Buffer&& buffer) {
                if (m_status != status::okay) {
                    throw io_error("Can not write to writer when in status 'closed' or 'error'");
                }
                try {
                    osmium::thread::check_for_exception(m_write_future);
                    if (buffer.committed() > 0) {
                        m_output->write_buffer(std::move(buffer));
                    }
                } catch (...) {
                    detail::add_to_queue(m_output_queue, std::current_exception());
                    m_output->write_end();
                    m_status = status::error;
                    throw;
                }
            }

        public:

            /**
             * The constructor of the Writer object opens a file and writes the
             * header to it.
             *
             * @param file File (contains name and format info) to open.
             * @param header Optional header data. If this is not given sensible
             *               defaults will be used. See the default constructor
             *               of osmium::io::Header for details.
             * @param allow_overwrite Allow overwriting of existing file? Can be
             *               osmium::io::overwrite::allow or osmium::io::overwrite::no
             *               (default).
             *
             * @throws osmium::io_error If there was an error.
             * @throws std::system_error If the file could not be opened.
             */
            explicit Writer(const osmium::io::File& file, const osmium::io::Header& header = osmium::io::Header(), overwrite allow_overwrite = overwrite::no) :
                m_file(file.check()),
                m_output_queue(20, "raw_output"), // XXX
                m_output(osmium::io::detail::OutputFormatFactory::instance().create_output(m_file, m_output_queue)),
                m_buffer(),
                m_buffer_size(default_buffer_size),
                m_write_future(),
                m_thread(),
                m_status(status::okay) {
                assert(!m_file.buffer()); // XXX can't handle pseudo-files

                std::unique_ptr<osmium::io::Compressor> compressor =
                    osmium::io::CompressionFactory::instance().create_compressor(file.compression(), osmium::io::detail::open_for_writing(m_file.filename(), allow_overwrite));

                std::promise<bool> write_promise;
                m_write_future = write_promise.get_future();
                m_thread = osmium::thread::thread_handler{write_thread, std::ref(m_output_queue), std::move(compressor), std::move(write_promise)};

                try {
                    m_output->write_header(header);
                } catch (...) {
                    detail::add_end_of_data_to_queue(m_output_queue);
                    throw;
                }
            }

            explicit Writer(const std::string& filename, const osmium::io::Header& header = osmium::io::Header(), overwrite allow_overwrite = overwrite::no) :
                Writer(osmium::io::File(filename), header, allow_overwrite) {
            }

            explicit Writer(const char* filename, const osmium::io::Header& header = osmium::io::Header(), overwrite allow_overwrite = overwrite::no) :
                Writer(osmium::io::File(filename), header, allow_overwrite) {
            }

            Writer(const Writer&) = delete;
            Writer& operator=(const Writer&) = delete;

            Writer(Writer&&) = default;
            Writer& operator=(Writer&&) = default;

            ~Writer() noexcept {
                try {
                    close();
                } catch (...) {
                    // Ignore any exceptions because destructor must not throw.
                }
            }

            /**
             * Get the currently configured size of the internal buffer.
             */
            size_t buffer_size() const noexcept {
                return m_buffer_size;
            }

            /**
             * Set the size of the internal buffer. This will only take effect
             * if you have not yet written anything or after the next flush().
             */
            void set_buffer_size(size_t size) noexcept {
                m_buffer_size = size;
            }

            /**
             * Flush the internal buffer if it contains any data. This is also
             * called by close(), so usually you don't have to call this.
             *
             * @throws Some form of osmium::io_error when there is a problem.
             */
            void flush() {
                if (m_status == status::okay && m_buffer && m_buffer.committed() > 0) {
                    osmium::memory::Buffer buffer{m_buffer_size,
                                                  osmium::memory::Buffer::auto_grow::no};
                    using std::swap;
                    swap(m_buffer, buffer);

                    write(std::move(buffer));
                }
            }

            /**
             * Write contents of a buffer to the output file.
             *
             * @throws Some form of osmium::io_error when there is a problem.
             */
            void operator()(osmium::memory::Buffer&& buffer) {
                flush();
                write(std::move(buffer));
            }

            /**
             * Add item to the internal buffer for eventual writing to the
             * output file.
             *
             * @throws Some form of osmium::io_error when there is a problem.
             */
            void operator()(const osmium::memory::Item& item) {
                if (!m_buffer) {
                    m_buffer = osmium::memory::Buffer{m_buffer_size,
                                                      osmium::memory::Buffer::auto_grow::no};
                }
                try {
                    m_buffer.push_back(item);
                } catch (osmium::buffer_is_full&) {
                    flush();
                    m_buffer.push_back(item);
                }
            }

            /**
             * Flushes internal buffer and closes output file. If you do not
             * call this, the destructor of Writer will also do the same
             * thing. But because this call might throw an exception, which
             * the destructor will ignore, it is better to call close()
             * explicitly.
             *
             * @throws Some form of osmium::io_error when there is a problem.
             */
            void close() {
                if (m_status == status::okay) {
                    if (m_buffer && m_buffer.committed() > 0) {
                        write(std::move(m_buffer));
                    }
                    m_status = status::closed;
                    m_output->write_end();
                }
                detail::add_end_of_data_to_queue(m_output_queue);
            }

        }; // class Writer

    } // namespace io

} // namespace osmium

#endif // OSMIUM_IO_WRITER_HPP
