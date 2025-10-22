/*
Copyright (c) 2025 Giuseppe Roberti.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <expected>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <regex>
#include <string>

namespace fs = std::filesystem;

struct error_code {
  enum kind : int {

    duplicate_arg = 1,
    unknown_arg,
    generic,

    path_absent,
    path_not_exist,
    path_not_dir,

  } m_code;

  std::optional<std::string> m_msg;

  error_code(auto c, auto m = {})
    : m_code{ c }, m_msg{ m }
  {
  }

  auto message() const
    -> std::string
  {
    return m_msg.value_or(default_msg());
  }

  auto value() const
    -> int
  {
    return m_code;
  }

private:
  std::string default_msg() const {
    switch (m_code) {

    case error_code::duplicate_arg: return "Use one modifier at most one time!";
    case error_code::unknown_arg: return "Unknown modifier!";
    case error_code::generic: return "Generic error";

    case error_code::path_absent: return "Please specify a directory to proceed!";
    case error_code::path_not_exist: return "The path is not accessible or does not exists!";
    case error_code::path_not_dir: return "The path is not a directory!";

    }
    return "<unspecified error message>";
  }
};

static inline auto make_unexpected(error_code::kind c, std::optional<std::string> m = {}) {
  return std::unexpected{ error_code{ c, m } };
}

template<typename... Args>
void err(std::format_string<Args...> fmt, Args&&... args) {
  std::cerr
    << "ERROR: "
    << std::format(fmt, std::forward<Args&&...>(args)...)
    << std::endl;
}

struct opts {

public:

  using size_type = std::size_t;

  using arg_type = std::string_view;

  struct iterator {

  private:
    const char** data;

  public:
    iterator(const char** data) : data{ data } {}

    inline bool operator==(iterator& o) { return data == o.data; }

    inline iterator& operator++() { ++data; return *this; }

    inline arg_type operator* () { return *data; }
  };

  opts(const int argc, const char** argv) noexcept
    : n{ static_cast<size_type>(argc) }
    , vs{ argv }
  {
  }

  inline arg_type at(size_type n) const noexcept { return vs[n]; }

  inline size_type size() const noexcept { return n; }

  iterator begin() const { return &vs[0]; }

  iterator it(std::size_t i) const { return &vs[i]; }

  iterator end() const { return &vs[n]; }

private:

  const std::size_t n;

  const char** vs;
};

struct finder {
public:

  struct type_filter {
    enum kind : int {
      directories,
      files
    } value;

    type_filter(kind v) : value{ v } {}

    static auto from(std::string_view t) noexcept
      -> std::optional<type_filter>
    {
      if (t.size() != 1)
        return {};

      switch (t.at(0)) {

      case 'd':
        return directories;

      case 'f':
        return files;
      }

      return {};
    }

    auto repr() noexcept -> std::string_view {
      switch (value) {

      case directories:
        return "directories";

      case files:
        return "files";
      }
    }
  };

  struct params {
    std::optional<fs::path> path;
    std::optional<type_filter> type;
    std::optional<std::regex> name;
    std::optional<std::regex> iname;

    params() = default;

    params(const params&) = delete;

    params(params&&) = default;

    params& operator=(const params&) = delete;

    params& operator=(params&&) = default;

    static auto replace_all(std::string str, const std::string& from, const std::string& to)
      -> std::string
    {
      if (from.empty()) return str; // avoid infinite loop

      size_t start_pos = 0;
      while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);

        start_pos += to.length(); // move past the last replacement
      }

      return str;
    }

    static auto regex_from(const std::string_view& s, bool icase) noexcept
      -> std::regex
    {
      auto t = icase
        ? std::regex_constants::ECMAScript | std::regex_constants::icase
        : std::regex_constants::ECMAScript;

      return std::regex{ replace_all(std::string{s}, "*", ".*"), t };
    }

    static auto from(const opts& opts) noexcept
      -> std::expected<params, error_code>
    {
      auto obj = params{};

      //obj.path = fs::current_path(); // uncomment to achieve same find behaviour

      if (opts.size() <= 1)
        return obj;

      auto it = opts.it(1);

      if (!(*it).starts_with("-")) {
        obj.path = opts.at(1);

        ++it;
      }

      enum {
        arg_none,
        arg_type,
        arg_name,
        arg_iname,
      } current = arg_none;

      for (; it != opts.end(); ++it) {
        if (current == arg_none) {
          if (*it == "-type") {
            if (obj.type)
              return make_unexpected(error_code::duplicate_arg);

            current = arg_type;
          }

          else if (*it == "-name") {
            if (obj.name)
              return make_unexpected(error_code::duplicate_arg);

            current = arg_name;
          }

          else if (*it == "-iname") {
            if (obj.iname)
              return make_unexpected(error_code::duplicate_arg);

            current = arg_iname;
          }

          else {
            return make_unexpected(error_code::unknown_arg);
          }
        }
        else {
          switch (current) {

          case arg_none:
            return make_unexpected(error_code::generic);

          case arg_type:
            obj.type = type_filter::from(*it);

            break;

          case arg_name:
            obj.name = regex_from(*it, false);

            break;

          case arg_iname:
            obj.iname = regex_from(*it, true);

            break;
          }

          current = arg_none;
        }
      }

      return obj;
    }
  } params_m;

  finder(params params) noexcept : params_m{ std::move(params) } {}

  static auto make(params params) noexcept -> finder { return std::move(params); }

  static auto from(opts opts)
    -> std::expected<finder, error_code>
  {
    return params::from(opts).transform(make);
  }

  auto run() const noexcept
    -> std::expected<int, error_code>
  {
    if (!params_m.path)
      return make_unexpected(error_code::path_absent);

    if (!fs::exists(*params_m.path))
      return make_unexpected(error_code::path_not_exist);

    if (!fs::is_directory(*params_m.path))
      return make_unexpected(error_code::path_not_dir);

    // iterate path

    visit(*params_m.path);

    return EXIT_SUCCESS;
  }

  static auto handle_err(error_code ec)
    -> std::expected<int, error_code>
  {
    err("{}", ec.message().c_str());

    return ec.value();
  }

private:

  inline auto after_filtering(const fs::directory_entry& entry) const noexcept {
    return shall_print(entry) ? std::make_optional(entry) : std::nullopt;
  }

  inline void print_entry(std::optional<fs::directory_entry> entry) const noexcept {
    if (entry)
      std::cout << entry->path().native() << "\n";
  }

  bool shall_print(const fs::directory_entry& entry) const noexcept {
    bool res = true;

    // filter by type

    if (params_m.type)
      switch (params_m.type->value) {

      case type_filter::directories:
        res = entry.is_directory();

        break;

      case type_filter::files:
        res = entry.is_regular_file();

        break;
      }

    // filter by name
    if (params_m.name)
      res = res && std::regex_match(entry.path().filename().native(), *params_m.name);

    // filter by iname
    if (params_m.iname)
      res = res && std::regex_match(entry.path().filename().native(), *params_m.iname);

    return res;
  }

  inline void visit(fs::path dir_path) const { visit(fs::directory_entry{ dir_path }); }

  void visit(fs::directory_entry entry) const {
    if (entry.is_symlink())
      return;

    print_entry(after_filtering(entry));

    auto it = fs::directory_iterator(entry, fs::directory_options::skip_permission_denied);

    for (const auto& e : it)
      if (e.is_directory())
        visit(e.path());
      else
        print_entry(after_filtering(e));
  }
};

int main(const int argc, const char** argv) {
  return finder::from({ argc, argv })
    .and_then(&finder::run)
    .or_else(finder::handle_err)
    .value();
}
