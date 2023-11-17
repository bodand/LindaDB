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
 * Originally created: 2023-11-17.
 *
 * src/LindaDB/public/ldb/index/tree/impl/avl_tree --
 *   Implementation of an AVL tree hosting data in a payload type.
 */
#ifndef AVL_TREE_HXX
#define AVL_TREE_HXX

#include <ldb/index/tree/impl/avl/avl_node.hxx>
#include <ldb/index/tree/index_query.hxx>
#include <ldb/index/tree/payload_dispatcher.hxx>
#include <ldb/profiler.hxx>

#include <spdlog/spdlog.h>

namespace ldb::index::tree {
    template<class K,
             class V,
             std::size_t Clustering = 0,
             class PayloadType = typename payload_dispatcher<K, V, Clustering>::type>
    struct avl_tree {
        using payload_type = PayloadType;
        using key_type = payload_type::key_type;
        using value_type = payload_type::value_type;


        template<index_query<value_type> Q>
        [[nodiscard]] std::optional<value_type>
        search(const Q& query) const {
            LDB_PROF_SCOPE_C("AVLTree_Search", prof::color_search);
            const auto* node = traverse_tree(query.key());
            if (!*node) return {};

            return (*node)->find_by_query(query);
        }

        void
        insert(const K& key, const V& value) {
            LDB_PROF_SCOPE_C("AVLTree_Insert", prof::color_insert);
            auto updated_parent = insert_into_tree(key, value);
            auto [parent, direction] = updated_parent;
            if (!parent) return;

            auto inserted = [direction, &parent] {
                if (direction == -1) return &(*parent)->left;
                return &(*parent)->right;
            }();

            do_insert_rotations(inserted, parent);
        }

        template<index_query<value_type> Q>
        std::optional<value_type>
        remove(const Q& query) {
            LDB_PROF_SCOPE_C("AVLTree_Remove", prof::color_remove);
            auto* node = traverse_tree(query.key());
            if (!*node) return {};

            auto elem = (*node)->remove_by_query(query);
            if (!elem) return {};

            if ((*node)->empty()) {
                auto parent = (*node)->parent;
                if (!(*node)->left && !(*node)->right) {
                    if (parent == nullptr) {
                        _root.reset(new node_type{});
                        node = &_root;
                    }
                    else {
                        std::ignore = (*parent)->release_child(*node);
                        node = parent;
                    }
                }
                else if (!(*node)->left && (*node)->right) {
                    auto new_node = std::unique_ptr<node_type>((*node)->right.release());
                    if (parent == nullptr) {
                        _root = std::move(new_node);
                        node = &_root;
                    }
                    else {
                        std::ignore = (*parent)->release_child(*node, std::move(new_node));
                        node = parent;
                    }
                }
                else if ((*node)->left && !(*node)->right) {
                    auto new_node = std::unique_ptr<node_type>((*node)->left.release());
                    if (parent == nullptr) {
                        _root = std::move(new_node);
                        node = &_root;
                    }
                    else {
                        std::ignore = (*parent)->release_child(*node, std::move(new_node));
                        node = parent;
                    }
                }
                else {
                    auto successor = min_child(&(*node)->right);
                    assert((*successor)->left == nullptr);
                    if ((*successor)->parent == node) {
                        auto right = std::move((*node)->right);
                        right->left = std::move((*node)->left);
                        if (parent == nullptr) {
                            _root = std::move(right);
                            node = &_root;
                        }
                        else {
                            std::ignore = (*parent)->release_child(*node, std::move(right));
                            node = parent;
                        }
                    }
                    else {
                        auto succ_parent = (*successor)->parent;
                        auto owned_succ = (*succ_parent)->release_child(*successor, std::move((*successor)->right));
                        owned_succ->right = std::move((*node)->right);
                        owned_succ->left = std::move((*node)->left);
                        if (parent == nullptr) {
                            _root = std::move(owned_succ);
                            node = &_root;
                        }
                        else {
                            std::ignore = (*parent)->release_child(*node, std::move(owned_succ));
                            node = parent;
                        }
                    }
                }
                do_delete_rotations(node);
            }

            return elem;
        }

