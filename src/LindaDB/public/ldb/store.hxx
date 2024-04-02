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
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, REMOVECLUDREMOVEG, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. REMOVE NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, REMOVEDIRECT, REMOVECIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (REMOVECLUDREMOVEG, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSREMOVEESS REMOVETERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER REMOVE CONTRACT, STRICT LIABILITY,
 * OR TORT (REMOVECLUDREMOVEG NEGLIGENCE OR OTHERWISE) ARISREMOVEG REMOVE ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Originally created: 2023-10-18.
 *
 * src/LindaDB/public/ldb/store --
 *   
 */
#ifndef LREMOVEDADB_STORE_HXX
#define LREMOVEDADB_STORE_HXX

#include <algorithm>
#include <array>
#include <atomic>
#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <fstream>
#include <functional>
#include <mutex>
#include <optional>
#include <random>
#include <ranges>
#include <shared_mutex>
#include <span>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <variant>

#include <ldb/data/chunked_list.hxx>
#include <ldb/index/tree/impl/avl2/avl2_tree.hxx>
#include <ldb/index/tree/index_query.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/lv/linda_value.hxx>
#include <ldb/query.hxx>
#include <ldb/store_command/read_store_command.hxx>
#include <ldb/store_command/remove_store_command.hxx>

#include <mpi.h>

namespace ldb {
    struct store {
        using storage_type = data::chunked_list<lv::linda_tuple>;
        using pointer_type = storage_type::iterator;
        using query_type = tuple_query<index::tree::avl2_tree<lv::linda_value,
                                                              pointer_type>>;
        using index_type = index::tree::avl2_tree<lv::linda_value, pointer_type>;

        static auto
        indices() noexcept {
            return ::ldb::over_index<index_type>;
        }

        void
        out(const lv::linda_tuple& tuple) {
            std::scoped_lock<std::shared_mutex> lck(_header_mtx);
            auto new_it = _data.push_back(tuple);

            {
                //                std::scoped_lock<std::shared_mutex> lck(_header_mtx);
                for (std::size_t i = 0;
                     i < _header_indices.size() && i < tuple.size();
                     ++i) {
                    _header_indices[i].insert(tuple[i], new_it);
                }
            }

            notify_readers();
        }


        std::optional<lv::linda_tuple>
        rdp(const query_type& query) const {
            return retrieve_weak(query,
                                 std::mem_fn(&store::read));
        }

        lv::linda_tuple
        rd(const query_type& query) const {
            return retrieve_strong(query,
                                   std::mem_fn(&store::read));
        }

        std::optional<lv::linda_tuple>
        inp(const query_type& query) {
            return retrieve_weak(query,
                                 std::mem_fn(&store::read_and_remove));
        }

        lv::linda_tuple
        in(const query_type& query) {
            return retrieve_strong(query,
                                   std::mem_fn(&store::read_and_remove));
        }

        template<class... Args>
        std::optional<lv::linda_tuple>
        inp(Args&&... args)
            requires((
                   (lv::is_linda_value_v<std::remove_cvref_t<Args>>
                    || meta::is_matcher_type_v<Args>)
                   && ...))
        {
            return inp(make_piecewise_query(indices(), std::forward<Args>(args)...));
        }

        template<class... Args>
        lv::linda_tuple
        in(Args&&... args)
            requires((
                   (lv::is_linda_value_v<std::remove_cvref_t<Args>>
                    || meta::is_matcher_type_v<Args>)
                   && ...))
        {
            return in(make_piecewise_query(indices(), std::forward<Args>(args)...));
        }

        template<class... Args>
        std::optional<lv::linda_tuple>
        rdp(Args&&... args)
            requires((
                   (lv::is_linda_value_v<std::remove_cvref_t<Args>>
                    || meta::is_matcher_type_v<Args>)
                   && ...))
        {
            return rdp(make_piecewise_query(indices(), std::forward<Args>(args)...));
        }

        template<class... Args>
        lv::linda_tuple
        rd(Args&&... args)
            requires((
                   (lv::is_linda_value_v<std::remove_cvref_t<Args>>
                    || meta::is_matcher_type_v<Args>)
                   && ...))
        {
            return rd(make_piecewise_query(indices(), std::forward<Args>(args)...));
        }

        auto
        insert_nosignal(const lv::linda_tuple& tuple) {
            std::scoped_lock<std::shared_mutex> lck(_header_mtx);
            auto new_it = _data.push_back(tuple);

            {
                //                std::scoped_lock<std::shared_mutex> lck(_header_mtx);
                for (std::size_t i = 0;
                     i < _header_indices.size() && i < tuple.size();
                     ++i) {
                    _header_indices[i].insert(tuple[i], new_it);
                }
            }

            notify_readers();
        }

        auto
        remove_nosignal(const lv::linda_tuple& tuple) {
            auto mem = std::mem_fn(&store::prepare_remove);
            return retrieve_weak<decltype(mem), my_remove_command>(concrete_tuple_query<index_type>(tuple),
                                                                   std::move(mem));
        }

        void
        dump_indices(std::ostream& os) const {
            std::ranges::for_each(_header_indices, [&os](auto& index) {
                index.apply([&os](const auto& value) {
                    os << value << "\n";
                });
            });
        }

        void
        terminate() {
            _data.terminate();
        }

    private:
        // TODO(C++23): update retreive_(strong|weak) to use deducing this and remove duplication

        template<class Extractor>
        lv::linda_tuple
        retrieve_strong(const query_type& query,
                        Extractor&& extractor)
            requires(std::invocable<Extractor, store, decltype(query)>)
        {
            for (;;) {
                if (auto read = std::forward<Extractor>(extractor)(this, query)) return *read;
                std::ofstream("_msg.log", std::ios::app) << "?: FAILED RETRIEVE" << std::endl;
                if (check_and_reset_sync_need()) continue;
                std::ofstream("_msg.log", std::ios::app) << "?: WAITING ON INSERT" << std::endl;
                {
                    std::ofstream dump("_dump.log", std::ios::app);
                    dump_indices(dump);
                }
                wait_for_sync();
            }
        }

        template<class Extractor>
        lv::linda_tuple
        retrieve_strong(const query_type& query,
                        Extractor&& extractor) const
            requires(std::invocable<Extractor, const store, decltype(query)>)
        {
            for (;;) {
                if (auto read = std::forward<Extractor>(extractor)(this, query)) return *read;
                std::ofstream("_msg.log", std::ios::app) << "?: FAILED RETRIEVE" << std::endl;
                if (check_and_reset_sync_need()) continue;
                std::ofstream("_msg.log", std::ios::app) << "?: WAITING ON INSERT" << std::endl;
                {
                    std::ofstream dump("_dump.log", std::ios::app);
                    dump_indices(dump);
                }
                wait_for_sync();
            }
        }

        template<class Extractor, class T = lv::linda_tuple>
        [[nodiscard]] std::optional<T>
        retrieve_weak(const query_type& query,
                      Extractor&& extractor)
            requires(std::invocable<Extractor, store, decltype(query)>)
        {
            return std::forward<Extractor>(extractor)(this, query);
        }

        template<class Extractor, class T = lv::linda_tuple>
        [[nodiscard]] std::optional<T>
        retrieve_weak(const query_type& query,
                      Extractor&& extractor) const
            requires(std::invocable<Extractor, const store, decltype(query)>)
        {
            return std::forward<Extractor>(extractor)(this, query);
        }

        mutable std::mt19937 _rng{std::random_device()()};

        void
        wait_for_sync() const {
            std::uniform_int_distribution dist(100, 100'000);
            std::unique_lock<std::mutex> lck(_read_mtx);
            auto sync_check_over_this = [this]() noexcept {
                return check_sync_need();
            };
            using namespace std::literals;
            _wait_read.wait_for(lck, std::chrono::milliseconds(dist(_rng)), sync_check_over_this);
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


        using my_remove_command = remove_store_command<index_type, pointer_type>;
        using my_read_command = read_store_command<index_type, pointer_type>;

        std::optional<lv::linda_tuple>
        read(const query_type& query) const;

        std::pair<bool, std::optional<my_read_command>>
        read_one(std::size_t i, const query_type& query) const;

        std::optional<lv::linda_tuple>
        read_directly(const query_type& query) const;

        std::optional<lv::linda_tuple>
        read_and_remove(const query_type& query) {
            return remove_impl(query);
        }

        std::optional<my_remove_command>
        prepare_remove(const query_type& query);

        std::optional<lv::linda_tuple>
        remove_impl(const query_type& query);

        std::pair<bool, std::optional<my_remove_command>>
        remove_one(std::size_t i, const query_type& query);

        std::optional<lv::linda_tuple>
        remove_directly(const query_type& query);

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

        mutable std::atomic<int> _sync_needed = 0;
        mutable std::mutex _read_mtx;
        mutable std::condition_variable _wait_read;

        mutable std::shared_mutex _header_mtx;
        std::array<index_type, 1> _header_indices{};
        storage_type _data{};
    };
}

#endif
