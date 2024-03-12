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
 * Originally created: 2024-01-10.
 *
 * test/LindaDB/bcast/broadcast --
 *   Tests for the broadcast type-erasure containers.
 *   -Wself-move is disabled for this file, because we check self-move's
 *   behavior, in case, outside code decides to be mischievous
 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"

//#include <utility>
//
//#include <catch2/catch_test_macros.hpp>
//#include <ldb/bcast/broadcast.hxx>
//#include <ldb/bcast/null_broadcast.hxx>
//#include <ldb/lv/linda_tuple.hxx>
//
//TEST_CASE("await_if can be awaited") {
//    const ldb::broadcast_awaitable await_bcast = ldb::null_awaiter{};
//    await(await_bcast);
//}
//
//TEST_CASE("default constructed await_if can be awaited, is nop") {
//    const ldb::broadcast_awaitable await_bcast;
//    await(await_bcast);
//}
//
//TEST_CASE("await_if can be move constructed") {
//    ldb::broadcast_awaitable await_bcast = ldb::null_awaiter{};
//    const ldb::broadcast_awaitable await_bcast2 = std::move(await_bcast);
//    await(await_bcast2);
//}
//
//TEST_CASE("await_if can be move assigned") {
//    ldb::broadcast_awaitable await_bcast = ldb::null_awaiter{};
//    ldb::broadcast_awaitable await_bcast2;
//    await_bcast2 = std::move(await_bcast);
//    await(await_bcast2);
//}
//
//TEST_CASE("await_if handles self-assigment") {
//    ldb::broadcast_awaitable await_bcast = ldb::null_awaiter{};
//    await_bcast = std::move(await_bcast);
//    await(await_bcast);
//}
//
//TEST_CASE("broadcast_if can broadcast insert") {
//    const ldb::broadcast bcast = ldb::null_broadcast{};
//    await(broadcast_insert(bcast, ldb::lv::linda_tuple{}));
//}
//
//TEST_CASE("broadcast_if can broadcast delete") {
//    const ldb::broadcast bcast = ldb::null_broadcast{};
//    await(broadcast_delete(bcast, ldb::lv::linda_tuple{}));
//}
//
//TEST_CASE("default constructed broadcast_if can be called, is nop") {
//    const ldb::broadcast bcast;
//    broadcast_insert(bcast, ldb::lv::linda_tuple{});
//    broadcast_delete(bcast, ldb::lv::linda_tuple{});
//}
//
//TEST_CASE("broadcast can be move constructed") {
//    ldb::broadcast bcast = ldb::null_broadcast{};
//    const ldb::broadcast bcast2 = std::move(bcast);
//    await(broadcast_insert(bcast2, ldb::lv::linda_tuple{}));
//}
//
//TEST_CASE("broadcast can be move assigned") {
//    ldb::broadcast bcast = ldb::null_broadcast{};
//    ldb::broadcast bcast2;
//    bcast2 = std::move(bcast);
//    await(broadcast_insert(bcast2, ldb::lv::linda_tuple{}));
//}
//
//TEST_CASE("broadcast handles self-assignment") {
//    ldb::broadcast bcast = ldb::null_broadcast{};
//    bcast = std::move(bcast);
//    await(broadcast_insert(bcast, ldb::lv::linda_tuple{}));
//}
//
//#pragma clang diagnostic pop
