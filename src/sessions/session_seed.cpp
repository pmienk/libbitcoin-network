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
#include <bitcoin/network/sessions/session_seed.hpp>

#include <functional>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocols.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_seed

using namespace system;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

session_seed::session_seed(p2p& network, size_t key) NOEXCEPT
  : session(network, key), tracker<session_seed>(network.log())
{
}

bool session_seed::inbound() const NOEXCEPT
{
    return false;
}

bool session_seed::notify() const NOEXCEPT
{
    return false;
}

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_seed::start(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Seeding is allowed even with !enable_address configured.

    if (is_zero(settings().outbound_connections) ||
        is_zero(settings().connect_batch_size))
    {
        LOG("Bypassed seeding because outbound connections disabled.");
        handler(error::bypassed);
        return;
    }

    if (address_count() >= settings().minimum_address_count())
    {
        LOG("Bypassed seeding because of sufficient ("
            << address_count() << " of " << settings().minimum_address_count()
            << ") address quantity.");
        handler(error::bypassed);
        return;
    }

    if (is_zero(settings().host_pool_capacity))
    {
        LOG("Cannot seed because no address pool capacity configured.");
        handler(error::seeding_unsuccessful);
        return;
    }

    if (settings().seeds.empty())
    {
        LOG("Cannot seed because no seeds configured");
        handler(error::seeding_unsuccessful);
        return;
    }

    LOG("Seeding because of insufficient ("
        << address_count() << " of " << settings().minimum_address_count()
        << ") address quantity.");

    session::start(BIND2(handle_started, _1, std::move(handler)));
}

void session_seed::handle_started(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Seeding runs entirely in start and is possible to stop.
    ////BC_ASSERT_MSG(!stopped(), "session stopped in start");

    if (ec)
    {
        handler(ec);
        return;
    }

    // Create a connector for each seed connection.
    const auto connectors = create_connectors(settings().seeds.size());
    auto it = settings().seeds.begin();
    count_ = connectors->size();
    handled_ = false;

    for (const auto& connector: *connectors)
    {
        const auto& seed = *(it++);

        subscribe_stop([=](const code&) NOEXCEPT
        {
            connector->stop();
            return false;
        });

        start_seed(error::success, seed, connector,
            BIND4(handle_connect, _1, _2, seed, handler));
    }
}

// Seed sequence.
// ----------------------------------------------------------------------------

// Attempt to connect one seed.
void session_seed::start_seed(const code&, const config::endpoint& seed,
    const connector::ptr& connector, const channel_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    LOG("Connecting to seed [" << seed << "]");

    // Guard restartable connector (shutdown delay).
    if (stopped())
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    connector->connect(seed, move_copy(handler));
}

void session_seed::handle_connect(const code& ec, const channel::ptr& channel,
    const config::endpoint& LOG_ONLY(seed), 
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        LOG("Failed to connect seed channel [" << seed << "] " << ec.message());
        BC_ASSERT_MSG(!channel, "unexpected channel instance");
        stop_seed(handler);
        return;
    }

    start_channel(channel,
        BIND2(handle_channel_start, _1, channel),
        BIND3(handle_channel_stop, _1, channel, handler));
}

void session_seed::attach_handshake(const channel::ptr& channel,
    result_handler&& handler) const NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for attach");

    // Tx relay is parsed (by version) but always set to false for seeding.
    constexpr auto enable_transaction = false;

    // Seeding does not require or provide any node services.
    // Nodes that require inbound connection services/txs will not accept.
    constexpr auto minimum_services = messages::service::node_none;
    constexpr auto maximum_services = messages::service::node_none;

    // Weak reference safe as sessions outlive protocols.
    const auto& self = *this;
    const auto maximum_version = settings().protocol_maximum;
    const auto extended_version = maximum_version >= messages::level::bip37;
    const auto enable_reject = settings().enable_reject &&
        maximum_version >= messages::level::bip61;

    // Protocol must pause the channel after receiving version and verack.

    // Reject is deprecated.
    if (enable_reject)
        channel->attach<protocol_version_70002>(self, minimum_services,
            maximum_services, enable_transaction)->shake(std::move(handler));

    else if (extended_version)
        channel->attach<protocol_version_70001>(self, minimum_services,
            maximum_services, enable_transaction)->shake(std::move(handler));

    else
        channel->attach<protocol_version_31402>(self, minimum_services,
            maximum_services)->shake(std::move(handler));
}

void session_seed::handle_channel_start(const code& LOG_ONLY(ec),
    const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        LOG("Failed to start seed channel [" << channel->authority() << "] "
            << ec.message());
    }

    // Pend channel for seed duration (for quick stop).
    // This immediately follows the handshake unpend of the same channel.
    pend(channel);
}

void session_seed::attach_protocols(const channel::ptr& channel) const NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");

    // Weak reference safe as sessions outlive protocols.
    const auto& self = *this;
    const auto enable_alert = settings().enable_alert;
    const auto negotiated_version = channel->negotiated_version();
    const auto enable_pong = negotiated_version >= messages::level::bip31;
    const auto enable_reject = settings().enable_reject &&
        negotiated_version >= messages::level::bip61;

    if (enable_pong)
        channel->attach<protocol_ping_60001>(self)->start();
    else
        channel->attach<protocol_ping_31402>(self)->start();

    // Alert is deprecated.
    if (enable_alert)
        channel->attach<protocol_alert_31402>(self)->start();

    // Reject is deprecated.
    if (enable_reject)
        channel->attach<protocol_reject_70002>(self)->start();

    // Seed protocol stops upon completion, causing session removal.
    channel->attach<protocol_seed_31402>(self)->start();
}

void session_seed::handle_channel_stop(const code& ec,
    const channel::ptr& channel, const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    LOG("Seed channel stop [" << channel->authority() << "] " << ec.message());

    if (!unpend(channel))
    {
        LOG("Unpend failed to locate seed channel.");
    }

    stop_seed(handler);
}

void session_seed::stop_seed(const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (is_zero(--count_))
    {
        LOG("Seed session closed.");
        unsubscribe_close();
    }

    if (!handled_)
    {
        if (stopped())
        {
            handler(error::service_stopped);
            handled_ = true;
        }
        else if (address_count() >= settings().minimum_address_count())
        {
            handler(error::success);
            handled_ = true;
        }
        else if (is_zero(count_))
        {
            handler(error::seeding_unsuccessful);
            handled_ = true;
        }
    }
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