    private:
        using node_type = avl_node<payload_type>;

        static std::unique_ptr<node_type>*
        min_child(std::unique_ptr<node_type>* node) {
            assert(node && "node must not be empty");
            assert(*node && "*node must not be empty");
            while ((*node)->left) {
                node = &(*node)->left;
            }
            return node;
        }

        void
        do_delete_rotations(std::unique_ptr<node_type>* subtree) {
            std::unique_ptr<node_type>* rotation_memory = nullptr;
            std::unique_ptr<node_type> root_memory = nullptr;
            typename node_type::factor_type bf{};
            for (auto parent = (*subtree)->parent; parent != nullptr; parent = rotation_memory) {
                rotation_memory = (*parent)->parent;
                if ((*parent)->left == *subtree) {
                    if ((*parent)->balance_factor > 0) {
                        auto sibling = &(*parent)->right;
                        bf = (*sibling)->balance_factor;
                        if (bf > 0) {
                            root_memory = rotate_right_left(parent, *sibling);
                        }
                        else {
                            root_memory = rotate_left(parent, *sibling);
                        }
                    }
                    else if ((*parent)->balance_factor < 0) {
                        subtree = parent;
                        (*subtree)->balance_factor = 0;
                        continue;
                    }
                    else {
                        (*parent)->balance_factor = 1;
                        break;
                    }
                }
                else {
                    if ((*parent)->balance_factor < 0) {
                        auto sibling = &(*parent)->left;
                        bf = (*sibling)->balance_factor;
                        if (bf > 0) {
                            root_memory = rotate_left_right(parent, *sibling);
                        }
                        else {
                            root_memory = rotate_right(parent, *sibling);
                        }
                    }
                    else if ((*parent)->balance_factor > 0) {
                        subtree = parent;
                        (*subtree)->balance_factor = 0;
                        continue;
                    }
                    else {
                        (*parent)->balance_factor = -1;
                        break;
                    }
                }

                root_memory->parent = rotation_memory;
                if (rotation_memory != nullptr) {
                    if ((*rotation_memory)->left == *parent) {
                        (*rotation_memory)->left = std::move(root_memory);
                    }
                    else {
                        (*rotation_memory)->right = std::move(root_memory);
                    }
                }
                else {
                    _root = std::move(root_memory);
                }

                if (bf == 0) break;
            }
        }

        void
        do_insert_rotations(std::unique_ptr<node_type>* inserted,
                            std::unique_ptr<node_type>* parent) {
            std::unique_ptr<node_type>* rotation_memory = nullptr;
            std::unique_ptr<node_type> root_memory = nullptr;
            for (; *parent == nullptr; parent = (*inserted)->parent) {
                auto& parent_balance = (*parent)->balance_factor;
                if (*inserted == (*parent)->right) {
                    if (parent_balance > 0) {
                        rotation_memory = (*inserted)->parent;
                        if ((*inserted)->balance_factor < 0) {
                            root_memory = rotate_right_left(parent, *inserted);
                        }
                        else {
                            root_memory = rotate_left(parent, *inserted);
                        }
                    }
                    else if (parent_balance < 0) {
                        parent_balance = 0;
                        break;
                    }
                    else {
                        parent_balance = 1;
                        inserted = parent;
                        continue;
                    }
                }
                else {
                    if (parent_balance < 0) {
                        rotation_memory = (*inserted)->parent;
                        if ((*inserted)->balance_factor > 0) {
                            root_memory = rotate_left_right(parent, *inserted);
                        }
                        else {
                            root_memory = rotate_right(parent, *inserted);
                        }
                    }
                    else if (parent_balance > 0) {
                        parent_balance = 0;
                        break;
                    }
                    else {
                        parent_balance = -1;
                        inserted = parent;
                        continue;
                    }
                }

                root_memory->parent = rotation_memory;
                if (*rotation_memory) {
                    if (*parent == (*rotation_memory)->left) {
                        (*rotation_memory)->left = std::move(root_memory);
                    }
                    else {
                        (*rotation_memory)->right = std::move(root_memory);
                    }
                }
                else {
                    _root = std::move(root_memory);
                }
                break;
            }
        }

