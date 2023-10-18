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

namespace ldb::index::tree {
    struct new_node_tag final { };

    template<payload T>
    struct tree_node final : private tree_node_handler<tree_node<T>> {
        using factor_type = std::make_signed_t<std::size_t>;
        using key_type = typename T::key_type;
        using value_type = typename T::value_type;
        using size_type = typename T::size_type;

        explicit tree_node(tree_node_handler<tree_node>* parent) noexcept(std::is_nothrow_default_constructible_v<T>)
             : _parent(parent) { }

        template<class... Args>
        explicit tree_node(tree_node_handler<tree_node>* parent,
                           new_node_tag /*x*/,
                           Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
             : _payload(std::forward<Args>(args)...),
               _parent(parent) { }

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

        [[nodiscard]] std::variant<tree_node*, std::optional<value_type>>
        search(const key_type& key) const {
            LDB_PROF_SCOPE_C("TreeNode_Search", prof::color_search);
            auto order = _payload <=> key;
            if (order > 0) {
                if (!_left.get()) return std::nullopt;
                return _left.get();
            }
            if (order < 0) {
                if (!_right.get()) return std::nullopt;
                return _right.get();
            }
            return _payload.try_get(key);
        }

        [[nodiscard]] tree_node*
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

            if (const auto& squished = _payload.force_set(key, value);
                squished) { // squished some element out from this layer
                insert_to_glb(*squished);
            }
            return nullptr;
        }

        void
        set_parent(tree_node_handler<tree_node<T>>* parent) {
            _parent = parent;
        }

        ~tree_node() noexcept override {
            if (_left) release(_left, this);
            if (_right) release(_right, this);
        }

    private:
        /**
         * WARNING: DO NOT CALL THIS FUNCTION FROM OUTSIDE A TREE_NODE (i.e. FROM THE TREE)
         */
        static void
        release(std::unique_ptr<tree_node>& sb,
                tree_node* caller) noexcept {
            auto* subtree = sb.release();
            assert(subtree->_parent == caller);
            while (subtree) {
                auto* next = subtree->_left.release();

                if (next == nullptr) {
                    next = subtree->_right.release();
                    if (!next) {
                        // THIS IS A HORRID HACK FOR SPEED
                        next = subtree->_parent->downcast_to_node();
                    }
                    std::default_delete<tree_node>{}(subtree);
                }
                subtree = next;
                if (subtree == caller) break;
            }
        }

        tree_node<T>*
        downcast_to_node() noexcept override {
            return this;
        }

        std::unique_ptr<tree_node>
        replace_this_as_child(tree_node<T>* old, std::unique_ptr<tree_node<T>>&& new_) override {
            if (_left.get() == old) {
                auto owned_old = std::exchange(_left, std::move(new_));
                if (_left) {
                    _left->_parent = this;
                }
                return owned_old;
            }
            if (_right.get() == old) {
                auto owned_old = std::exchange(_right, std::move(new_));
                if (_right) {
                    _right->_parent = this;
                }
                return owned_old;
            }
            assert(false && "invalid child pointer in replace_this_as_child (tree_node)");
            LDB_UNREACHABLE;
        }

        std::unique_ptr<tree_node>
        detach_left() override {
            return std::unique_ptr<tree_node>{_left.release()};
        }

        std::unique_ptr<tree_node>
        detach_right() override {
            return std::unique_ptr<tree_node>{_right.release()};
        }

        void
        attach_left(std::unique_ptr<tree_node> left) override {
            assert(_left == nullptr && "attaching to non-empty left of node");
            _left = std::move(left);
            if (_left) {
                _left->_parent = this;
            }
        }

        void
        attach_right(std::unique_ptr<tree_node> right) override {
            assert(_right == nullptr && "attaching to non-empty right of node");
            _right = std::move(right);
            if (_right) {
                _right->_parent = this;
            }
        }

        void
        increment_side_of_child(tree_node* child) override {
            auto parent = _parent;
            if (_left.get() == child) {
                --_balance_factor;
                if (_balance_factor == -2) {
                    parent = child->balance_factor() > 0 ? rotate_left_right(this, child)
                                                         : rotate_right(this, child);
                }
            }
            if (_right.get() == child) {
                ++_balance_factor;
                if (_balance_factor == 2) {
                    parent = child->balance_factor() < 0 ? rotate_right_left(this, child)
                                                         : rotate_left(this, child);
                }
            }

            assert(abs(child->balance_factor()) < 2
                   && "child is not balanced after rotation(s)");
            assert(abs(balance_factor()) < 2
                   && "node is not balanced after rotation(s)");
            assert(parent && "you doofus");
            parent->increment_side_of_child(this);
        }

