/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_PING_60001_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_PING_60001_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_ping_31402.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

class BCT_API protocol_ping_60001
  : public protocol_ping_31402, protected tracker<protocol_ping_60001>
{
public:
    typedef std::shared_ptr<protocol_ping_60001> ptr;

    protocol_ping_60001(session& session,
        const channel::ptr& channel) NOEXCEPT;

    /// Start protocol (strand required).
    void start() NOEXCEPT override;

protected:
    void send_ping() NOEXCEPT override;
    void handle_timer(const code& ec) NOEXCEPT override;
    bool handle_receive_ping(const code& ec,
        const messages::ping::cptr& message) NOEXCEPT override;

    virtual void handle_send_pong(const code& ec) NOEXCEPT;
    virtual bool handle_receive_pong(const code& ec,
        const messages::pong::cptr& message) NOEXCEPT;

private:
    // This is protected by strand.
    uint64_t nonce_;
};

} // namespace network
} // namespace libbitcoin

#endif
