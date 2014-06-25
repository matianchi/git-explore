#ifndef OSMIUM_BUILDER_OSM_OBJECT_BUILDER_HPP
#define OSMIUM_BUILDER_OSM_OBJECT_BUILDER_HPP

/*

This file is part of Osmium (http://osmcode.org/libosmium).

Copyright 2013,2014 Jochen Topf <jochen@topf.org> and others (see README).

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

#include <cstring>
#include <initializer_list>
#include <new>
#include <utility>

#include <osmium/builder/builder.hpp>
#include <osmium/osm.hpp>
#include <osmium/osm/item_type.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/node_ref.hpp>
#include <osmium/osm/object.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/osm/types.hpp>

namespace osmium {

    namespace memory {
        class Buffer;
    }

    namespace builder {

        class TagListBuilder : public ObjectBuilder<TagList> {

        public:

            explicit TagListBuilder(osmium::memory::Buffer& buffer, Builder* parent=nullptr) :
                ObjectBuilder<TagList>(buffer, parent) {
            }

            ~TagListBuilder() {
                add_padding();
            }

            void add_tag(const char* key, const char* value) {
                add_size(append(key) + append(value));
            }

        }; // class TagListBuilder

        template <class T>
        class NodeRefListBuilder : public ObjectBuilder<T> {

        public:

            explicit NodeRefListBuilder(osmium::memory::Buffer& buffer, Builder* parent=nullptr) :
                ObjectBuilder<T>(buffer, parent) {
            }

            ~NodeRefListBuilder() {
                static_cast<Builder*>(this)->add_padding();
            }

            void add_node_ref(const NodeRef& node_ref) {
                new (static_cast<Builder*>(this)->reserve_space_for<osmium::NodeRef>()) osmium::NodeRef(node_ref);
                static_cast<Builder*>(this)->add_size(sizeof(osmium::NodeRef));
            }

            void add_node_ref(const object_id_type ref, const osmium::Location location=Location()) {
                add_node_ref(NodeRef(ref, location));
            }

        }; // class NodeRefListBuilder

        typedef NodeRefListBuilder<WayNodeList> WayNodeListBuilder;
        typedef NodeRefListBuilder<OuterRing> OuterRingBuilder;
        typedef NodeRefListBuilder<InnerRing> InnerRingBuilder;

        class RelationMemberListBuilder : public ObjectBuilder<RelationMemberList> {

            void add_role(osmium::RelationMember* member, const char* role) {
                member->set_role_size(std::strlen(role) + 1);
                add_size(append(role));
                add_padding(true);
            }

        public:

            explicit RelationMemberListBuilder(osmium::memory::Buffer& buffer, Builder* parent=nullptr) :
                ObjectBuilder<RelationMemberList>(buffer, parent) {
            }

            ~RelationMemberListBuilder() {
                add_padding();
            }

            void add_member(osmium::item_type type, object_id_type ref, const char* role, const osmium::OSMObject* full_member = nullptr) {
                osmium::RelationMember* member = reserve_space_for<osmium::RelationMember>();
                new (member) osmium::RelationMember(ref, type, full_member != nullptr);
                add_size(sizeof(RelationMember));
                add_role(member, role);
                if (full_member) {
                    add_item(full_member);
                }
            }

        }; // class RelationMemberListBuilder

        template <class T>
        class OSMObjectBuilder : public ObjectBuilder<T> {

        public:

            explicit OSMObjectBuilder(osmium::memory::Buffer& buffer, Builder* parent=nullptr) :
                ObjectBuilder<T>(buffer, parent) {
                static_cast<Builder*>(this)->reserve_space_for<string_size_type>();
                static_cast<Builder*>(this)->add_size(sizeof(string_size_type));
            }

            void add_tags(std::initializer_list<std::pair<const char*, const char*>> tags) {
                osmium::builder::TagListBuilder tl_builder(static_cast<Builder*>(this)->buffer(), this);
                for (const auto& p : tags) {
                    tl_builder.add_tag(p.first, p.second);
                }
            }

        }; // class OSMObjectBuilder

        typedef OSMObjectBuilder<osmium::Node> NodeBuilder;
        typedef OSMObjectBuilder<osmium::Relation> RelationBuilder;

        class WayBuilder : public OSMObjectBuilder<osmium::Way> {

        public:

            explicit WayBuilder(osmium::memory::Buffer& buffer, Builder* parent=nullptr) :
                OSMObjectBuilder<osmium::Way>(buffer, parent) {
            }

            void add_node_refs(std::initializer_list<osmium::NodeRef> nodes) {
                osmium::builder::WayNodeListBuilder builder(buffer(), this);
                for (const auto& node_ref : nodes) {
                    builder.add_node_ref(node_ref);
                }
            }

        }; // class WayBuilder

        class AreaBuilder : public OSMObjectBuilder<osmium::Area> {

        public:

            explicit AreaBuilder(osmium::memory::Buffer& buffer, Builder* parent=nullptr) :
                OSMObjectBuilder<osmium::Area>(buffer, parent) {
            }

            /**
             * Initialize area attributes from the attributes of the given object.
             */
            void initialize_from_object(const osmium::OSMObject& source) {
                osmium::Area& area = object();
                area.id(osmium::object_id_to_area_id(source.id(), source.type()));
                area.version(source.version());
                area.changeset(source.changeset());
                area.timestamp(source.timestamp());
                area.visible(source.visible());
                area.uid(source.uid());

                add_user(source.user());
            }

        }; // class AreaBuilder

        typedef ObjectBuilder<osmium::Changeset> ChangesetBuilder;

    } // namespace builder

} // namespace osmium

#endif // OSMIUM_BUILDER_OSM_OBJECT_BUILDER_HPP
