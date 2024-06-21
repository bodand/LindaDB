/* LindaDB project
 *
 * Copyright (c) 2024 András Bodor <bodand@pm.me>
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
 * Originally created: 2024-04-04.
 *
 * src/LindaDB/public/ldb/simple_simple_store --
 *   
 */
#ifndef LINDADB_SIMPLE_STORE_HXX
#define LINDADB_SIMPLE_STORE_HXX

#include <optional>

#include <ldb/index/tree/impl/avl2/avl2_tree.hxx>
#include <ldb/index/tree/index_query.hxx>
#include <ldb/index/tree/payload/vectorset_payload.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/lv/linda_value.hxx>
#include <ldb/query.hxx>

namespace ldb {
    struct simple_store {
        using value_type = lv::linda_tuple;
        using storage_type = index::tree::avl2_tree<value_type,
                                                    value_type,
                                                    16,
                                                    index::tree::payloads::vectorset_payload<value_type, 16>>;
        using pointer_type = value_type*;
        using query_type = tuple_query<storage_type>;

        static auto
        indices() noexcept {
            return ::ldb::over_index<storage_type>;
        }

        void
        insert(const lv::linda_tuple& tuple) {
            LDBT_ZONE_A;
            _storage.insert(tuple);
            notify_readers();
        }

        std::optional<lv::linda_tuple>
        try_read(const query_type& query) const {
            LDBT_ZONE_A;
            return retrieve_weak(query,
                                 std::mem_fn(&simple_store::perform_read));
        }

        template<class... Args>
        std::optional<lv::linda_tuple>
        try_read(Args&&... args)
            requires((
                   (lv::is_linda_value_v<std::remove_cvref_t<Args>>
                    || meta::is_matcher_type_v<Args>)
                   && ...))
        {
            return try_read(make_piecewise_query(indices(), std::forward<Args>(args)...));
        }

        lv::linda_tuple
        read(const query_type& query) const {
            LDBT_ZONE_A;
            return retrieve_strong(query,
                                   std::mem_fn(&simple_store::perform_read));
        }

        template<class... Args>
        lv::linda_tuple
        read(Args&&... args)
            requires((
                   (lv::is_linda_value_v<std::remove_cvref_t<Args>>
                    || meta::is_matcher_type_v<Args>)
                   && ...))
        {
            return read(make_piecewise_query(indices(), std::forward<Args>(args)...));
        }

        std::optional<lv::linda_tuple>
        try_remove(const query_type& query) {
            LDBT_ZONE_A;
            return retrieve_weak(query,
                                 std::mem_fn(&simple_store::perform_remove));
        }

        template<class... Args>
        std::optional<lv::linda_tuple>
        try_remove(Args&&... args)
            requires((
                   (lv::is_linda_value_v<std::remove_cvref_t<Args>>
                    || meta::is_matcher_type_v<Args>)
                   && ...))
        {
            return try_remove(make_piecewise_query(indices(), std::forward<Args>(args)...));
        }

        lv::linda_tuple
        remove(const query_type& query) {
            LDBT_ZONE_A;
            return retrieve_strong(query,
                                   std::mem_fn(&simple_store::perform_remove));
        }

        template<class... Args>
        lv::linda_tuple
        remove(Args&&... args)
            requires((
                   (lv::is_linda_value_v<std::remove_cvref_t<Args>>
                    || meta::is_matcher_type_v<Args>)
                   && ...))
        {
            return remove(make_piecewise_query(indices(), std::forward<Args>(args)...));
        }

        void
        dump(std::ostream& os) const {
            _storage.apply([&os](auto& node) {
                os << node << "\n";
            });
        }

    private:
        std::optional<value_type>
        perform_read(const query_type& query) const {
            return _storage.search_query(query);
        }

        std::optional<value_type>
        perform_remove(const query_type& query) {
            return _storage.remove_query(query);
        }

        template<class Extractor>
        lv::linda_tuple
        retrieve_strong(const query_type& query,
                        Extractor&& extractor)
            requires(std::invocable<Extractor, simple_store, decltype(query)>)
        {
            LDBT_ZONE_A;
            for (;;) {
                if (auto read = std::forward<Extractor>(extractor)(this, query)) return *read;
                if (check_and_reset_sync_need()) continue;
                wait_for_sync();
            }
        }

        template<class Extractor>
        lv::linda_tuple
        retrieve_strong(const query_type& query,
                        Extractor&& extractor) const
            requires(std::invocable<Extractor, const simple_store, decltype(query)>)
        {
            LDBT_ZONE_A;
            for (;;) {
                if (auto read = std::forward<Extractor>(extractor)(this, query)) return *read;
                if (check_and_reset_sync_need()) continue;
                wait_for_sync();
            }
        }

        template<class Extractor, class T = lv::linda_tuple>
        [[nodiscard]] std::optional<T>
        retrieve_weak(const query_type& query,
                      Extractor&& extractor)
            requires(std::invocable<Extractor, simple_store, decltype(query)>)
        {
            LDBT_ZONE_A;
            return std::forward<Extractor>(extractor)(this, query);
        }

        template<class Extractor, class T = lv::linda_tuple>
        [[nodiscard]] std::optional<T>
        retrieve_weak(const query_type& query,
                      Extractor&& extractor) const
            requires(std::invocable<Extractor, const simple_store, decltype(query)>)
        {
            LDBT_ZONE_A;
            return std::forward<Extractor>(extractor)(this, query);
        }

        void
        wait_for_sync() const {
            LDBT_ZONE_A;
            LDBT_UQ_LOCK(_read_mtx);
            auto sync_check_over_this = [this]() noexcept {
                return check_sync_need();
            };
            _wait_read.wait(lck, sync_check_over_this);
        }

        void
        notify_readers() const noexcept {
            mark_sync_need();
            notify_sync_start();
        }

        void
        notify_sync_start() const noexcept {
            _wait_read.notify_one();
        }

        [[gnu::always_inline]] [[nodiscard]] bool
        check_and_reset_sync_need() const noexcept {
            return _sync_needed.exchange(0, std::memory_order::acq_rel) > 0;
        }

        [[gnu::always_inline]] [[nodiscard]] bool
        check_sync_need() const noexcept {
            return _sync_needed.load(std::memory_order::acquire) > 0;
        }

        void
        mark_sync_need() const noexcept {
            _sync_needed.fetch_add(1, std::memory_order::acq_rel);
        }

        storage_type _storage{};

        mutable std::atomic<int> _sync_needed = 0;
        mutable LDBT_MUTEX(_read_mtx);
        mutable LDBT_CV(_wait_read);
    };
}

#endif