        std::unique_ptr<node_type>
        rotate_left(
               std::unique_ptr<node_type>* parent,
               const std::unique_ptr<node_type>& right) {
            assert(parent);
            assert(*parent);
            assert(right);
            assert((*parent)->right == right);
            assert((*parent)->balance_factor == 1);
            assert(right->balance_factor >= 0);
            assert(right->balance_factor < 2);

            auto owned_parent_right = std::move((*parent)->right);
            assert(owned_parent_right == right);

            parent = perform_left_rotation(parent, owned_parent_right);

            (*parent)->balance_factor = 1 * (owned_parent_right->balance_factor == 0);
            owned_parent_right->balance_factor = -1 * (owned_parent_right->balance_factor == 0);

            assert((*parent)->balance_factor >= 0);
            assert((*parent)->balance_factor < 2);
            assert(owned_parent_right->balance_factor <= 0);
            assert(owned_parent_right->balance_factor > -2);
            return owned_parent_right;
        }

        std::unique_ptr<node_type>
        rotate_right(
               std::unique_ptr<node_type>* parent,
               std::unique_ptr<node_type>& left) {
            assert(parent);
            assert(*parent);
            assert(left);
            assert((*parent)->left == left);
            assert((*parent)->balance_factor == -1);
            assert(left->balance_factor <= 0);
            assert(left->balance_factor > -2);

            auto owned_parent_left = std::move((*parent)->left);
            assert(owned_parent_left == left);

            parent = perform_right_rotation(parent, owned_parent_left);

            (*parent)->balance_factor = 1 * (owned_parent_left->balance_factor == 0);
            owned_parent_left->balance_factor = -1 * (owned_parent_left->balance_factor == 0);

            assert((*parent)->balance_factor <= 0);
            assert((*parent)->balance_factor > -2);
            assert(owned_parent_left->balance_factor >= 0);
            assert(owned_parent_left->balance_factor < 2);
            return owned_parent_left;
        }

        std::unique_ptr<node_type>
        rotate_left_right(
               std::unique_ptr<node_type>* parent,
               std::unique_ptr<node_type>& left) {
            assert(parent);
            assert(*parent);
            assert(left);
            assert((*parent)->left == left);
            assert((*parent)->balance_factor == -1);
            assert(left->balance_factor == 1);

            auto inner_node = std::move(left->right);
            auto left_ptr = &left;
            perform_left_rotation(left_ptr, inner_node);
            auto new_inner_node = left->parent;
            parent = perform_right_rotation(parent, *new_inner_node);

            (*parent)->balance_factor = -1 * (inner_node->balance_factor > 0);
            left->balance_factor = 1 * (inner_node->balance_factor < 0);
            inner_node->balance_factor = 0;

            assert(inner_node->balance_factor == 0);
            assert(inner_node->left->balance_factor <= 0);
            assert(inner_node->left->balance_factor > -2);
            assert(inner_node->right->balance_factor >= 0);
            assert(inner_node->right->balance_factor < 2);
            return inner_node;
        }

        std::unique_ptr<node_type>
        rotate_right_left(
               std::unique_ptr<node_type>* parent,
               std::unique_ptr<node_type>& right) {
            assert(parent);
            assert(*parent);
            assert(right);
            assert((*parent)->right == right);
            assert((*parent)->balance_factor == 1);
            assert(right->balance_factor == -1);

            auto inner_node = std::move(right->left);
            auto right_ptr = &right;
            perform_right_rotation(right_ptr, inner_node);
            auto new_inner_node = right->parent;
            parent = perform_left_rotation(parent, *new_inner_node);

            (*parent)->balance_factor = -1 * (inner_node->balance_factor > 0);
            right->balance_factor = 1 * (inner_node->balance_factor < 0);
            inner_node->balance_factor = 0;

            assert(inner_node->balance_factor == 0);
            assert(inner_node->left->balance_factor <= 0);
            assert(inner_node->left->balance_factor > -2);
            assert(inner_node->right->balance_factor >= 0);
            assert(inner_node->right->balance_factor < 2);
            return inner_node;
        }

