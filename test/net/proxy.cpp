/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
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
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(proxy_tests)

class mock_proxy
  : public proxy
{
public:
    static std::string extract_command(const system::chunk_ptr& payload)
    {
        return proxy::extract_command(payload);
    }

    // Call must be stranded.
    template <class Message, typename Handler = pump::handler<Message>>
    void subscribe_message(Handler&& handler)
    {
        proxy::subscribe<Message>(std::forward<Handler>(handler));
    }

    // Call must be stranded.
    void subscribe_stop1(result_handler handler)
    {
        proxy::subscribe_stop(std::move(handler));
    }

    mock_proxy(socket::ptr socket) noexcept
      : proxy(socket)
    {
    }

    void stop(const code& ec) noexcept override
    {
        proxy::stop(ec);

        if (!stop_)
        {
            stop_ = true;
            stopped_.set_value(ec);
        }
    }

    code require_stopped() const
    {
        return stopped_.get_future().get();
    }

    size_t maximum_payload() const noexcept override
    {
        return 0;
    }

    uint32_t protocol_magic() const noexcept override
    {
        return 0;
    }

    bool validate_checksum() const noexcept override
    {
        return false;
    }

    bool verbose() const noexcept override
    {
        return false;
    }

    uint32_t version() const noexcept override
    {
        return 0;
    }

    void signal_activity() noexcept override
    {
    }

private:
    mutable bool stop_{ false };
    mutable std::promise<code> stopped_;
};

BOOST_AUTO_TEST_CASE(proxy__extract_command__empty_payload__unknown)
{
    const auto payload = std::make_shared<system::data_chunk>();
    BOOST_REQUIRE_EQUAL(mock_proxy::extract_command(payload), "<unknown>");
}

BOOST_AUTO_TEST_CASE(proxy__extract_command__short_payload__unknown)
{
    constexpr auto minimum = sizeof(uint32_t) + messages::heading::command_size;
    const auto payload = std::make_shared<system::data_chunk>(sub1(minimum), 'a');
    BOOST_REQUIRE_EQUAL(mock_proxy::extract_command(payload), "<unknown>");
}

BOOST_AUTO_TEST_CASE(proxy__extract_command__minimal_payload__expected)
{
    const auto payload = std::make_shared<system::data_chunk>(system::data_chunk
    {
        'a', 'b', 'c', 'd', 'w', 'x', 'y', 'z', 'w', 'x', 'y', 'z', 'w', 'x', 'y', 'z'
    });

    BOOST_REQUIRE_EQUAL(mock_proxy::extract_command(payload), "wxyzwxyzwxyz");
}

BOOST_AUTO_TEST_CASE(proxy__extract_command__extra_payload__expected)
{
    const auto payload = std::make_shared<system::data_chunk>(system::data_chunk
    {
        'a', 'b', 'c', 'd', 'w', 'x', 'y', 'z', 'w', 'x', 'y', 'z', 'w', 'x', 'y', 'z', 'A', 'B', 'C'
    });

    BOOST_REQUIRE_EQUAL(mock_proxy::extract_command(payload), "wxyzwxyzwxyz");
}

BOOST_AUTO_TEST_CASE(proxy__paused__default__true)
{
    threadpool pool(1);
    auto socket_ptr = std::make_shared<network::socket>(pool.service());
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);

    std::promise<bool> paused;
    boost::asio::post(proxy_ptr->strand(), [=, &paused]()
    {
        paused.set_value(proxy_ptr->paused());
    });

    BOOST_REQUIRE(paused.get_future().get());
    proxy_ptr->stop(error::invalid_magic);
    proxy_ptr.reset();
}

BOOST_AUTO_TEST_CASE(proxy__paused__pause__true)
{
    threadpool pool(1);
    auto socket_ptr = std::make_shared<network::socket>(pool.service());
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);

    std::promise<bool> paused;
    boost::asio::post(proxy_ptr->strand(), [=, &paused]()
    {
        proxy_ptr->pause();
        paused.set_value(proxy_ptr->paused());
    });

    BOOST_REQUIRE(paused.get_future().get());
    proxy_ptr->stop(error::invalid_magic);
    proxy_ptr.reset();
}

