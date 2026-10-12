#pragma once
#include <boost/asio.hpp>
#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <cstdint>
#include <functional>
#include <string_view>
#include <variant>

struct TransparentHash {
  using is_transparent = void;

  std::size_t operator()(std::string_view s) const noexcept {
    return std::hash<std::string_view>{}(s);
  }
};

struct SimpleStringReply {
  std::string_view value;
};

struct BulkStringReply {
  std::string_view value;
};

struct NullBulkStringReply {};

struct IntegerReply {
  std::int64_t value;
};

struct ErrorReply {
  std::string_view message;
};

using Reply = std::variant<SimpleStringReply, BulkStringReply,
                           NullBulkStringReply, IntegerReply, ErrorReply>;

static inline bool is_equal(std::string_view a, std::string_view b) {
  return a.size() == b.size() && strncasecmp(a.data(), b.data(), a.size()) == 0;
}

class Store {
 private:
  boost::unordered::unordered_map<std::string, std::string, TransparentHash,
                                  std::equal_to<>>
      kv_map;
  const std::string_view CMD_SET = "SET";
  const std::string_view CMD_GET = "GET";
  const std::string_view CMD_DEL = "DEL";

  std::vector<std::string_view> split(std::string_view str) {
    std::vector<std::string_view> tokens;
    tokens.reserve(16);
    size_t pos = 0;

    while (pos < str.size()) {
      while (pos < str.size() && str[pos] == ' ') ++pos;
      if (pos == str.size()) break;

      size_t end = str.find(' ', pos);
      if (end == std::string_view::npos) end = str.size();

      tokens.push_back(str.substr(pos, end - pos));

      pos = end;
    }

    return tokens;
  }

  void set(std::string_view key, std::string_view value) {
    kv_map.insert_or_assign(key, value);
  }

  std::optional<std::string_view> get(std::string_view key) {
    auto it = kv_map.find(key);
    if (it != kv_map.end()) return it->second;
    return std::nullopt;
  }

  size_t del(std::string_view key) { return kv_map.erase(key); }

 public:
  Reply process(std::string_view str) {
#ifndef NDEBUG
    std::cout << "[recv] " << str << '\n';
#endif
    std::vector<std::string_view> cmd_list = split(str);

    if (cmd_list.size() <= 0) return ErrorReply{"wrong command"};

    std::string_view main_cmd = cmd_list[0];

    if (is_equal(main_cmd, CMD_SET)) {
      if (cmd_list.size() != 3) return ErrorReply{"wrong number of arguments"};
      this->set(cmd_list[1], cmd_list[2]);

      return SimpleStringReply{"OK"};
    } else if (is_equal(main_cmd, CMD_GET)) {
      if (cmd_list.size() != 2) return ErrorReply{"wrong number of arguments"};

      auto value = this->get(cmd_list[1]);
      if (value.has_value()) {
        return BulkStringReply{*value};
      }

      return NullBulkStringReply{};
    } else if (is_equal(main_cmd, CMD_DEL)) {
      if (cmd_list.size() != 2) return ErrorReply{"wrong number of arguments"};

      return IntegerReply{static_cast<std::int64_t>(this->del(cmd_list[1]))};
    }

    return ErrorReply{"unknown command"};
  }

  static std::string serialize_reply(const Reply& reply) {
    return std::visit(
        [](const auto& r) -> std::string {
          using T = std::decay_t<decltype(r)>;
          std::string out;

          if constexpr (std::is_same_v<T, SimpleStringReply>) {
            out.reserve(r.value.size() + 3);
            out += '+';
            out.append(r.value.data(), r.value.size());
            out += "\r\n";
          } else if constexpr (std::is_same_v<T, BulkStringReply>) {
            out.reserve(r.value.size() + 32);
            out += '$';
            out += std::to_string(r.value.size());
            out += "\r\n";
            out.append(r.value.data(), r.value.size());
            out += "\r\n";
          } else if constexpr (std::is_same_v<T, NullBulkStringReply>) {
            out = "$-1\r\n";
          } else if constexpr (std::is_same_v<T, IntegerReply>) {
            out += ':';
            out += std::to_string(r.value);
            out += "\r\n";
          } else if constexpr (std::is_same_v<T, ErrorReply>) {
            out += '-';
            out.append(r.message.data(), r.message.size());
            out += "\r\n";
          }

          return out;
        },
        reply);
  }
};
