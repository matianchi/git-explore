#ifndef OSMIUM_GEOM_WKB_HPP
#define OSMIUM_GEOM_WKB_HPP

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

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include <osmium/osm/location.hpp>
#include <osmium/geom/factory.hpp>

namespace osmium {

    namespace geom {

        struct wkb_factory_traits {
            typedef std::string point_type;
            typedef std::string linestring_type;
            typedef std::string polygon_type;
        };

        class WKBFactory : public GeometryFactory<WKBFactory, wkb_factory_traits> {

            friend class GeometryFactory;

            /// OSM data always uses SRID 4326 (WGS84).
            static constexpr int srid = 4326;

        public:

            WKBFactory(bool ewkb=false) :
                GeometryFactory<WKBFactory, wkb_factory_traits>(),
                m_ewkb(ewkb) {
            }

            void set_hex_mode() {
                m_hex = true;
            }

        private:

            /**
             * Type of WKB geometry.
             * These definitions are from
             * 99-049_OpenGIS_Simple_Features_Specification_For_SQL_Rev_1.1.pdf (for WKB)
             * and http://trac.osgeo.org/postgis/browser/trunk/doc/ZMSgeoms.txt (for EWKB).
             * They are used to encode geometries into the WKB format.
             */
            enum wkbGeometryType : uint32_t {
                wkbPoint               = 1,
                wkbLineString          = 2,
                wkbPolygon             = 3,
                wkbMultiPoint          = 4,
                wkbMultiLineString     = 5,
                wkbMultiPolygon        = 6,
                wkbGeometryCollection  = 7,

                // SRID-presence flag (EWKB)
                wkbSRID                = 0x20000000
            };

            /**
             * Byte order marker in WKB geometry.
             */
            enum wkbByteOrder : uint8_t {
                wkbXDR = 0,         // Big Endian
                wkbNDR = 1          // Little Endian
            };

            std::string m_data {};
            uint32_t m_points {0};
            bool m_ewkb;
            bool m_hex {false};

            template <typename T>
            void str_push(std::string& str, T data) {
                size_t size = str.size();
                str.resize(size + sizeof(T));
                std::memcpy(const_cast<char *>(&str[size]), reinterpret_cast<char*>(&data), sizeof(data));
            }

            std::string convert_to_hex(std::string& str) {
                static const char* lookup_hex = "0123456789abcdef";
                std::string out;

                for (char c : str) {
                    out += lookup_hex[(c >> 4) & 0xf];
                    out += lookup_hex[c & 0xf];
                }

                return out;
            }

            void header(std::string& str, wkbGeometryType type) {
                str_push(str, wkbNDR);
                if (m_ewkb) {
                    str_push(str, type | wkbSRID);
                    str_push(str, srid);
                } else {
                    str_push(str, type);
                }
            }

            point_type make_point(const osmium::Location location) {
                std::string data;
                header(data, wkbPoint);
                str_push(data, location.lon());
                str_push(data, location.lat());

                if (m_hex) {
                    return convert_to_hex(data);
                } else {
                    return data;
                }
            }

            void linestring_start() {
                m_data.clear();
                m_points = 0;
                header(m_data, wkbLineString);
                str_push(m_data, static_cast<uint32_t>(0));
            }

            void linestring_add_location(const osmium::Location location) {
                str_push(m_data, location.lon());
                str_push(m_data, location.lat());
                ++m_points;
            }

            linestring_type linestring_finish() {
                if (m_points < 2) {
                    m_data.clear();
                    throw geometry_error("not enough points for linestring");
                } else {
                    std::string data;
                    std::swap(data, m_data);
                    memcpy(&data[5], &m_points, sizeof(uint32_t));

                    if (m_hex) {
                        return convert_to_hex(data);
                    } else {
                        return data;
                    }
                }
            }

        }; // class WKBFactory

    } // namespace geom

} // namespace osmium

#endif // OSMIUM_GEOM_WKB_HPP
