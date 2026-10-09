#pragma once

#include <boost/asio.hpp>

#include "global.hpp"

using boost::asio::ip::tcp;

class Session {
 public:
  explicit Session(tcp::socket&& client) : client_socket(std::move(client)) {}

  boost::asio::awaitable<void> run_session(void) {
    size_t n;
    try {
      while (true) {
        n = co_await recv();
        co_await send(n);
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
  boost::asio::awaitable<size_t> recv(void) {
    auto n = co_await this->client_socket.async_read_some(
        boost::asio::buffer(this->buffer), boost::asio::use_awaitable);

    std::cout << "[recv] " << std::string_view(this->buffer.data(), n);

    co_return n;
  }

  boost::asio::awaitable<void> send(const size_t n) {
    co_await boost::asio::async_write(this->client_socket,
                                      boost::asio::buffer(this->buffer, n),
                                      boost::asio::use_awaitable);

    std::cout << "[send] " << std::string_view(this->buffer.data(), n);
  }

  tcp::socket client_socket;
  std::array<char, 4096> buffer;
};
