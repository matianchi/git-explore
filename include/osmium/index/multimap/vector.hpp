#ifndef OSMIUM_INDEX_MULTIMAP_VECTOR_HPP
#define OSMIUM_INDEX_MULTIMAP_VECTOR_HPP

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

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

#include <osmium/index/multimap.hpp>
#include <osmium/detail/mmap_vector_anon.hpp>
#include <osmium/detail/mmap_vector_file.hpp>
#include <osmium/index/detail/element_type.hpp>
#include <osmium/io/detail/read_write.hpp>

namespace osmium {

    namespace index {

        namespace multimap {

            template <typename TKey, typename TValue, template<typename...> class TVector>
            class VectorBasedSparseMultimap : public Multimap<TKey, TValue> {

            public:

                typedef typename osmium::index::detail::element_type<TKey, TValue> element_type;
                typedef TVector<element_type> vector_type;
                typedef typename vector_type::iterator iterator;
                typedef typename vector_type::const_iterator const_iterator;

            private:

                vector_type m_vector;

                static bool is_removed(element_type& element) {
                    return element.value == 0;
                }

            public:

                void set(const TKey key, const TValue value) override final {
                    m_vector.push_back(element_type(key, value));
                }

                std::pair<iterator, iterator> get_all(const TKey key) {
                    const element_type element(key);
                    return std::equal_range(m_vector.begin(), m_vector.end(), element);
                }

                size_t size() const override final {
                    return m_vector.size();
                }

                size_t byte_size() const {
                    return m_vector.size() * sizeof(element_type);
                }

                size_t used_memory() const override final {
                    return sizeof(element_type) * size();
                }

                void clear() override final {
                    m_vector.clear();
                    m_vector.shrink_to_fit();
                }

                void sort() override final {
                    std::sort(m_vector.begin(), m_vector.end());
                }

                void remove(const TKey key, const TValue value) {
                    auto r = get_all(key);
                    for (auto it = r.first; it != r.second; ++it) {
                        if (it->second == value) {
                            it->second = 0;
                            return;
                        }
                    }
                }

                void erase_removed() {
                    m_vector.erase(
                        std::remove_if(m_vector.begin(), m_vector.end(), is_removed),
                        m_vector.end()
                    );
                }

                void dump_as_list(int fd) const {
                    osmium::io::detail::reliable_write(fd, reinterpret_cast<const char*>(m_vector.data()), byte_size());
                }

            }; // class VectorBasedSparseMultimap

        } // namespace multimap

    } // namespace index

} // namespace osmium

#endif // OSMIUM_INDEX_MULTIMAP_VECTOR_HPP
