#include <boost/asio.hpp>
#include <iostream>

#include "server.hpp"

int main() {
  boost::asio::io_context io;

  Server server(io, 6379);

  boost::asio::signal_set signals(io, SIGINT, SIGTERM);
  signals.async_wait([&](const boost::system::error_code& error,
                         const int signal_number) {
    if (!error) {
      std::cout << "[sign] Shutdown Signal received - signal: " << signal_number
                << "\n";

      server.stop();
    }
  });

  io.run();

  return 0;
}
