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
#include <bitcoin/network/net/hosts.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace config;
using namespace messages;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

inline bool is_invalid(const address_item& host) NOEXCEPT
{
    return is_zero(host.port) || host.ip == null_ip_address;
}

hosts::hosts(const logger& log, const settings& settings) NOEXCEPT
  : file_path_(settings.path),
    count_(zero),
    minimum_(settings.address_minimum),
    maximum_(settings.address_maximum),
    capacity_(possible_narrow_cast<size_t>(settings.host_pool_capacity)),
    disabled_(is_zero(capacity_)),
    buffer_(capacity_),
    reporter(log)
{
}

size_t hosts::count() const NOEXCEPT
{
    return count_.load(std::memory_order_relaxed);
}

code hosts::start() NOEXCEPT
{
    if (disabled_)
        return error::success;

    try
    {
        ifstream file(file_path_, ifstream::in);
        if (!file.good())
        {
            LOG("Hosts file not found.");
            return error::success;
        }

        std::string line;
        while (std::getline(file, line))
        {
            const config::authority entry(line);
            const auto host = entry.to_address_item();

            if (!is_invalid(host))
                buffer_.push_back(host);
        }

        if (file.bad())
            return error::file_load;
    }
    catch (const std::exception&)
    {
        return error::file_load;
    }

    if (buffer_.empty())
    {
        code ec;
        std::filesystem::remove(file_path_, ec);
    }

    count_.store(buffer_.size(), std::memory_order_relaxed);
    return error::success;
}

code hosts::stop() NOEXCEPT
{
    if (disabled_)
        return error::success;

    if (buffer_.empty())
    {
        code ec;
        std::filesystem::remove(file_path_, ec);
        return ec ? error::file_save : error::success;
    }

    try
    {
        ofstream file(file_path_, ofstream::out);
        if (!file.good())
            return error::file_save;

        for (const auto& entry: buffer_)
            file << config::authority(entry) << std::endl;

        if (file.bad())
            return error::file_save;
    }
    catch (const std::exception&)
    {
        return error::file_save;
    }

    // Idempotent stop.
    disabled_ = true;

    buffer_.clear();
    count_.store(zero, std::memory_order_relaxed);
    return error::success;
}

void hosts::restore(const address_item& address) NOEXCEPT
{
    if (disabled_)
        return;

    // Do not treat invalid address as an error, just log it.
    if (is_invalid(address))
    {
        LOG("Invalid host address from peer.");
        return;
    }

    if (!exists(address))
    {
        buffer_.push_back(address);
        count_.store(buffer_.size(), std::memory_order_relaxed);
    }
}

void hosts::store(const messages::address::ptr& addresses) NOEXCEPT
{
    // If enabled then minimum capacity is one and buffer is at capacity.
    if (disabled_ || !addresses || addresses->addresses.empty())
        return;

    // Accept between 1 and all of this peer's addresses up to capacity.
    const auto& hosts = addresses->addresses;
    const auto usable = std::min(hosts.size(), capacity_);
    const auto random = pseudo_random::next(one, usable);

    // But always accept at least the amount we are short if available.
    const auto gap = capacity_ - buffer_.size();
    const auto accept = std::max(gap, random);

    // Convert minimum desired to nonzero step for iteration.
    const auto step = std::max(usable / accept, one);
    auto accepted = zero;

    // Push valid addresses into the buffer.
    for (size_t index = 0; index < usable; index = ceilinged_add(index, step))
    {
        const auto& host = hosts.at(index);

        if (is_invalid(host))
        {
            LOG("Invalid host address in peer set.");
            continue;
        }

        if (exists(host))
            continue;

        ++accepted;
        buffer_.push_back(host);
        count_.store(buffer_.size(), std::memory_order_relaxed);
    }

    LOG("Accepted (" << accepted << " of " << hosts.size() << ") "
        "host addresses from peer.");
}

void hosts::take(const address_item_handler& handler) NOEXCEPT
{
    if (buffer_.empty())
    {
        handler(error::address_not_found, {});
        return;
    }

    // Select address from random buffer position.
    const auto limit = sub1(buffer_.size());
    const auto index = pseudo_random::next(zero, limit);
    const auto it = std::next(buffer_.begin(), index);
    const auto host = *it;

    // Remove from the buffer.
    buffer_.erase(it);
    count_.store(buffer_.size(), std::memory_order_relaxed);
    handler(error::success, host);
}

void hosts::fetch(const address_items_handler& handler) const NOEXCEPT
{
    if (buffer_.empty())
    {
        handler(error::address_not_found, {});
        return;
    }

    // Vary the return count (quantity fingerprinting).
    const auto divide = pseudo_random::next<size_t>(minimum_, maximum_);
    const auto size = std::min(messages::max_address, buffer_.size() / divide);

    // Vary the start position (value fingerprinting).
    const auto limit = sub1(buffer_.size());
    auto index = pseudo_random::next(zero, limit);

    // Collect the messages.
    const auto out = to_shared<messages::address>();
    out->addresses.reserve(size);
    for (size_t count = 0; count < size; ++count)
        out->addresses.push_back(buffer_.at(index++ % limit));

    // Shuffle the message (order fingerprinting).
    pseudo_random::shuffle(out->addresses);
    handler(error::success, out);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
