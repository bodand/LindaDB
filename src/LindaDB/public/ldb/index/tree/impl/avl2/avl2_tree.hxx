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
 * Originally created: 2023-11-18.
 *
 * src/LindaDB/public/ldb/index/tree/impl/avl2_tree --
 *   Implementation of an AVL tree hosting data in a payload type.
 */
#ifndef AVL2_TREE_HXX
#define AVL2_TREE_HXX

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>

#include <ldb/common.hxx>
#include <ldb/index/tree/index_query.hxx>
#include <ldb/index/tree/payload.hxx>
#include <ldb/index/tree/payload_dispatcher.hxx>

inline std::ostream&
operator<<(std::ostream&& os, const std::weak_ordering& wo) {
    if (wo < 0) return os << "(LESS)";
    if (wo > 0) return os << "(GREATER)";
    return os << "(EQ)";
}

namespace ldb::index::tree {
    enum avlbf : std::int8_t {
        LEFT_HEAVY = -1,
        BALANCED = 0,
        RIGHT_HEAVY = 1
    };

    enum avltype : std::int8_t {
        LEAF,
        HALF_LEAF,
        INTERNAL
    };

    template<payload P>
    struct avl2_node {
        using payload_type = P;
        using key_type = payload_type::key_type;
        using value_type = payload_type::value_type;

        avlbf bf = BALANCED;
        P data{};

        avl2_node() = default;

        template<class... Args>
        explicit avl2_node(std::unique_ptr<avl2_node>* par,
                           Args&&... args)
             : data(std::forward<Args>(args)...),
               _parent(par) {
            LDBT_ZONE_A;
            assert_that(_left.get() != this);
            assert_that(_right.get() != this);
            assert_that(_parent == nullptr || _parent->get() != this);
            assert_that(_parent == nullptr || *_parent != nullptr);
        }

        avl2_node(const avl2_node& other) = delete;
        avl2_node(avl2_node&& other) noexcept
             : _parent(other._parent),
               _left(other.release_left()),
               _right(other.release_right()),
               bf(other.bf),
               data(std::move(other.data)) {
            LDBT_ZONE_A;
            assert_that(_left.get() != this);
            assert_that(_right.get() != this);
            assert_that(_parent == nullptr || _parent->get() != this);
            assert_that(_parent == nullptr || *_parent != nullptr);
        }

        avl2_node&
        operator=(const avl2_node& other) = delete;
        avl2_node&
        operator=(avl2_node&& other) noexcept {
            LDBT_ZONE_A;
            if (this == &other) return *this;
            set_parent(other._parent);
            set_left(other.release_left());
            set_right(other.release_right());
            bf = other.bf;
            data = std::move(other.data);
            return *this;
        }

        using side_pointer = std::unique_ptr<avl2_node>* (avl2_node::*) ();
        using set_side_pointer = void (avl2_node::*)(std::unique_ptr<avl2_node>);

        [[nodiscard]] auto
        get_side_of(avl2_node* child) const noexcept {
            LDBT_ZONE_A;
            assert_that(_left.get() != this);
            assert_that(_right.get() != this);
            assert_that(_parent == nullptr || _parent->get() != this);
            assert_that(_parent == nullptr || *_parent != nullptr);
            if (child == _left.get()) return std::make_pair<side_pointer, set_side_pointer>(
                   &avl2_node::left_ptr,
                   &avl2_node::set_left);
            if (child == _right.get()) return std::make_pair<side_pointer, set_side_pointer>(
                   &avl2_node::right_ptr,
                   &avl2_node::set_right);
            assert_that(false && "non direct child passed to get_side_of");
            LDB_UNREACHABLE;
        }

        [[nodiscard]] std::unique_ptr<avl2_node<P>>*
        parent() const {
            LDBT_ZONE_A;
            assert_that(_left.get() != this);
            assert_that(_right.get() != this);
            assert_that(_parent == nullptr || _parent->get() != this);
            assert_that(_parent == nullptr || *_parent != nullptr);
            return _parent;
        }