        std::unique_ptr<node_type>*
        perform_left_rotation(std::unique_ptr<node_type>* const& parent,
                              std::unique_ptr<node_type>& right_child) {
            (*parent)->right = std::move(right_child->left);
            right_child->left = std::move(*parent);
            return &right_child->left;
        }

        std::unique_ptr<node_type>*
        perform_right_rotation(std::unique_ptr<node_type>* const& parent,
                               std::unique_ptr<node_type>& left_child) {
            (*parent)->left = std::move(left_child->right);
            left_child->right = std::move(*parent);
            return &left_child->right;
        }

        const std::unique_ptr<node_type>*
        traverse_tree(const key_type& key) const {
            auto* node = &_root;
            assert(*node);

            for (;;) {
                if (*node == nullptr) return node;
                auto dir = (*node)->compare_to_key(key);
                if (dir < 0) {
                    node = &(*node)->left;
                    continue;
                }
                if (dir > 0) {
                    node = &(*node)->right;
                    continue;
                }
                return node;
            }
        }

        std::unique_ptr<node_type>*
        traverse_tree(const key_type& key) {
            auto* node = &_root;
            assert(*node);

            for (;;) {
                if (*node == nullptr) return node;
                auto dir = (*node)->compare_to_key(key);
                if (dir < 0) {
                    node = &(*node)->left;
                    continue;
                }
                if (dir > 0) {
                    node = &(*node)->right;
                    continue;
                }
                return node;
            }
        }

        std::pair<std::unique_ptr<node_type>*, int>
        insert_into_tree(const key_type& key,
                         const value_type& value) {
            auto* node = &_root;
            assert(*node);

            for (;;) {
                auto dir = (*node)->compare_to_key(key);
                if (dir < 0) {
                    auto left = &(*node)->left;
                    if (*left == nullptr) return insert_with_node_to(node,
                                                                     &node_type::left,
                                                                     key,
                                                                     value);
                    node = left;
                    continue;
                }
                if (dir > 0) {
                    auto right = &(*node)->right;
                    if (*right == nullptr) return insert_with_node_to(node,
                                                                      &node_type::right,
                                                                      key,
                                                                      value);
                    node = right;
                    continue;
                }
                return insert_into_node(key, value, node);
            }
        }

        std::pair<std::unique_ptr<node_type>*, int>
        insert_into_node(const key_type& key, const value_type& value, auto*& node) {
            auto maybe_bundle = (*node)->insert_into_lower(key, value);
            if (!maybe_bundle) return {nullptr, 0};

            auto left = &(*node)->left;
            if (!*left) return insert_with_node_to(node,
                                                   &node_type::left,
                                                   *maybe_bundle);
            node = left;

            while ((*node)->right) {
                node = &(*node)->right;
            }

            if (auto maybe_upper_bundle = (*node)->insert_into_upper(key, value);
                maybe_upper_bundle) { // couldn't fit in node
                return insert_with_node_to(node,
                                           &node_type::right,
                                           *maybe_upper_bundle);
            }
            return {nullptr, 0};
        }

        template<class... Args>
        std::pair<std::unique_ptr<node_type>*, int>
        insert_with_node_to(std::unique_ptr<node_type>* node,
                            std::unique_ptr<node_type> node_type::*side,
                            Args&&... args) {
            auto child = &(**node.*side);
            assert(!*child);
            *child = std::make_unique<node_type>(node,
                                                 std::forward<Args>(args)...);
            return {node, 1 - 2 * (side == &node_type::left)};
        }

        std::unique_ptr<node_type> _root{std::make_unique<node_type>()};
    };
}

#endif
