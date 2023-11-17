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
 * src/LindaDB/public/ldb/index/tree/impl/avl/avl_node --
 *   A node in an AVL tree, used in the implementation of avl_tree. This structure
 */
#ifndef AVL_NODE_HXX
#define AVL_NODE_HXX

#include <ldb/index/tree/payload_dispatcher.hxx>

namespace ldb::index::tree {
    /**
     * \brief Node type for the AVL tree implementation.
     *
     * \tparam P The payload type of the AVL tree node
     */
    template<payload P>
    struct avl_node final {
        using factor_type = std::int8_t;
        using key_type = typename P::key_type;
        using value_type = typename P::value_type;
        using size_type = typename P::size_type;

        avl_node() = default;

        avl_node(const avl_node& other) = delete;
        avl_node(avl_node&& other) noexcept
             : balance_factor(other.balance_factor),
               parent(other.parent),
               left(std::move(other.left)),
               right(std::move(other.right)),
               _payload(std::move(other._payload)) { }

        avl_node&
        operator=(const avl_node& other) = delete;
        avl_node&
        operator=(avl_node&& other) noexcept {
            if (this == &other)
                return *this;
            balance_factor = other.balance_factor;
            parent = other.parent;
            left = std::move(other.left);
            right = std::move(other.right);
            _payload = std::move(other._payload);
            return *this;
        }

        explicit
        avl_node(const P& payload)
             : _payload(payload) { }

        template<class... Args>
        explicit avl_node(std::unique_ptr<avl_node>* parent, Args&&... payload_args)
             : parent(parent),
               _payload(std::forward<Args>(payload_args)...) { }

        // clang-format off
        ~avl_node() { // clang-format on
            while (left) release(&left);
            while (right) release(&right);
        }

        template<class K>
        auto
        compare_to_key(const K& key) const noexcept(noexcept(key <=> _payload)) {
            return key <=> _payload;
        }

        template<index_query<value_type> Q>
        auto
        find_by_query(const Q& query) const {
            return _payload.try_get(query);
        }

        template<index_query<value_type> Q>
        auto
        remove_by_query(const Q& query) {
            return _payload.remove(query);
        }

        auto
        insert_into_lower(const key_type& key,
                          const value_type& value) {
            return _payload.force_set_lower(key, value);
        }

        std::optional<typename P::bundle_type>
        insert_into_upper(const key_type& key,
                          const value_type& value) {
            if (_payload.try_set(key, value)) return {};
            return _payload.force_set_upper(key, value);
        }

        [[nodiscard]] size_type
        empty() const noexcept {
            return _payload.empty();
        }

        [[nodiscard]] std::unique_ptr<avl_node>
        release_child(std::unique_ptr<avl_node>& node,
                      std::unique_ptr<avl_node>&& replacement = std::unique_ptr<avl_node>{}) {
            assert(node->left == nullptr);
            assert(node->right == nullptr);
            if (left == node) return std::exchange(left, std::move(replacement));
            if (right == node) return std::exchange(right, std::move(replacement));
            assert(false && "non-child passed to release_child");
            LDB_UNREACHABLE;
        }

        std::unique_ptr<avl_node>* parent{};
        factor_type balance_factor = 0;
        std::unique_ptr<avl_node> left = nullptr;
        std::unique_ptr<avl_node> right = nullptr;

    private:
        static void
        release(std::unique_ptr<avl_node>* subtree) noexcept {
            LDB_PROF_SCOPE("TreeNode_ReleaseSubtree");
            while ((*subtree)->left && (*subtree)->right) {
                subtree = &(*subtree)->left;
            }

            if ((*subtree)->left) {
                *subtree = std::move((*subtree)->left);
            }
            else if ((*subtree)->right) {
                *subtree = std::move((*subtree)->right);
            }
            else {
                (*subtree).reset();
            }
        }

        P _payload{};
    };
}

#endif