        void
        set_parent(std::unique_ptr<avl2_node<P>>* p) {
            LDBT_ZONE_A;
            assert_that(_left.get() != this);
            assert_that(_right.get() != this);
            if (p) {
                assert_that(*p != nullptr);
                assert_that((*p).get() != this);
                assert_that((*p).get()->left().get() == this
                            || (*p).get()->right().get() == this);
            }
            _parent = p;
        }

        [[nodiscard]] const std::unique_ptr<avl2_node>&
        left() const {
            LDBT_ZONE_A;
            assert_that(_left.get() != this);
            assert_that(_right.get() != this);
            assert_that(_parent == nullptr || _parent->get() != this);
            assert_that(_parent == nullptr || *_parent != nullptr);
            return _left;
        }
        [[nodiscard]] decltype(auto)
        release_left() {
            LDBT_ZONE_A;
            assert_that(_left.get() != this);
            assert_that(_right.get() != this);
            assert_that(_parent == nullptr || _parent->get() != this);
            assert(_parent == nullptr || *_parent != nullptr);
            if (_left) _left->_parent = nullptr;
            return std::move(_left);
        }

        [[nodiscard]] const std::unique_ptr<avl2_node>*
        left_ptr() const {
            LDBT_ZONE_A;
            assert(_left.get() != this);
            assert(_right.get() != this);
            assert(_parent == nullptr || _parent->get() != this);
            assert(_parent == nullptr || *_parent != nullptr);
            return &_left;
        }
        [[nodiscard]] std::unique_ptr<avl2_node>*
        left_ptr() {
            LDBT_ZONE_A;
            assert(_left.get() != this);
            assert(_right.get() != this);
            assert(_parent == nullptr || _parent->get() != this);
            assert(_parent == nullptr || *_parent != nullptr);
            return &_left;
        }

        void
        set_left(std::unique_ptr<avl2_node> left) {
            LDBT_ZONE_A;
            assert(_left.get() != this);
            assert(_right.get() != this);
            assert(_parent == nullptr || _parent->get() != this);
            assert(_parent == nullptr || *_parent != nullptr);
            _left = std::move(left);
            if (_left) {
                if (_left->_left) _left->_left->set_parent(&_left);
                if (_left->_right) _left->_right->set_parent(&_left);
                if (auto par = parent();
                    par) {
                    auto [get, set] = (*par)->get_side_of(this);
                    _left->set_parent((**par.*get)());
                }
            }
        }

        [[nodiscard]] const std::unique_ptr<avl2_node>&
        right() const {
            LDBT_ZONE_A;
            assert(_left.get() != this);
            assert(_right.get() != this);
            assert(_parent == nullptr || _parent->get() != this);
            assert(_parent == nullptr || *_parent != nullptr);
            return _right;
        }
        [[nodiscard]] decltype(auto)
        release_right() {
            LDBT_ZONE_A;
            assert(_left.get() != this);
            assert(_right.get() != this);
            assert(_parent == nullptr || _parent->get() != this);
            assert(_parent == nullptr || *_parent != nullptr);
            if (_right) _right->_parent = nullptr;
            return std::move(_right);
        }

        [[nodiscard]] const std::unique_ptr<avl2_node>*
        right_ptr() const {
            LDBT_ZONE_A;
            assert(_left.get() != this);
            assert(_right.get() != this);
            assert(_parent == nullptr || _parent->get() != this);
            assert(_parent == nullptr || *_parent != nullptr);
            return &_right;
        }
        [[nodiscard]] std::unique_ptr<avl2_node>*
        right_ptr() {
            LDBT_ZONE_A;
            assert(_left.get() != this);
            assert(_right.get() != this);
            assert(_parent == nullptr || _parent->get() != this);
            assert(_parent == nullptr || *_parent != nullptr);
            return &_right;
        }

