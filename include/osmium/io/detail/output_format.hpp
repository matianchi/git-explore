#ifndef OSMIUM_IO_DETAIL_OUTPUT_FORMAT_HPP
#define OSMIUM_IO_DETAIL_OUTPUT_FORMAT_HPP

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

#include <functional>
#include <future>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include <osmium/io/file.hpp>
#include <osmium/io/file_format.hpp>
#include <osmium/io/header.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/thread/queue.hpp>

namespace osmium {

    namespace io {

        namespace detail {

            typedef osmium::thread::Queue<std::future<std::string>> data_queue_type;

            /**
             * Virtual base class for all classes writing OSM files in different
             * formats.
             *
             * Do not use this class or derived classes directly. Use the
             * osmium::io::Writer class instead.
             */
            class OutputFormat {

            protected:

                osmium::io::File m_file;
                data_queue_type& m_output_queue;

            public:

                OutputFormat(const osmium::io::File& file, data_queue_type& output_queue) :
                    m_file(file),
                    m_output_queue(output_queue) {
                }

                OutputFormat(const OutputFormat&) = delete;
                OutputFormat(OutputFormat&&) = delete;

                OutputFormat& operator=(const OutputFormat&) = delete;
                OutputFormat& operator=(OutputFormat&&) = delete;

                virtual ~OutputFormat() {
                }

                virtual void write_header(const osmium::io::Header&) {
                }

                virtual void write_buffer(osmium::memory::Buffer&&) = 0;

                virtual void close() = 0;

            }; // class OutputFormat

            /**
             * This factory class is used to create objects that write OSM data
             * into a specified output format.
             *
             * Do not use this class directly. Instead use the osmium::io::Writer
             * class.
             */
            class OutputFormatFactory {

            public:

                typedef std::function<osmium::io::detail::OutputFormat*(const osmium::io::File&, data_queue_type&)> create_output_type;

            private:

                typedef std::map<osmium::io::file_format, create_output_type> map_type;

                map_type m_callbacks;

                OutputFormatFactory() :
                    m_callbacks() {
                }

            public:

                static OutputFormatFactory& instance() {
                    static OutputFormatFactory factory;
                    return factory;
                }

                bool register_output_format(osmium::io::file_format format, create_output_type create_function) {
                    if (! m_callbacks.insert(map_type::value_type(format, create_function)).second) {
                        return false;
                    }
                    return true;
                }

                std::unique_ptr<osmium::io::detail::OutputFormat> create_output(const osmium::io::File& file, data_queue_type& output_queue) {
                    file.check();

                    auto it = m_callbacks.find(file.format());
                    if (it != m_callbacks.end()) {
                        return std::unique_ptr<osmium::io::detail::OutputFormat>((it->second)(file, output_queue));
                    }

                    throw std::runtime_error(std::string("Support for output format '") + as_string(file.format()) + "' not compiled into this binary.");
                }

            }; // class OutputFormatFactory

        } // namespace detail

    } // namespace io

} // namespace osmium

#endif // OSMIUM_IO_DETAIL_OUTPUT_FORMAT_HPP