BOOST_AUTO_TEST_CASE(proxy__paused__resume__false)
{
    threadpool pool(1);
    auto socket_ptr = std::make_shared<network::socket>(pool.service());
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);

    std::promise<bool> paused;
    boost::asio::post(proxy_ptr->strand(), [=, &paused]()
    {
        // Resume queues up a read that will not execute until after this. 
        proxy_ptr->resume();
        paused.set_value(proxy_ptr->paused());
    });

    BOOST_REQUIRE(!paused.get_future().get());

    // Ensures stop is not executed concurrenty due to resume, guarding promise.
    std::promise<bool> stopped;
    boost::asio::post(proxy_ptr->strand(), [=, &stopped]()
    {
        proxy_ptr->stop(error::invalid_magic);
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    proxy_ptr.reset();
}

BOOST_AUTO_TEST_CASE(proxy__paused__resume_pause__true)
{
    threadpool pool(1);
    auto socket_ptr = std::make_shared<network::socket>(pool.service());
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);

    std::promise<bool> paused;
    boost::asio::post(proxy_ptr->strand(), [=, &paused]()
    {
        // Resume queues up a read that will not execute until after this. 
        proxy_ptr->resume();
        proxy_ptr->pause();
        paused.set_value(proxy_ptr->paused());
    });

    BOOST_REQUIRE(paused.get_future().get());

    // Ensures stop is not executed concurrenty due to resume, guarding promise.
    std::promise<bool> stopped;
    boost::asio::post(proxy_ptr->strand(), [=, &stopped]()
    {
        proxy_ptr->stop(error::invalid_magic);
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    proxy_ptr.reset();
}

BOOST_AUTO_TEST_CASE(proxy__paused__resume_after_read_fail__true)
{
    threadpool pool(2);
    auto socket_ptr = std::make_shared<network::socket>(pool.service());
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);

    std::promise<bool> paused_after_resume;
    boost::asio::post(proxy_ptr->strand(), [=, &paused_after_resume]()
    {
        // Resume queues up a read that will invoke stopped.
        proxy_ptr->resume();
        paused_after_resume.set_value(proxy_ptr->paused());
    });

    BOOST_REQUIRE(!paused_after_resume.get_future().get());
    BOOST_REQUIRE(proxy_ptr->require_stopped());

    std::promise<bool> paused_after_read_fail;
    boost::asio::post(proxy_ptr->strand(), [=, &paused_after_read_fail]()
    {
        paused_after_read_fail.set_value(proxy_ptr->paused());
    });

    BOOST_REQUIRE(paused_after_read_fail.get_future().get());

    // Ensures stop is not executed concurrenty due to resume, guarding promise.
    std::promise<bool> stopped;
    boost::asio::post(proxy_ptr->strand(), [=, &stopped]()
    {
        proxy_ptr->stop(error::invalid_magic);
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    proxy_ptr.reset();
}

BOOST_AUTO_TEST_CASE(proxy__stopped__default__false)
{
    threadpool pool(2);
    auto socket_ptr = std::make_shared<network::socket>(pool.service());
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    BOOST_REQUIRE(!proxy_ptr->stopped());

    proxy_ptr->stop(error::invalid_magic);
    proxy_ptr.reset();
}

BOOST_AUTO_TEST_CASE(proxy__stranded__default__false)
{
    threadpool pool(2);
    auto socket_ptr = std::make_shared<network::socket>(pool.service());
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    BOOST_REQUIRE(!proxy_ptr->stranded());

    proxy_ptr->stop(error::invalid_magic);
    proxy_ptr.reset();
}

BOOST_AUTO_TEST_CASE(proxy__authority__default__expected)
{
    threadpool pool(2);
    const config::authority default_authority{};
    auto socket_ptr = std::make_shared<network::socket>(pool.service());
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    BOOST_REQUIRE(proxy_ptr->authority() == default_authority);

    proxy_ptr->stop(error::invalid_magic);
    proxy_ptr.reset();
}

BOOST_AUTO_TEST_CASE(proxy__subscribe_stop__subscribed__expected)
{
    threadpool pool(2);
    auto socket_ptr = std::make_shared<network::socket>(pool.service());
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    constexpr auto expected_ec = error::invalid_magic;

    std::promise<code> stop2_stopped;
    std::promise<code> stop_subscribed;
    proxy_ptr->subscribe_stop(
        [=, &stop2_stopped](code ec)
        {
            stop2_stopped.set_value(ec);
        },
        [=, &stop_subscribed](code ec)
        {
            stop_subscribed.set_value(ec);
        });

    BOOST_REQUIRE(!proxy_ptr->stopped());
    BOOST_REQUIRE_EQUAL(stop_subscribed.get_future().get(), error::success);

    proxy_ptr->stop(expected_ec);
    BOOST_REQUIRE_EQUAL(stop2_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE(proxy_ptr->stopped());

    proxy_ptr.reset();
}

BOOST_AUTO_TEST_CASE(proxy__do_subscribe_stop__subscribed__expected)
{
    threadpool pool(2);
    auto socket_ptr = std::make_shared<network::socket>(pool.service());
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    constexpr auto expected_ec = error::invalid_magic;

    std::promise<code> stop1_stopped;
    boost::asio::post(proxy_ptr->strand(), [&]()
    {
        proxy_ptr->subscribe_stop1([=, &stop1_stopped](code ec)
        {
            stop1_stopped.set_value(ec);
        });
    });

    BOOST_REQUIRE(!proxy_ptr->stopped());

    proxy_ptr->stop(expected_ec);
    BOOST_REQUIRE_EQUAL(stop1_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE(proxy_ptr->stopped());

    proxy_ptr.reset();
}

BOOST_AUTO_TEST_CASE(proxy__subscribe_message__subscribed__expected)
{
    threadpool pool(2);
    auto socket_ptr = std::make_shared<network::socket>(pool.service());
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    constexpr auto expected_ec = error::invalid_magic;

    std::promise<code> message_stopped;
    boost::asio::post(proxy_ptr->strand(), [&]()
    {
        proxy_ptr->subscribe_message<messages::ping>(
            [&](code ec, messages::ping::ptr ping)
            {
                BOOST_REQUIRE(!ping);
                message_stopped.set_value(ec);
            });
    });

    BOOST_REQUIRE(!proxy_ptr->stopped());

    proxy_ptr->stop(expected_ec);
    BOOST_REQUIRE_EQUAL(message_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE(proxy_ptr->stopped());

    proxy_ptr.reset();
}

BOOST_AUTO_TEST_CASE(proxy__stop__all_subscribed__expected)
{
    threadpool pool(2);
    auto socket_ptr = std::make_shared<network::socket>(pool.service());
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    constexpr auto expected_ec = error::invalid_magic;

    std::promise<code> stop2_stopped;
    std::promise<code> stop_subscribed;
    proxy_ptr->subscribe_stop(
        [=, &stop2_stopped](code ec)
        {
            stop2_stopped.set_value(ec);
        },
        [=, &stop_subscribed](code ec)
        {
            stop_subscribed.set_value(ec);
        });

    std::promise<code> stop1_stopped;
    std::promise<code> message_stopped;
    boost::asio::post(proxy_ptr->strand(), [&]()
    {
        proxy_ptr->subscribe_stop1([=, &stop1_stopped](code ec)
        {
            stop1_stopped.set_value(ec);
        });

        proxy_ptr->subscribe_message<messages::ping>(
            [&](code ec, messages::ping::ptr ping)
            {
                BOOST_REQUIRE(!ping);
                message_stopped.set_value(ec);
            });
    });

    BOOST_REQUIRE(!proxy_ptr->stopped());
    BOOST_REQUIRE_EQUAL(stop_subscribed.get_future().get(), error::success);

    proxy_ptr->stop(expected_ec);
    BOOST_REQUIRE_EQUAL(message_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE_EQUAL(stop1_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE_EQUAL(stop2_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE(proxy_ptr->stopped());

    proxy_ptr.reset();
}

BOOST_AUTO_TEST_CASE(proxy__send__not_connected__expected)
{
    threadpool pool(2);
    auto socket_ptr = std::make_shared<network::socket>(pool.service());
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    auto ping_ptr = std::shared_ptr<messages::ping>(new messages::ping{ 42 });

    std::promise<code> promise;
    const auto handler = [&](code ec)
    {
        // Send failure causes stop before handler invoked.
        BOOST_REQUIRE(proxy_ptr->stopped());
        promise.set_value(ec);
    };

    proxy_ptr->send<messages::ping>(ping_ptr, handler);

    // 10009 (WSAEBADF, invalid file handle) gets mapped to file_system.
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::file_system);
    proxy_ptr.reset();
}

BOOST_AUTO_TEST_CASE(proxy__send__not_connected_move__expected)
{
    threadpool pool(2);
    auto socket_ptr = std::make_shared<network::socket>(pool.service());
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    auto ping_ptr = std::shared_ptr<messages::ping>(new messages::ping{ 42 });

    std::promise<code> promise;
    proxy_ptr->send<messages::ping>(ping_ptr, [&](code ec)
    {
        // Send failure causes stop before handler invoked.
        BOOST_REQUIRE(proxy_ptr->stopped());
        promise.set_value(ec);
    });

    // 10009 (WSAEBADF, invalid file handle) gets mapped to file_system.
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::file_system);
    proxy_ptr.reset();
}

BOOST_AUTO_TEST_SUITE_END()