        void
        set_right(std::unique_ptr<avl2_node> right) {
            LDBT_ZONE_A;
            assert(_left.get() != this);
            assert(_right.get() != this);
            assert(_parent == nullptr || _parent->get() != this);
            assert(_parent == nullptr || *_parent != nullptr);
            _right = std::move(right);
            if (_right) {
                if (_right->_left) _right->_left->set_parent(&_right);
                if (_right->_right) _right->_right->set_parent(&_right);
                if (auto par = parent();
                    par) {
                    auto [get, set] = (*par)->get_side_of(this);
                    _right->set_parent((**par.*get)());
                }
            }
        }

        [[nodiscard]] bool
        balance_is(const avlbf type) const {
            LDBT_ZONE_A;
            assert(_left.get() != this);
            assert(_right.get() != this);
            assert(_parent == nullptr || _parent->get() != this);
            assert(_parent == nullptr || *_parent != nullptr);
            return bf == type;
        }

        auto
        try_insert(const key_type& key,
                   const value_type& value) {
            LDBT_ZONE_A;
            assert(_left.get() != this);
            assert(_right.get() != this);
            assert(_parent == nullptr || _parent->get() != this);
            assert(_parent == nullptr || *_parent != nullptr);
            return data.try_set(key, value);
        }

        template<class B>
        auto
        try_insert(B&& bundle) {
            LDBT_ZONE_A;
            assert(_left.get() != this);
            assert(_right.get() != this);
            assert(_parent == nullptr || _parent->get() != this);
            assert(_parent == nullptr || *_parent != nullptr);
            return data.try_set(std::forward<B>(bundle));
        }

        auto
        insert_into_lower(const key_type& key,
                          const value_type& value) {
            LDBT_ZONE_A;
            assert(_left.get() != this);
            assert(_right.get() != this);
            assert(_parent == nullptr || _parent->get() != this);
            assert(_parent == nullptr || *_parent != nullptr);
            return data.force_set_lower(key, value);
        }

        std::optional<typename P::bundle_type>
        insert_into_upper(const key_type& key,
                          const value_type& value) {
            LDBT_ZONE_A;
            assert(_left.get() != this);
            assert(_right.get() != this);
            assert(_parent == nullptr || _parent->get() != this);
            assert(_parent == nullptr || *_parent != nullptr);
            if (data.try_set(key, value)) return {};
            return data.force_set_upper(key, value);
        }

        template<class Q>
        auto
        find_by_query(const Q& query) const {
            LDBT_ZONE_A;
            assert(_left.get() != this);
            assert(_right.get() != this);
            assert(_parent == nullptr || _parent->get() != this);
            assert(_parent == nullptr || *_parent != nullptr);
            return data.try_get(query);
        }

        template<class Q>
        auto
        remove_by_query(const Q& query) {
            LDBT_ZONE_A;
            assert(_left.get() != this);
            assert(_right.get() != this);
            assert(_parent == nullptr || _parent->get() != this);
            assert(_parent == nullptr || *_parent != nullptr);
            return data.remove(query);
        }

        avltype
        type() {
            LDBT_ZONE_A;
            assert(_left.get() != this);
            assert(_right.get() != this);
            assert(_parent == nullptr || _parent->get() != this);
            assert(_parent == nullptr || *_parent != nullptr);
            if (_left && _right) return INTERNAL;
            if (_left || _right) return HALF_LEAF;
            return LEAF;
        }

        template<class Fn>
        void
        apply(const Fn& fn) const {
            LDBT_ZONE_A;
            if (_left) _left->apply(fn);
            data.apply(fn);
            if (_right) _right->apply(fn);
        }

        ~avl2_node() {
            LDBT_ZONE_A;
            assert(_left.get() != this);
            assert(_right.get() != this);
            if (_left) release(&_left);
            if (_right) release(&_right);
        }

