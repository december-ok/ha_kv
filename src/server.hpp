#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <set>

#include "global.hpp"
#include "session.hpp"

using boost::asio::ip::tcp;

class Server {
 public:
  explicit Server(boost::asio::io_context& io, uint16_t port)
      : io_(io), acceptor_(io, tcp::endpoint(tcp::v4(), port)) {
    boost::asio::co_spawn(io, start_listen(), boost::asio::detached);
  }

  void stop() {
    acceptor_.close();

    for (auto& session : session_set) {
      session->stop();
    }
    session_set.clear();
  }

 private:
  boost::asio::awaitable<void> start_listen(void) {
    std::cout << "Server is listening on Port "
              << this->acceptor_.local_endpoint().port() << "\n";
    while (true) {
      try {
        auto client =
            co_await this->acceptor_.async_accept(boost::asio::use_awaitable);

        auto session = std::make_shared<Session>(std::move(client));

        boost::asio::co_spawn(
            this->io_,
            [this, session]() -> boost::asio::awaitable<void> {
              co_await session->run_session();
              this->session_set.erase(session);
            },
            boost::asio::detached);

        session_set.insert(session);

        std::cout << "[acpt] size: " << session_set.size() << '\n';
      } catch (const boost::system::system_error& error) {
        if (error.code() == boost::asio::error::operation_aborted) {
          break;
        }
        print_error(error);
      }
    }
  }

  boost::asio::io_context& io_;
  tcp::acceptor acceptor_;
  std::set<std::shared_ptr<Session>> session_set;
};
