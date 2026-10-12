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

  // 반환된 string_view는 consume() 전까지만 유효
  boost::asio::awaitable<std::pair<std::string_view, size_t>> recv(void) {
    auto n = co_await boost::asio::async_read_until(
        this->client_socket, this->buffer, '\n', boost::asio::use_awaitable);

    std::string_view s(static_cast<const char*>(this->buffer.data().data()), n);

    while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
      s.remove_suffix(1);

    co_return std::pair(s, n);
  }

  boost::asio::awaitable<void> send_reply(const Reply& reply) {
    std::string str = Store::serialize_reply(reply);

    co_await boost::asio::async_write(this->client_socket,
                                      boost::asio::buffer(str),
                                      boost::asio::use_awaitable);
  }
};