    private:
        friend std::ostream&
        operator<<(std::ostream& os, const avl2_node& node) {
            return os << "[AVL NODE: " << node.data << "]";
        }

        std::unique_ptr<avl2_node>* _parent{};
        std::unique_ptr<avl2_node> _left{};
        std::unique_ptr<avl2_node> _right{};

        static void
        release(std::unique_ptr<avl2_node>* subtree) noexcept {
            LDBT_ZONE_A;
            while ((*subtree)->_left && (*subtree)->_right) {
                subtree = &(*subtree)->_left;
            }

            if ((*subtree)->_left) {
                *subtree = std::move((*subtree)->_left);
            }
            else if ((*subtree)->_right) {
                *subtree = std::move((*subtree)->_right);
            }
            else {
                (*subtree).reset();
            }
        }
    };

    template<class K,
             class V,
             std::size_t Clustering = 0,
             class PayloadType = typename payload_dispatcher<K, V, Clustering>::type>
    struct avl2_tree {
        using payload_type = PayloadType;
        using key_type = payload_type::key_type;
        using value_type = payload_type::value_type;

        template<index_lookup<value_type> Q>
        [[nodiscard]] std::optional<value_type>
        search(const Q& query) const {
            LDBT_ZONE_A;
            LDBT_SH_LOCK(_mtx);
            const auto* node = traverse_tree(query.key());
            if (!*node) return {};

            return (*node)->find_by_query(query);
        }

        template<class Q>
        [[nodiscard]] std::optional<value_type>
        search_query(const Q& query) const {
            LDBT_ZONE_A;
            LDBT_SH_LOCK(_mtx);
            const auto* node = traverse_tree(query);
            if (!*node) return {};

            return (*node)->find_by_query(query);
        }

        void
        insert(const value_type& value)
            requires(std::same_as<key_type, value_type>)
        {
            insert(value, value);
        }

        void
        insert(const key_type& key,
               const value_type& value) {
            LDBT_ZONE_A;
            LDBT_UQ_LOCK(_mtx);
            std::unique_ptr<node_type>* parent = nullptr;
            std::unique_ptr<node_type>* current = &root;
            while (*current) {
                auto cmp = key <=> (*current)->data;
                if (cmp == 0) {
                    // T-tree, if there is enough space in
                    // bounding node, then just insert
                    // nothing changes in the tree structure
                    if (auto squished =
                               (*current)->insert_into_lower(key, value);
                        squished) {
                        // bounding node could not fit element
                        // needed to squeeze it in -> squished out largest
                        // element in bounding node, need to place that
                        parent = current;
                        current = (*current)->left_ptr();
                        while (*current) {
                            parent = current;
                            current = (*current)->right_ptr();
                        }

                        // node currently behaving as glb may be able to fit
                        // the squished value, if so no need to create new node
                        if (auto succ = (*parent)->try_insert(*squished);
                            succ) return;

                        // could not insert squished value into existing node,
                        // new node needs to be made
                        *current = std::make_unique<node_type>(parent, *squished);
                        break;
                    }

                    // no squeezing performed, just insertion into existing node
                    // no need for further action
                    return;
                }

                parent = current;
                if (cmp < 0) {
                    current = (*current)->left_ptr();
                }
                else {
                    current = (*current)->right_ptr();
                }
            }

            // T-tree, if last node could fit elem to be inserted
            // insert it there
            // if it does not fit, fall back to AVL-tree behavior
            // of inserting new node
            if (parent != nullptr && *current == nullptr) {
                if (auto succ = (*parent)->try_insert(key, value);
                    succ) return;
            }

            // normal AVL-tree behavior, with check if T-tree did not
            // already perform the insertion for us
            if (*current == nullptr) {
                *current = std::make_unique<node_type>(parent,
                                                       key,
                                                       value);
            }

            while ((*current)->parent()) {
                assert(parent);
                assert(*parent);
                auto& p = *parent;
                if ((*current).get() == p->left().get()) {
                    if (p->bf == RIGHT_HEAVY) {
                        // right heavy subtree increased on left end
                        // -> subtree now balanced
                        p->bf = BALANCED;
                        break;
                    }
                    if (p->balance_is(BALANCED)) {
                        // was balanced, now left-heavy
                        p->bf = LEFT_HEAVY;
                    }
                    else if (p->bf == LEFT_HEAVY) {
                        // right heavy subtree increased on left end
                        // -> rotate
                        insert_left_imbalance(parent);
                        break;
                    }
                }
                else {
                    if (p->balance_is(LEFT_HEAVY)) {
                        // left heavy subtree increased on right end
                        // -> subtree now balanced
                        p->bf = BALANCED;
                        break;
                    }
                    if (p->balance_is(BALANCED)) {
                        // was balanced, now right-heavy
                        p->bf = RIGHT_HEAVY;
                    }
                    else if (p->balance_is(RIGHT_HEAVY)) {
                        // right heavy subtree increased on right end
                        // -> rotate
                        insert_right_imbalance(parent);
                        break;
                    }
                }

                current = parent;
                parent = (*current)->parent();
            }
        }


