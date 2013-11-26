#ifndef OSMIUM_OSM_OSTREAM_HPP
#define OSMIUM_OSM_OSTREAM_HPP

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

#include <cstdint>
#include <iostream>

#include <osmium/osm/bbox.hpp>
#include <osmium/osm/item_type.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/tag.hpp>

namespace osmium {

    inline std::ostream& operator<<(std::ostream& out, const item_type item_type) {
        std::ios_base::fmtflags old_flags = out.flags(std::ios::hex | std::ios::showbase);
        out << static_cast<uint32_t>(item_type);
        out.flags(old_flags);
        return out;
    }

    inline std::ostream& operator<<(std::ostream& out, const Location& location) {
        if (location) {
            out << '(' << location.lon() << ',' << location.lat() << ')';
        } else {
            out << "(undefined,undefined)";
        }
        return out;
    }

    inline std::ostream& operator<<(std::ostream& out, const Tag& tag) {
        out << tag.key() << '=' << tag.value();
        return out;
    }

    inline std::ostream& operator<<(std::ostream& out, const osmium::BBox& bbox) {
        if (bbox) {
            out << '('
                << bbox.bottom_left().lon()
                << ','
                << bbox.bottom_left().lat()
                << ','
                << bbox.top_right().lon()
                << ','
                << bbox.top_right().lat()
                << ')';
        } else {
            out << "(undefined)";
        }
        return out;
    }

} // namespace osmium

#endif // OSMIUM_OSM_OSTREAM_HPP
