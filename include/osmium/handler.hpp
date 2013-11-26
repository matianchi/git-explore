#ifndef OSMIUM_HANDLER_HPP
#define OSMIUM_HANDLER_HPP

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

namespace osmium {

    class Node;
    class Way;
    class Relation;
    class Changeset;

    namespace handler {

        class Handler {

        public:

            void node(const osmium::Node&) const {
            }

            void way(const osmium::Way&) const {
            }

            void relation(const osmium::Relation&) const {
            }

            void changeset(const osmium::Changeset&) const {
            }

            void init() const {
            }

            void before_nodes() const {
            }

            void after_nodes() const {
            }

            void before_ways() const {
            }

            void after_ways() const {
            }

            void before_relations() const {
            }

            void after_relations() const {
            }

            void before_changesets() const {
            }

            void after_changesets() const {
            }

            void done() const {
            }

        }; // class Handler

    } // namspace handler

} // namespace osmium

#endif // OSMIUM_HANDLER_HPP