        template<index_lookup<value_type> Q>
        [[nodiscard]] std::optional<value_type>
        remove(const Q& query) {
            LDBT_ZONE_A;
            LDBT_UQ_LOCK(_mtx);
            auto* node = traverse_tree(query.key());
            if (!*node) return {};

            // T-tree: removing from node does not always result
            //         in removal of the node
            // query mechanism: finding a bounding node does not always mean
            //                  we can remove from it
            auto found = (*node)->remove_by_query(query);
            if (!found) return {};

            return remove_node_internal(node, std::move(found));
        }

        template<class Q>
        [[nodiscard]] std::optional<value_type>
        remove_query(const Q& query) {
            LDBT_ZONE_A;
            LDBT_UQ_LOCK(_mtx);
            auto* node = traverse_tree(query);
            if (!*node) return {};

            // T-tree: removing from node does not always result
            //         in removal of the node
            // query mechanism: finding a bounding node does not always mean
            //                  we can remove from it
            auto found = (*node)->remove_by_query(query);
            if (!found) return {};

            return remove_node_internal(node, std::move(found));
        }

        template<class Fn>
        void
        apply(const Fn& fn) const {
            LDBT_ZONE_A;
            LDBT_UQ_LOCK(_mtx);
            if (root) root->apply(fn);
        }

    private:
        using node_type = avl2_node<payload_type>;

        template<class T>
        auto
        remove_node_internal(std::unique_ptr<node_type>* node, T&& found) {
            // T-tree: depending on the node type, we may need to shuffle around
            //         some elements in the nodes
            switch ((*node)->type()) {
            case LEAF:
                if ((*node)->data.empty()) {
                    delete_node(node);
                }
                break;
            case HALF_LEAF: {
                if (handle_half_leaf_removal(node)) return std::forward<T>(found);
                break;
            }

            case INTERNAL: {
                if ((*node)->data.size() > (*node)->data.capacity() / 2) {
                    // if node is still more than half-full don't do anything
                    break;
                }

                auto* glb = find_greatest_lower_bound(node);
                assert(*glb); // otherwise couldn't be INTERNAL

                (*node)->data.merge_until_full((*glb)->data);
                auto glb_type = (*glb)->type();
                assert(glb_type != INTERNAL);

                if (glb_type == LEAF) {
                    if ((*glb)->data.empty()) delete_node(glb);
                    break;
                }

                assert(glb_type == HALF_LEAF);
                // try merge for half-leaf glb
                //   success -> handle_half_leaf removes now empty leaf
                //   failure -> nothing happens
                // either case we have nothing to do
                handle_half_leaf_removal(glb);
                break;
            }
            }

            return std::forward<T>(found);
        }

