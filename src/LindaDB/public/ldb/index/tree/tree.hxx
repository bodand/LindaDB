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
 * src/LindaDB/public/ldb/index/tree/legacy_tree --
 *   Implementation of the main, legacy_tree class. Functions as the base parent of the
 *   root of the real legacy_tree implementation orchestrated by the tree_node objects.
 */
#ifndef LINDADB_TREE_HXX
#define LINDADB_TREE_HXX

#include <cassert>
#include <cstdlib>
#include <memory>
#include <optional>
#include <ostream>
#include <shared_mutex>
#include <sstream>
#include <variant>

#include <ldb/index/tree/legacy_tree_node.hxx>
#include <ldb/index/tree/payload_dispatcher.hxx>
#include <ldb/profiler.hxx>

namespace ldb::index::tree {
    template<class K,
             class V,
             std::size_t Clustering = 0,
             class PayloadType = typename payload_dispatcher<K, V, Clustering>::type>
    struct legacy_tree final : private tree_node_handler<legacy_tree_node<PayloadType>> {
        using payload_type = PayloadType;
        using key_type = payload_type::key_type;
        using value_type = payload_type::value_type;

        template<index_query<value_type> Q>
        [[nodiscard]] std::optional<value_type>
        search(const Q& query) const {
            LDB_PROF_SCOPE_C("Tree_Search", prof::color_search);
            LDB_SLOCK(lck, _mtx);
            std::optional<V> res;
            node_type* node = _root.get();
            while (node != nullptr) {
                std::visit(search_obj{res, node},
                           node->search(query));
            }
            return res;
        }

        void
        insert(const K& key, const V& value) {
            LDB_PROF_SCOPE_C("Tree_Insert", prof::color_search);
            LDB_LOCK(lck, _mtx);
            node_type* node = _root.get();
            do {
                node = node->insert(key, value);
            } while (node != nullptr);
        }

        template<index_query<value_type> Q>
        std::optional<value_type>
        remove(const Q& query) {
            std::optional<value_type> ret;
            remove(query, &ret);
            return ret;
        }

        template<index_query<value_type> Q>
        void
        remove(const Q& query, std::optional<value_type>* ret) {
            LDB_PROF_SCOPE_C("Tree_Remove", prof::color_remove);
            LDB_LOCK(lck, _mtx);
            node_type* node = _root.get();
            do {
                node = node->remove(query, ret);
            } while (node != nullptr);
            if (!_root) _root = std::make_unique<legacy_tree_node<payload_type>>(static_cast<tree_node_handler<legacy_tree_node<payload_type>>*>(this));
        }

        void
        dump(std::ostream& os) const {
            LDB_SLOCK(lck, _mtx);
            _root->dump(os);
            os << "\n";
        }

        [[nodiscard]] std::string
        dump_string() const {
            std::ostringstream ss;
            dump(ss);
            return ss.str();
        }

        [[nodiscard]] LDB_CONSTEXPR23 std::size_t
        node_capacity() const noexcept {
            LDB_SLOCK(lck, _mtx);
            return _root->capacity();
        }

    private:
        using node_type = legacy_tree_node<payload_type>;

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
        replace_this_as_child(node_type* old, std::unique_ptr<node_type>&& new_, update_type type) override {
            LDB_PROF_SCOPE("Tree_ReplaceRoot");
            std::ignore = type;
            assert(new_ != nullptr && "root's node cannot be null");

            if (_root.get() == old) {
                auto owned_old = std::exchange(_root, std::move(new_));
                _root->set_parent(this);
                return owned_old;
            }
            assert(false && "invalid child of management object legacy_tree (replace_this_as_child)");
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
        detach_this(node_type* child) override {
            std::ignore = child;
            _root.reset();
        }

        void
        attach_left(std::unique_ptr<node_type> left) override {
            assert(_root == nullptr && "child node not null of management object legacy_tree (attach_left)");
            _root = std::move(left);
            _root->set_parent(this);
        }

        void
        attach_right(std::unique_ptr<node_type> right) override {
            assert(_root == nullptr && "child node not null of management object legacy_tree (attach_right)");
            _root = std::move(right);
            _root->set_parent(this);
        }

        void
        update_side_of_child(node_type* child, update_type upd) override {
            std::ignore = child;
            std::ignore = upd;
            /* nop */
        }

        mutable LDB_SMUTEX(std::shared_mutex, _mtx);
        std::unique_ptr<legacy_tree_node<payload_type>> _root = std::make_unique<legacy_tree_node<payload_type>>(static_cast<tree_node_handler<legacy_tree_node<payload_type>>*>(this));
    };
}

#endif
