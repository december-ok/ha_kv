#include <boost/asio.hpp>
#include <iostream>

#include "server.hpp"

int main(int argc, char* argv[]) {
  try {
    const auto port_num = argc > 1 ? std::stoul(argv[1]) : 6379;
    if (port_num > 65535) throw std::out_of_range("port > 65535");
    const auto port = static_cast<uint16_t>(port_num);

    boost::asio::io_context io;

    Server server(io, port);

    boost::asio::signal_set signals(io, SIGINT, SIGTERM);
    signals.async_wait(
        [&](const boost::system::error_code& error, const int signal_number) {
          if (!error) {
            std::cout << "[sign] Shutdown Signal received - signal: "
                      << signal_number << "\n";

            server.stop();
          }
        });

    io.run();
  } catch (const boost::system::system_error& error) {
    print_error(error);
    return 1;
  } catch (const std::exception& e) {
    std::cerr << "[fatal] " << e.what() << '\n';
    return 1;
  }

  return 0;
}
