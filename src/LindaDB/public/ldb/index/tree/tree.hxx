/* LindaDB project
 *
 * Copyright (c) 2023 András Bodor <bodand@pm.me>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of the copyright holder nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Originally created: 2023-10-15.
 *
 * src/LindaDB/public/ldb/index/tree/tree --
 *   Implementation of the main, tree class. Functions as the base parent of the
 *   root of the real tree implementation orchestrated by the tree_node objects.
 */
#ifndef LINDADB_TREE_HXX
#define LINDADB_TREE_HXX

#include <cassert>
#include <cstdlib>
#include <memory>
#include <optional>
#include <ostream>
#include <variant>

#include <ldb/index/tree/payload_dispatcher.hxx>
#include <ldb/index/tree/tree_node.hxx>
#include <ldb/profiler.hxx>

namespace ldb::index::tree {
    template<class K,
             class V,
             std::size_t Clustering = 0>
    struct tree final : private tree_node_handler<tree_node<typename payload_dispatcher<K, V, Clustering>::type>> {
        using payload_type = typename payload_dispatcher<K, V, Clustering>::type;

        [[nodiscard]] std::optional<V>
        search(const K& key) const {
            LDB_PROF_SCOPE_C("Tree_Search", prof::color_search);
            std::optional<V> res;
            node_type* node = _root.get();
            while (node != nullptr) {
                std::visit(search_obj{res, node},
                           node->search(key));
            }
            return res;
        }

        void
        insert(const K& key, const V& value) {
            LDB_PROF_SCOPE_C("Tree_Insert", prof::color_search);
            node_type* node = _root.get();
            do {
                node = node->insert(key, value);
            } while (node != nullptr);
        }

        void
        dump(std::ostream& os) const {
            _root->dump(os);
            os << "\n";
        }

        [[nodiscard]] std::size_t
        height() const noexcept {
            return _root->total_height_inclusive();
        }

    private:
        using node_type = tree_node<payload_type>;

        struct search_obj {
            void
            operator()(node_type* next) const noexcept {
                node = next;
            }

            void
            operator()(const std::optional<V>& val) const noexcept {
                node = nullptr;
                res = val;
            }

            std::optional<V>& res;
            node_type*& node;
        };

        std::unique_ptr<node_type>
        replace_this_as_child(node_type* old, std::unique_ptr<node_type>&& new_) override {
            assert(new_ != nullptr && "root's node cannot be null");
            if (_root.get() == old) {
                auto owned_old = std::exchange(_root, std::move(new_));
                _root->set_parent(this);
                _root->refresh_heights();
                return owned_old;
            }
            assert(false && "invalid child of management object tree (replace_this_as_child)");
            LDB_UNREACHABLE;
        }

        std::unique_ptr<node_type>
        detach_child(node_type* child) override {
            if (_root.get() == child) return detach_left(); // or right
            assert(false && "invalid child of management object tree (detach_child)");
            LDB_UNREACHABLE;
        }

        std::unique_ptr<node_type>
        detach_left() override {
            return std::unique_ptr<node_type>{_root.release()};
        }

        std::unique_ptr<node_type>
        detach_right() override {
            return std::unique_ptr<node_type>{_root.release()};
        }

        void
        attach_left(std::unique_ptr<node_type> left) override {
            assert(_root == nullptr && "child node not null of management object tree (attach_left)");
            _root = std::move(left);
            _root->set_parent(this);
            _root->refresh_heights();
        }

        void
        attach_right(std::unique_ptr<node_type> right) override {
            assert(_root == nullptr && "child node not null of management object tree (attach_right)");
            _root = std::move(right);
            _root->set_parent(this);
            _root->refresh_heights();
        }

        void
        increment_side_of_child(node_type* child) override {
            std::ignore = child;
            /* nop */
        }

        std::unique_ptr<tree_node<payload_type>> _root = std::make_unique<tree_node<payload_type>>(static_cast<tree_node_handler<tree_node<payload_type>>*>(this));
    };
}

#endif
