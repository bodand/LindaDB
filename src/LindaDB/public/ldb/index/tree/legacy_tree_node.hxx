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
 * src/LindaDB/public/ldb/index/tree/tree_node --
 *   Provides a vertex in the universal AVL tree class.
 */
#ifndef LINDADB_TREE_NODE_HXX
#define LINDADB_TREE_NODE_HXX

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <type_traits>
#include <variant>

#include <ldb/index/tree/payload.hxx>
#include <ldb/index/tree/tree_node_handler.hxx>
#include <ldb/profiler.hxx>

#include <spdlog/spdlog.h>

namespace ldb::index::tree {
    struct new_node_tag final { };

    template<payload T>
    struct legacy_tree_node final : private tree_node_handler<legacy_tree_node<T>> {
        using factor_type = std::make_signed_t<std::size_t>;
        using key_type = typename T::key_type;
        using value_type = typename T::value_type;
        using size_type = typename T::size_type;

        explicit
        legacy_tree_node(tree_node_handler<legacy_tree_node>* parent) noexcept(std::is_nothrow_default_constructible_v<T>)
             : _parent(parent) {
            LDB_PROF_SCOPE("TreeNode_New");
        }

        template<class... Args>
        explicit legacy_tree_node(tree_node_handler<legacy_tree_node>* parent,
                           new_node_tag /*x*/,
                           Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
             : _payload(std::forward<Args>(args)...),
               _parent(parent) {
            LDB_PROF_SCOPE("TreeNode_NewValue");
        }

        legacy_tree_node(const legacy_tree_node& cp) = default;
        legacy_tree_node(legacy_tree_node&& mv) noexcept = default;

        legacy_tree_node&
        operator=(const legacy_tree_node& cp) = default;
        legacy_tree_node&
        operator=(legacy_tree_node&& mv) noexcept = default;

        ~legacy_tree_node() noexcept override {
            LDB_PROF_SCOPE("TreeNode_Destroy");
            while (_left) release(&_left);
            while (_right) release(&_right);
        }

        [[nodiscard]] factor_type
        balance_factor() const noexcept {
            return _balance_factor;
        }

        [[nodiscard]] constexpr size_type
        capacity() const noexcept {
            return _payload.capacity();
        }

        [[nodiscard]] size_type
        size() const noexcept {
            return _payload.size();
        }

        [[nodiscard]] size_type
        full() const noexcept {
            return _payload.full();
        }

        [[nodiscard]] size_type
        empty() const noexcept {
            return _payload.empty();
        }

        void
        dump(std::ostream& os, std::size_t depth = 0) const {
            depth += 2;
            os << "(" << _payload << " " << balance_factor() << "\n"
               << std::string(depth, ' ');
            if (_left) {
                _left->dump(os, depth);
            }
            else {
                os << "()";
            }
            os << "\n"
               << std::string(depth, ' ');

            if (_right) {
                _right->dump(os, depth);
            }
            else {
                os << "()";
            }
            os << ")";
        }

        template<index_query<value_type> Q>
        [[nodiscard]] std::variant<legacy_tree_node*, std::optional<value_type>>
        search(const Q& query) const {
            LDB_PROF_SCOPE_C("TreeNode_Search", prof::color_search);
            auto order = _payload <=> query.key();
            if (order > 0) {
                if (!_left.get()) return std::nullopt;
                return _left.get();
            }
            if (order < 0) {
                if (!_right.get()) return std::nullopt;
                return _right.get();
            }
            return _payload.try_get(query);
        }

        [[nodiscard]] legacy_tree_node*
        insert(const key_type& key,
               const value_type& value) {
            LDB_PROF_SCOPE_C("TreeNode_Insert", prof::color_insert);
            auto order = _payload <=> key;
            if (order != 0) {
                // try to dense the tree: if the current node has less than optimal payload size,
                // and we can act on it without breaking the bst property, we do so
                if (_payload.size() < 2 && _payload.capacity() >= 2) {
                    assert(!_payload.full()
                           && "a payload MUST NOT get priority insertion if it is full");

                    if (auto succ = _payload.try_set(key, value);
                        succ) return nullptr; // if we managed to help the priority node we are done
                }
                // even more dense: if the current node 1) is a leaf; and 2) not bounding node of key; and 3) not full
                // we can try to widen the current node with the to-be-inserted element instead of creating a new leaf
                if (!_payload.full()
                    && _left == nullptr
                    && _right == nullptr) {
                    if (auto succ = _payload.try_set(key, value);
                        succ) return nullptr; // short-circuited
                }
            }

            if (order > 0) {
                if (_left) return _left.get();
                add_left(key, value);
                return nullptr;
            }

            if (order < 0) {
                if (_right) return _right.get(); // ->insert(key, value);
                add_right(key, value);
                return nullptr;
            }

            if (auto squished = _payload.force_set_lower(key, value);
                squished) { // squished some element out from this layer
                auto bundle = *squished;
                insert_to_glb(std::move(bundle));
            }
            return nullptr;
        }

        template<index_query<value_type> Q>
        [[nodiscard]] legacy_tree_node*
        remove(const Q& query,
               std::optional<value_type>* value_out) {
            LDB_PROF_SCOPE_C("TreeNode_Remove", prof::color_remove);
            auto order = _payload <=> query.key();

            if (order > 0) {
                if (_left) return _left.get();
                return nullptr;
            }

            if (order < 0) {
                if (_right) return _right.get();
                return nullptr;
            }

            if (auto res = _payload.remove(query);
                res) {
                if (value_out) *value_out = *res;
                // must remove node
                if (_payload.empty()) {
                    LDB_PROF_SCOPE_C("TreeNode_CleanupEmpty", prof::color_remove);
                    if (!_left && !_right) { // no children
                        _parent->detach_this(this);
                        return nullptr;
                    }
                    if (_left && !_right) { // only left child
                        // this line scraps `this`
                        std::ignore = _parent->replace_this_as_child(this, std::move(_left), update_type::INCREMENT);
                        return nullptr;
                    }
                    if (!_left && _right) { // only right child
                        // this line scraps `this`
                        std::ignore = _parent->replace_this_as_child(this, std::move(_right), update_type::DECREMENT);
                        return nullptr;
                    }
                    // both children exist :(
                    // have both children -> successor must from children -> successor == min_child
                    auto successor_ref = min_child(&_right);
                    // successor does not have left node (or that would be the successor)
                    // and it only may have a single child as its right subtree (or its balance factor
                    // would be > 1)
                    assert((*successor_ref)->_left == nullptr);
                    assert((*successor_ref)->_balance_factor >= 0);
                    assert((*successor_ref)->_balance_factor <= 1);
                    auto successor = (*successor_ref)->_parent->replace_this_as_child((*successor_ref).get(), //
                                                                                      std::move((*successor_ref)->_right),
                                                                                      update_type::DECREMENT);
                    successor->_right = nullptr;

                    // successor is always a single node
                    assert(successor->_left == nullptr);
                    assert(successor->_right == nullptr);
                    successor->attach_right(detach_right());
                    successor->attach_left(detach_left());
                    successor->_balance_factor = _balance_factor;
                    // this line scraps `this`
                    std::ignore = _parent->replace_this_as_child(this, std::move(successor));
                }
                return nullptr;
            }
            if (value_out) *value_out = std::nullopt;
            return nullptr;
        }

        void
        set_parent(tree_node_handler<legacy_tree_node<T>>* parent) {
            _parent = parent;
        }

    private:
        using squished_type = T::bundle_type;

        static void
        release(std::unique_ptr<legacy_tree_node>* subtree) noexcept {
            LDB_PROF_SCOPE("TreeNode_ReleaseSubtree");
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

        std::unique_ptr<legacy_tree_node>
        replace_this_as_child(legacy_tree_node<T>* old, std::unique_ptr<legacy_tree_node<T>>&& new_, update_type type) override {
            LDB_PROF_SCOPE("TreeNode_ReplaceThisAsChild");
            if (_left.get() == old) {
                LDB_PROF_SCOPE("TreeNode_ReplaceLeft");
                auto owned_old = std::exchange(_left, std::move(new_));
                if (_left) {
                    _left->_parent = this;
                    update_balance_factor(static_cast<int>(type), _left.get());
                }
                else if (owned_old) {
                    update_balance_factor(1, nullptr);
                }
                return owned_old;
            }
            if (_right.get() == old) {
                LDB_PROF_SCOPE("TreeNode_ReplaceRight");
                auto owned_old = std::exchange(_right, std::move(new_));
                if (_right) {
                    _right->_parent = this;
                    update_balance_factor(static_cast<int>(type), _right.get());
                }
                else if (owned_old) {
                    update_balance_factor(-1, nullptr);
                }
                return owned_old;
            }
            assert(false && "invalid child pointer in replace_this_as_child (tree_node)");
            LDB_UNREACHABLE;
        }

        std::unique_ptr<legacy_tree_node>
        detach_left() override {
            LDB_PROF_SCOPE("TreeNode_DetachLeft");
            return std::unique_ptr<legacy_tree_node>{_left.release()};
        }

        std::unique_ptr<legacy_tree_node>
        detach_right() override {
            LDB_PROF_SCOPE("TreeNode_DetachRight");
            return std::unique_ptr<legacy_tree_node>{_right.release()};
        }

        bool
        update_balance_factor(int by, legacy_tree_node<T>* child) {
            LDB_PROF_SCOPE("TreeNode_UpdateBalanceFactor");
            if (by == 0) return true;
            _balance_factor += by;
            if (_balance_factor == 2) {
                if (child && child->balance_factor() < 0) {
                    rotate_right_left(this, _right.get());
                }
                else {
                    rotate_left(this, _right.get());
                }
                _balance_factor = 0;
                return true;
            }
            if (_balance_factor == -2) {
                if (child && child->balance_factor() > 0) {
                    rotate_left_right(this, _left.get());
                }
                else {
                    rotate_right(this, _left.get());
                }
                _balance_factor = 0;
                return true;
            }
            return false;
        }

        void
        detach_this(legacy_tree_node<T>* child) override {
            LDB_PROF_SCOPE("TreeNode_DetachThis");
            assert(child);

            auto parent = _parent;
            if (_left.get() == child) {
                /* destroy */ detach_left();
                if (update_balance_factor(1, nullptr)) parent = nullptr;
            }
            if (_right.get() == child) {
                /* destroy */ detach_right();
                if (update_balance_factor(-1, nullptr)) parent = nullptr;
            }
            if (parent) parent->update_side_of_child(this, update_type::DECREMENT);
        }

        void
        attach_left(std::unique_ptr<legacy_tree_node> left) override {
            LDB_PROF_SCOPE("TreeNode_AttachLeft");
            assert(_left == nullptr && "attaching to non-empty left of node");
            _left = std::move(left);
            if (_left) {
                _left->_parent = this;
            }
        }

        void
        attach_right(std::unique_ptr<legacy_tree_node> right) override {
            LDB_PROF_SCOPE("TreeNode_AttachRight");
            assert(_right == nullptr && "attaching to non-empty right of node");
            _right = std::move(right);
            if (_right) {
                _right->_parent = this;
            }
        }

        static std::unique_ptr<legacy_tree_node>*
        min_child(std::unique_ptr<legacy_tree_node>* node) {
            LDB_PROF_SCOPE("TreeNode_MinChild");
            assert(node && "node must not be empty");
            assert((*node) && "*node must not be empty");
            while ((*node)->_left) {
                node = &(*node)->_left;
            }
            return node;
        }

        void
        update_side_of_child(legacy_tree_node* child, update_type type) override {
            LDB_PROF_SCOPE("TreeNode_UpdateSideOfChild");
            assert(child);
            if (type == update_type::NO_UPDATE) return;

            auto parent = _parent;
            if (_left.get() == child
                && update_balance_factor(-1 * static_cast<int>(type), child)) parent = nullptr;

            if (_right.get() == child
                && update_balance_factor(1 * static_cast<int>(type), child)) parent = nullptr;

            assert(abs(child->_balance_factor) < 2
                   && "child is not balanced after rotation(s)");
            assert(abs(_balance_factor) < 2
                   && "node is not balanced after rotation(s)");
            if (parent) parent->update_side_of_child(this, type);
        }

        static tree_node_handler<legacy_tree_node>*
        rotate_right(legacy_tree_node* self, legacy_tree_node* child) {
            LDB_PROF_SCOPE("TreeRotate_Right");
            assert(self);
            assert(child);
            assert(self->_left.get() == child);

            auto ret = right_rotate(self, child);

            self->_balance_factor = 1 * (child->_balance_factor == 0);
            child->_balance_factor = -1 * (child->_balance_factor == 0);

            return ret;
        }

        static tree_node_handler<legacy_tree_node>*
        rotate_left(legacy_tree_node* self, legacy_tree_node* child) {
            LDB_PROF_SCOPE("TreeRotate_Left");
            assert(self);
            assert(child);
            assert(self->_right.get() == child);

            auto ret = left_rotate(self, child);

            self->_balance_factor = 1 * (child->_balance_factor == 0);
            child->_balance_factor = -1 * (child->_balance_factor == 0);

            return ret;
        }

        static tree_node_handler<legacy_tree_node>*
        rotate_left_right(legacy_tree_node* self, legacy_tree_node* child) {
            LDB_PROF_SCOPE("TreeRotate_LeftRight");
            assert(self);
            assert(child);
            assert(child->_right);
            assert(self->_left.get() == child);
            // assert(child->balance_factor() > 0);

            auto inner_node = child->_right.get();
            auto new_parent = left_rotate(child, inner_node);
            assert(new_parent == self);
            std::ignore = new_parent;
            auto ret = right_rotate(self, inner_node);

            self->_balance_factor = -1 * (inner_node->_balance_factor > 0);
            child->_balance_factor = 1 * (inner_node->_balance_factor < 0);
            inner_node->_balance_factor = 0;

            return ret;
        }

        static tree_node_handler<legacy_tree_node>*
        rotate_right_left(legacy_tree_node* self, legacy_tree_node* child) {
            LDB_PROF_SCOPE("TreeRotate_RightLeft");
            assert(self);
            assert(child);
            assert(child->_left);
            assert(self->_right.get() == child);
            // assert(child->balance_factor() < 0);

            auto inner_node = child->_left.get();
            auto new_parent = right_rotate(child, inner_node);
            assert(new_parent == self);
            std::ignore = new_parent;
            auto ret = left_rotate(self, inner_node);

            self->_balance_factor = -1 * (inner_node->_balance_factor > 0);
            child->_balance_factor = 1 * (inner_node->_balance_factor < 0);
            inner_node->_balance_factor = 0;

            return ret;
        }

        static tree_node_handler<legacy_tree_node>*
        right_rotate(legacy_tree_node* self, legacy_tree_node* child) {
            auto owned_self = self->_parent->replace_this_as_child(self, self->detach_left());
            owned_self->attach_left(child->detach_right());
            child->attach_right(std::move(owned_self));
            return child->_parent;
        }

        static tree_node_handler<legacy_tree_node>*
        left_rotate(legacy_tree_node* self, legacy_tree_node* child) {
            auto owned_self = self->_parent->replace_this_as_child(self, self->detach_right());
            owned_self->attach_right(child->detach_left());
            child->attach_left(std::move(owned_self));
            return child->_parent;
        }

        void
        increment_left() {
            --_balance_factor;
            if (_balance_factor == 0) return;
            if (_parent) _parent->update_side_of_child(this, update_type::INCREMENT);
        }

        void
        increment_right() {
            ++_balance_factor;
            if (_balance_factor == 0) return;
            if (_parent) _parent->update_side_of_child(this, update_type::INCREMENT);
        }

        template<class... Args>
        void
        add_left(Args&&... args) {
            LDB_PROF_SCOPE("TreeNode_AddLeft");
            assert(_balance_factor >= 0);
            assert(_left == nullptr);
            _left = std::make_unique<legacy_tree_node<T>>(static_cast<tree_node_handler<legacy_tree_node>*>(this),
                                                   new_node_tag{},
                                                   std::forward<Args>(args)...);
            increment_left();
        }

        template<class... Args>
        void
        add_right(Args&&... args) {
            LDB_PROF_SCOPE("TreeNode_AddRight");
            assert(_balance_factor <= 0);
            assert(_right == nullptr);
            _right = std::make_unique<legacy_tree_node<T>>(static_cast<tree_node_handler<legacy_tree_node>*>(this),
                                                    new_node_tag{},
                                                    std::forward<Args>(args)...);
            increment_right();
        }

        void
        insert_to_glb(squished_type&& squished) {
            LDB_PROF_SCOPE("TreeNode_InsertToGlb");
            if (_left) return _left->insert_to_glb_descent(std::move(squished));
            add_left(std::move(squished));
        }

        void
        insert_to_glb_descent(squished_type&& squished) {
            if (_right) return _right->insert_to_glb_descent(std::move(squished));

            if (auto succ = _payload.try_set(squished);
                succ) return;

            auto new_squished = _payload.force_set_upper(std::move(squished));
            assert(new_squished.has_value());
            add_right(std::move(*new_squished));
        }

        T _payload{};
        factor_type _balance_factor = 0;
        tree_node_handler<legacy_tree_node<T>>* _parent = nullptr;
        std::unique_ptr<legacy_tree_node<T>> _left = nullptr;
        std::unique_ptr<legacy_tree_node<T>> _right = nullptr;
    };
}

#endif
