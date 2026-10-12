#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <set>

#include "global.hpp"
#include "session.hpp"
#include "store.hpp"

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
  boost::asio::io_context& io_;
  tcp::acceptor acceptor_;
  std::set<std::shared_ptr<Session>> session_set;
  Store store;

  boost::asio::awaitable<void> start_listen(void) {
    boost::system::error_code ec;
    std::cout << "Server is listening on Port "
              << this->acceptor_.local_endpoint().port() << "\n";
    while (this->acceptor_.is_open()) {
      auto client = co_await this->acceptor_.async_accept(
          boost::asio::redirect_error(boost::asio::use_awaitable, ec));

      if (ec == boost::asio::error::operation_aborted)
        break;
      else if (ec) {
        print_error(boost::system::system_error(ec));
        boost::asio::steady_timer timer(this->io_,
                                        std::chrono::milliseconds(100));
        co_await timer.async_wait(boost::asio::use_awaitable);
        continue;
      }

      auto session = std::make_shared<Session>(std::move(client), store);

      boost::asio::co_spawn(
          this->io_,
          [this, session]() -> boost::asio::awaitable<void> {
            co_await session->run_session();
            this->session_set.erase(session);
          },
          boost::asio::detached);

      session_set.insert(session);

      std::cout << "[conn] simultaneous coneection(s): " << session_set.size()
                << '\n';
    }
  }
};
