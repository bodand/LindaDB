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
 * src/LindaDB/public/ldb/index/tree/tree_node_handler --
 *   The generic node handler interface. This allows the uniform handling of
 *   the root node, where the tree object behaves as the node handler instead of
 *   a real parent node.
 */
#ifndef LINDADB_TREE_NODE_HANDLER_HXX
#define LINDADB_TREE_NODE_HANDLER_HXX

#include <memory>

namespace ldb::index::tree {
    template<class NodeT>
    struct tree_node_handler {
        tree_node_handler() = default;
        tree_node_handler(const tree_node_handler& cp) = delete;
        tree_node_handler(tree_node_handler&& mv) noexcept = delete;
        tree_node_handler&
        operator=(const tree_node_handler& cp) = delete;
        tree_node_handler&
        operator=(tree_node_handler&& mv) noexcept = delete;
        virtual ~tree_node_handler() noexcept = default;

        virtual std::unique_ptr<NodeT>
        replace_this_as_child(NodeT* old, std::unique_ptr<NodeT>&& new_) = 0;

        virtual std::unique_ptr<NodeT>
        detach_left() = 0;

        virtual std::unique_ptr<NodeT>
        detach_right() = 0;

        virtual void
        attach_left(std::unique_ptr<NodeT> left) = 0;

        virtual void
        attach_right(std::unique_ptr<NodeT> right) = 0;

        virtual void
        increment_side_of_child(NodeT* child) = 0;

        virtual NodeT*
        downcast_to_node() noexcept { return nullptr; }
    };
}

#endif
