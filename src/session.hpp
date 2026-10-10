#pragma once

#include <boost/asio.hpp>

#include "global.hpp"
#include "store.hpp"

using boost::asio::ip::tcp;

class Session {
 public:
  explicit Session(tcp::socket&& client, Store& st)
      : client_socket(std::move(client)), store(st) {}

  boost::asio::awaitable<void> run_session(void) {
    try {
      while (true) {
        auto [cmd_str, n] = co_await recv();

        Reply reply = store.process(cmd_str);

        this->buffer.consume(n);

        co_await send_reply(reply);
      }
    } catch (const boost::system::system_error& error) {
      if (error.code() == boost::asio::error::eof) {
        std::cout << "[quit] user quit\n";
      } else if (error.code() != boost::asio::error::operation_aborted) {
        print_error(error);
      }
      this->stop();
    }
  }

  void stop() {
    boost::system::error_code ec;

    client_socket.shutdown(tcp::socket::shutdown_both, ec);
    client_socket.close(ec);
  }

 private:
  tcp::socket client_socket;
  boost::asio::streambuf buffer{65536};
  Store& store;

  boost::asio::awaitable<std::pair<std::string, size_t>> recv(void) {
    auto n = co_await boost::asio::async_read_until(
        this->client_socket, this->buffer, '\n', boost::asio::use_awaitable);

    std::string s(boost::asio::buffers_begin(this->buffer.data()),
                  boost::asio::buffers_begin(this->buffer.data()) + n);

    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();

    co_return std::pair(std::move(s), n);
  }

  boost::asio::awaitable<void> send_reply(const Reply& reply) {
    std::string str = Store::serialize_reply(reply);

    co_await boost::asio::async_write(this->client_socket,
                                      boost::asio::buffer(str),
                                      boost::asio::use_awaitable);
  }
};