        bool
        handle_half_leaf_removal(std::unique_ptr<node_type>* node) {
            LDBT_ZONE_A;
            auto* leaf = (*node)->left_ptr();
            if (*leaf == nullptr) leaf = (*node)->right_ptr();
            assert((*leaf)->type() == LEAF);

            // t-tree: if merging did not succeed (not enought space in
            //         half-leaf node) then don't do anything
            if (!(*node)->data.try_merge((*leaf)->data)) {
                return true;
            }

            // if merge suceeded, then leaf is empty, and thus needs to be
            // removed
            delete_node(leaf);
            return false;
        }

        void
        delete_node(std::unique_ptr<node_type>* node_ptr) {
            LDBT_ZONE_A;
            assert(node_ptr);
            assert(*node_ptr);
            assert((*node_ptr)->data.empty() && "non-empty node removed");

            std::unique_ptr<node_type>* target;
            if ((*node_ptr)->left() == nullptr
                || (*node_ptr)->right() == nullptr) {
                target = node_ptr;
            }
            else {
                target = find_successor(node_ptr);
                assert(target);
                assert(*target);
                (*node_ptr)->data = std::move((*target)->data);
            }

            auto* current = target;
            assert(current);
            assert(*current);
            auto* parent = (*current)->parent();

            while ((*current)->parent()) {
                assert(parent);
                assert(*parent);
                auto& p = *parent;
                if ((*current).get() == p->left().get()) {
                    if (p->balance_is(LEFT_HEAVY)) {
                        // left removed from left-heavy subtree
                        p->bf = BALANCED;
                    }
                    else if (p->balance_is(BALANCED)) {
                        // left removed from balanced subtree
                        p->bf = RIGHT_HEAVY;
                        break;
                    }
                    else {
                        parent = delete_right_imbalance(parent);
                        if ((*parent)->balance_is(LEFT_HEAVY)) break;
                    }
                }
                else {
                    if (p->balance_is(RIGHT_HEAVY)) {
                        // right removed from right-heavy subtree
                        p->bf = BALANCED;
                    }
                    else if (p->balance_is(BALANCED)) {
                        // right removed from balanced subtree
                        p->bf = LEFT_HEAVY;
                        break;
                    }
                    else {
                        parent = delete_left_imbalance(parent);
                        if ((*parent)->balance_is(RIGHT_HEAVY)) break;
                    }
                }

                current = parent;
                parent = (*current)->parent();
            }

            auto child = (*target)->left() == nullptr ? (*target)->release_right()
                                                      : (*target)->release_left();
            if (child) {
                child->set_parent((*target)->parent());
            }
            if ((*target)->parent()) {
                if (target->get() == (*(*target)->parent())->left().get()) {
                    (*(*target)->parent())->set_left(std::move(child));
                }
                else {
                    (*(*target)->parent())->set_right(std::move(child));
                }
            }
        }

        template<class Key>
        const std::unique_ptr<node_type>*
        traverse_tree(const Key& key) const {
            LDBT_ZONE_A;
            auto* node = &root;

            for (;;) {
                if (*node == nullptr) return node;
                auto dir = key <=> (*node)->data;
                if (dir < 0) {
                    node = (*node)->left_ptr();
                    continue;
                }
                if (dir > 0) {
                    node = (*node)->right_ptr();
                    continue;
                }
                return node;
            }
        }

        template<class Key>
        std::unique_ptr<node_type>*
        traverse_tree(const Key& key) {
            LDBT_ZONE_A;
            auto* node = &root;

            for (;;) {
                if (*node == nullptr) return node;
                auto dir = key <=> (*node)->data;
                if (dir < 0) {
                    node = (*node)->left_ptr();
                    continue;
                }
                if (dir > 0) {
                    node = (*node)->right_ptr();
                    continue;
                }
                return node;
            }
        }