        static tree_node_handler<tree_node>*
        rotate_right(tree_node* self, tree_node* child) {
            LDB_PROF_SCOPE("TreeRotate_Right");
            assert(self->_left.get() == child);

            auto ret = right_rotate(self, child);

            if (child->_balance_factor == 0) {
                self->_balance_factor = 1;
                child->_balance_factor = -1;
            }
            else {
                self->_balance_factor = 0;
                child->_balance_factor = 0;
            }

            return ret;
        }

        static tree_node_handler<tree_node>*
        rotate_left(tree_node* self, tree_node* child) {
            LDB_PROF_SCOPE("TreeRotate_Left");
            assert(self->_right.get() == child);

            auto ret = left_rotate(self, child);

            if (child->_balance_factor == 0) {
                self->_balance_factor = 1;
                child->_balance_factor = -1;
            }
            else {
                self->_balance_factor = 0;
                child->_balance_factor = 0;
            }

            return ret;
        }

        static tree_node_handler<tree_node>*
        rotate_left_right(tree_node* self, tree_node* child) {
            LDB_PROF_SCOPE("TreeRotate_LeftRight");
            assert(self->_left.get() == child);
            assert(child->balance_factor() > 0);

            auto inner_node = child->_right.get();
            auto new_parent = rotate_left(child, inner_node);
            assert(new_parent == self);
            std::ignore = new_parent;
            auto ret = rotate_right(self, inner_node);

            if (inner_node->_balance_factor == 0) {
                self->_balance_factor = 0;
                child->_balance_factor = 0;
            }
            else if (inner_node->_balance_factor > 0) {
                self->_balance_factor = -1;
                child->_balance_factor = 0;
            }
            else {
                self->_balance_factor = 0;
                child->_balance_factor = -1;
            }

            return ret;
        }

        static tree_node_handler<tree_node>*
        rotate_right_left(tree_node* self, tree_node* child) {
            LDB_PROF_SCOPE("TreeRotate_RightLeft");
            assert(self->_right.get() == child);
            assert(child->balance_factor() < 0);

            auto inner_node = child->_left.get();
            auto new_parent = rotate_right(child, inner_node);
            assert(new_parent == self);
            std::ignore = new_parent;
            auto ret = rotate_left(self, inner_node);

            if (inner_node->_balance_factor == 0) {
                self->_balance_factor = 0;
                child->_balance_factor = 0;
            }
            else if (inner_node->_balance_factor > 0) {
                self->_balance_factor = -1;
                child->_balance_factor = 0;
            }
            else {
                self->_balance_factor = 0;
                child->_balance_factor = -1;
            }

            return ret;
        }

        static tree_node_handler<tree_node>*
        right_rotate(tree_node* self, tree_node* child) {
            auto owned_self = self->_parent->replace_this_as_child(self, self->detach_left());
            owned_self->attach_left(child->detach_right());
            child->attach_right(std::move(owned_self));
            return child->_parent;
        }

        static tree_node_handler<tree_node>*
        left_rotate(tree_node* self, tree_node* child) {
            auto owned_self = self->_parent->replace_this_as_child(self, self->detach_right());
            owned_self->attach_right(child->detach_left());
            child->attach_left(std::move(owned_self));
            return child->_parent;
        }

        void
        increment_left() {
            --_balance_factor;
            if (_parent) _parent->increment_side_of_child(this);
        }

        void
        increment_right() {
            ++_balance_factor;
            if (_parent) _parent->increment_side_of_child(this);
        }

        template<class... Args>
        void
        add_left(Args&&... args) {
            LDB_PROF_SCOPE("TreeNode_AddLeft");
            _left = std::make_unique<tree_node<T>>(static_cast<tree_node_handler<tree_node>*>(this),
                                                   new_node_tag{},
                                                   std::forward<Args>(args)...);
            increment_left();
        }

        template<class... Args>
        void
        add_right(Args&&... args) {
            LDB_PROF_SCOPE("TreeNode_AddRight");
            _right = std::make_unique<tree_node<T>>(static_cast<tree_node_handler<tree_node>*>(this),
                                                    new_node_tag{},
                                                    std::forward<Args>(args)...);
            increment_right();
        }

        void
        insert_to_glb(const std::pair<key_type, value_type>& squished) {
            LDB_PROF_SCOPE("TreeNode_InsertToGlb");
            if (_left) return _left->insert_to_glb_descent(squished);
            add_left(squished.first, squished.second);
        }

        void
        insert_to_glb_descent(const std::pair<key_type, value_type>& squished) {
            if (!_right) {
                auto&& [key, value] = squished;
                if (auto succ = _payload.try_set(key, value);
                    succ) return;
                return add_right(squished.first, squished.second);
            }
            _right->insert_to_glb_descent(squished);
        }

        T _payload{};
        factor_type _balance_factor = 0;
        tree_node_handler<tree_node<T>>* _parent = nullptr;
        std::unique_ptr<tree_node<T>> _left = nullptr;
        std::unique_ptr<tree_node<T>> _right = nullptr;
    };
}

#endif
