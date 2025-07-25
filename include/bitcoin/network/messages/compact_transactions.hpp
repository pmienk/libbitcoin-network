/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_NETWORK_MESSAGES_COMPACT_TRANSACTIONS_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_COMPACT_TRANSACTIONS_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

// TODO: build witness into feature w/magic and negotiated version.
struct BCT_API compact_transactions
{
    typedef std::shared_ptr<const compact_transactions> cptr;

    static const identifier id;
    static const std::string command;
    static const uint32_t version_minimum;
    static const uint32_t version_maximum;

    static cptr deserialize(uint32_t version,
        const system::data_chunk& data, bool witness=true) NOEXCEPT;
    static compact_transactions deserialize(uint32_t version,
        system::reader& source, bool witness=true) NOEXCEPT;

    bool serialize(uint32_t version,
        const system::data_slab& data, bool witness=true) const NOEXCEPT;
    void serialize(uint32_t version, system::writer& sink,
        bool witness=true) const NOEXCEPT;

    size_t size(uint32_t version, bool witness=true) const NOEXCEPT;

    system::hash_digest block_hash;
    system::chain::transaction_cptrs transaction_ptrs;
};

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