        std::unique_ptr<node_type>*
        find_successor(std::unique_ptr<node_type>* node_ptr) {
            LDBT_ZONE_A;
            auto ptr = (*node_ptr)->right_ptr();
            if (*ptr) {
                while (*ptr) ptr = (*ptr)->left_ptr();
            }
            else {
                for (ptr = (*ptr)->parent();
                     node_ptr->get() == (*ptr)->right().get();
                     node_ptr = ptr, ptr = (*ptr)->parent())
                    ;

                if (ptr->get() == root.get())
                    ptr = nullptr;
            }

            return ptr;
        }

        std::unique_ptr<node_type>*
        find_greatest_lower_bound(std::unique_ptr<node_type>* node_ptr) {
            LDBT_ZONE_A;
            auto ptr = (*node_ptr)->left_ptr();
            if (*ptr) {
                while ((*ptr)->right()) ptr = (*ptr)->right_ptr();
            }
            return ptr;
        }

        std::unique_ptr<node_type>*
        rotate_left(std::unique_ptr<node_type>* node_ptr) {
            LDBT_ZONE_A;
            std::unique_ptr<node_type>* parent_ptr = (*node_ptr)->parent();

            if (parent_ptr == nullptr) {
                // we are replacing root
                auto owned_node = std::move(*node_ptr);
                root = owned_node->release_right();
                root->set_parent(nullptr);
                auto owned_left = root->release_left();
                root->set_left(std::move(owned_node));
                root->left()->set_parent(&root);
                root->left()->set_right(std::move(owned_left));
                if (root->right()) root->right()->set_parent(&root);
                return &root;
            }

            auto [side_ptr, set_side] = (*parent_ptr)->get_side_of(node_ptr->get());
            auto owned_node = std::move(*node_ptr);

            auto child = owned_node->release_right();
            auto child_left = child->release_left();

            (**parent_ptr.*set_side)(std::move(child));
            // owned_node->set_right(child->release_left());

            auto inserted = (**parent_ptr.*side_ptr)();
            (*inserted)->set_parent(parent_ptr);
            (*inserted)->set_left(std::move(owned_node));
            (*inserted)->left()->set_right(std::move(child_left));

            return inserted;
        }

        std::unique_ptr<node_type>*
        rotate_right(std::unique_ptr<node_type>* node_ptr) {
            LDBT_ZONE_A;
            std::unique_ptr<node_type>* parent_ptr = (*node_ptr)->parent();

            if (parent_ptr == nullptr) {
                // we are replacing root
                auto owned_node = std::move(*node_ptr);
                root = std::move(owned_node->release_left());
                root->set_parent(nullptr);
                auto owned_right = root->release_right();
                root->set_right(std::move(owned_node));
                root->right()->set_parent(&root);
                root->right()->set_left(std::move(owned_right));
                if (root->left()) root->left()->set_parent(&root);
                return &root;
            }

            auto [side_ptr, set_side] = (*parent_ptr)->get_side_of(node_ptr->get());
            auto owned_node = std::move(*node_ptr);

            auto child = owned_node->release_left();
            auto child_right = child->release_right();
            // owned_node->set_left(child->release_right());

            (**parent_ptr.*set_side)(std::move(child));

            auto inserted = (**parent_ptr.*side_ptr)();
            (*inserted)->set_parent(parent_ptr);
            (*inserted)->set_right(std::move(owned_node));
            (*inserted)->right()->set_left(std::move(child_right));

            return inserted;
        }

        std::unique_ptr<node_type>*
        insert_left_imbalance(std::unique_ptr<node_type>* node_ptr) {
            LDBT_ZONE_A;
            auto& node = *node_ptr;

            if (node->left()->bf == node->bf) {
                assert(node->balance_is(LEFT_HEAVY));
                node_ptr = rotate_right(node_ptr);
                auto& node = *node_ptr;

                node->bf = BALANCED;
                node->right()->bf = BALANCED;
                return node_ptr;
            }
            assert(node->left()->balance_is(RIGHT_HEAVY));
            assert(node->balance_is(LEFT_HEAVY));

            auto bf = node->left()->right()->bf;
            rotate_left(node->left_ptr());
            node_ptr = rotate_right(node_ptr);

            set_balance_factors(node_ptr, bf);

            return node_ptr;
        }


