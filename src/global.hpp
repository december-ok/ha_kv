#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <source_location>

inline void print_error(
    const boost::system::system_error& error,
    const std::source_location loc = std::source_location::current()) {
  std::cerr << '[' << loc.file_name() << ':' << loc.line() << "] "
            << " - " << error.what() << " (error code: " << error.code().value()
            << ", message: " << error.code().message() << ")\n";
}