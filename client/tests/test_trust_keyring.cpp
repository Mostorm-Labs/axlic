#include "axlic/core/trust_keyring.hpp"

#include <catch2/catch_test_macros.hpp>

#define NOMINMAX
#include <windows.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace {

constexpr std::string_view kEmptyFloor = R"json({"format":"axlic-trust-floor","version":1,"keys":[]})json";

constexpr std::string_view kFloorWithKey = R"json({
  "format": "axlic-trust-floor",
  "version": 1,
  "keys": [
    {"key_id": "k1", "public_key_xy_hex": "01010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"}
  ]
})json";

constexpr std::string_view kKeyHex = "02020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202";

class TemporaryRoot {
 public:
  TemporaryRoot() {
    const auto suffix = std::to_string(GetCurrentProcessId()) + "-" +
                        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    path = std::filesystem::temp_directory_path() / ("axlic-a2-keyring-" + suffix);
    std::filesystem::create_directories(path);
  }
  ~TemporaryRoot() { std::filesystem::remove_all(path); }
  TemporaryRoot(const TemporaryRoot&) = delete;
  TemporaryRoot& operator=(const TemporaryRoot&) = delete;
  std::filesystem::path path;
};

void write_overlay(const std::filesystem::path& root, const std::string& content) {
  std::ofstream stream(root / "trust-keyring.v1.json", std::ios::binary | std::ios::trunc);
  stream << content;
}

}  // namespace

TEST_CASE("empty floor with no overlay yields an empty fail-closed keyring") {
  TemporaryRoot root;
  const auto read = axlic::core::load_trust_keyring_from(kEmptyFloor, root.path);
  CHECK(read.keys.empty());
  CHECK_FALSE(read.overlay_ignored);
}

TEST_CASE("overlay entries merge into the floor") {
  TemporaryRoot root;
  write_overlay(root.path, std::string(R"json({
    "format": "axlic-trust-keyring-append",
    "version": 1,
    "keys": [
      {"key_id": "k2", "public_key_xy_hex": ")json") + std::string(kKeyHex) + R"json("}
    ]
  })json");
  const auto read = axlic::core::load_trust_keyring_from(kFloorWithKey, root.path);
  REQUIRE(read.keys.size() == 2U);
  CHECK(read.keys[0].key_id == "k1");
  CHECK(read.keys[1].key_id == "k2");
  CHECK(read.keys[1].public_key_xy[0] == 0x02U);
  CHECK_FALSE(read.overlay_ignored);
}

TEST_CASE("overlay entry with the same key_id overrides the floor entry") {
  TemporaryRoot root;
  write_overlay(root.path, std::string(R"json({
    "format": "axlic-trust-keyring-append",
    "version": 1,
    "keys": [
      {"key_id": "k1", "public_key_xy_hex": ")json") + std::string(kKeyHex) + R"json("}
    ]
  })json");
  const auto read = axlic::core::load_trust_keyring_from(kFloorWithKey, root.path);
  REQUIRE(read.keys.size() == 1U);
  CHECK(read.keys[0].key_id == "k1");
  CHECK(read.keys[0].public_key_xy[0] == 0x02U);
  CHECK_FALSE(read.overlay_ignored);
}

TEST_CASE("malformed overlay is ignored and only the floor contributes trust") {
  TemporaryRoot root;
  SECTION("not json") { write_overlay(root.path, "not json at all"); }
  SECTION("wrong format tag") {
    write_overlay(root.path, R"json({"format":"something-else","version":1,"keys":[]})json");
  }
  SECTION("wrong version") {
    write_overlay(root.path, R"json({"format":"axlic-trust-keyring-append","version":2,"keys":[]})json");
  }
  SECTION("entry uppercase hex") {
    write_overlay(root.path, std::string(R"json({
      "format": "axlic-trust-keyring-append",
      "version": 1,
      "keys": [
        {"key_id": "k9", "public_key_xy_hex": ")json") +
                                       std::string(128U, 'A') + R"json("}
      ]
    })json");
  }
  SECTION("entry wrong hex length") {
    write_overlay(root.path, R"json({
      "format": "axlic-trust-keyring-append",
      "version": 1,
      "keys": [
        {"key_id": "k9", "public_key_xy_hex": "0102"}
      ]
    })json");
  }
  const auto read = axlic::core::load_trust_keyring_from(kFloorWithKey, root.path);
  REQUIRE(read.keys.size() == 1U);
  CHECK(read.keys[0].key_id == "k1");
  CHECK(read.keys[0].public_key_xy[0] == 0x01U);
  CHECK(read.overlay_ignored);
}

TEST_CASE("malformed floor collapses to an empty keyring") {
  TemporaryRoot root;
  const auto read = axlic::core::load_trust_keyring_from("}{ not json", root.path);
  CHECK(read.keys.empty());
}

TEST_CASE("embedded floor is the configured trust-floor document and parses") {
  std::ifstream stream(std::filesystem::path(AXLIC_TRUST_FLOOR_FILE), std::ios::binary);
  REQUIRE(stream.is_open());
  const std::string document{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
  REQUIRE(std::string(axlic::core::embedded_trust_floor_json()) == document);
  const auto parsed = axlic::core::parse_trust_keyring_document(
      axlic::core::embedded_trust_floor_json(), "axlic-trust-floor");
  REQUIRE(parsed.has_value());
}

TEST_CASE("key ids beyond the size bound are rejected") {
  const std::string long_key_id(1025U, 'a');
  const auto document = std::string(R"json({"format":"axlic-trust-floor","version":1,"keys":[{"key_id":")json") +
                           long_key_id + R"json(","public_key_xy_hex":")json" + std::string(kKeyHex) + R"json("}]})json";
  CHECK_FALSE(axlic::core::parse_trust_keyring_document(document, "axlic-trust-floor").has_value());
}