        std::unique_ptr<node_type>*
        insert_right_imbalance(std::unique_ptr<node_type>* node_ptr) {
            LDBT_ZONE_A;
            assert(node_ptr);
            assert(*node_ptr);
            auto& node = *node_ptr;

            if (node->right()->bf == node->bf) {
                assert(node->balance_is(RIGHT_HEAVY));
                node_ptr = rotate_left(node_ptr);
                auto& node = *node_ptr;

                node->bf = BALANCED;
                node->left()->bf = BALANCED;
                return node_ptr;
            }
            assert(node->right()->balance_is(LEFT_HEAVY));
            assert(node->balance_is(RIGHT_HEAVY));

            auto bf = node->right()->left()->bf;
            rotate_right(node->right_ptr());
            node_ptr = rotate_left(node_ptr);

            set_balance_factors(node_ptr, bf);

            return node_ptr;
        }

        std::unique_ptr<node_type>*
        delete_left_imbalance(std::unique_ptr<node_type>* node_ptr) {
            LDBT_ZONE_A;
            auto node = node_ptr->get();

            if (node->left()->balance_is(LEFT_HEAVY)) {
                node_ptr = rotate_right(node_ptr);
                node = node_ptr->get();

                node->bf = BALANCED;
                node->right()->bf = BALANCED;

                return node_ptr;
            }

            if (node->left()->balance_is(BALANCED)) {
                node_ptr = rotate_right(node_ptr);
                node = node_ptr->get();

                node->bf = RIGHT_HEAVY;
                node->right()->bf = LEFT_HEAVY;

                return node_ptr;
            }

            auto bf = node->left()->right()->bf;
            rotate_left(node->left_ptr());
            node_ptr = rotate_right(node_ptr);

            set_balance_factors(node_ptr, bf);

            return node_ptr;
        }

        std::unique_ptr<node_type>*
        delete_right_imbalance(std::unique_ptr<node_type>* node_ptr) {
            LDBT_ZONE_A;
            auto node = node_ptr->get();

            if (node->right()->balance_is(RIGHT_HEAVY)) {
                node_ptr = rotate_left(node_ptr);
                node = node_ptr->get();

                node->bf = BALANCED;
                node->left()->bf = BALANCED;
                return node_ptr;
            }

            if (node->right()->balance_is(BALANCED)) {
                node_ptr = rotate_left(node_ptr);
                node = node_ptr->get();

                node->bf = LEFT_HEAVY;
                node->left()->bf = RIGHT_HEAVY;

                return node_ptr;
            }

            auto bf = node->right()->left()->bf;
            rotate_right(node->right_ptr());
            node_ptr = rotate_left(node_ptr);

            set_balance_factors(node_ptr, bf);

            return node_ptr;
        }

        static void
        set_balance_factors(std::unique_ptr<node_type>* node_ptr,
                            avlbf bf) {
            LDBT_ZONE_A;
            (*node_ptr)->bf = BALANCED;
            auto& left = (*node_ptr)->left();
            auto& right = (*node_ptr)->right();
            if (bf == LEFT_HEAVY) {
                if (left) left->bf = BALANCED;
                if (right) right->bf = RIGHT_HEAVY;
            }
            else if (bf == RIGHT_HEAVY) {
                if (left) left->bf = LEFT_HEAVY;
                if (right) right->bf = BALANCED;
            }
            else if (bf == BALANCED) {
                if (left) left->bf = BALANCED;
                if (right) right->bf = BALANCED;
            }
        }

        std::unique_ptr<node_type> root{};
        mutable LDBT_SH_MUTEX(_mtx);
    };
}

#endif
