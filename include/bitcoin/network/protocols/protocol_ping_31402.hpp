/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_PING_31402_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_PING_31402_HPP

#include <memory>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>

namespace libbitcoin {
namespace network {

class session;

class BCT_API protocol_ping_31402
  : public protocol, track<protocol_ping_31402>
{
public:
    typedef std::shared_ptr<protocol_ping_31402> ptr;

    protocol_ping_31402(const session& session,
        const channel::ptr& channel) noexcept;

    /// Start protocol (strand required).
    void start() noexcept override;

    /// The channel is stopping (called on strand by stop subscription).
    void stopping(const code& ec) noexcept override;

protected:
    const std::string& name() const noexcept override;

    virtual void send_ping() noexcept;
    virtual void handle_timer(const code& ec) noexcept;
    virtual void handle_send_ping(const code& ec) noexcept;
    virtual void handle_receive_ping(const code& ec,
        const messages::ping::ptr& message) noexcept;

private:
    // This is protected by strand.
    deadline::ptr timer_;
};

} // namespace network
} // namespace libbitcoin

#endif
