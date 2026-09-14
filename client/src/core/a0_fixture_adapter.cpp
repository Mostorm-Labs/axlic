#include "axlic/core/a0_fixture_adapter.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace axlic::core {
namespace {

constexpr std::size_t kMaximumStateCarrierSize = 256U * 1024U;

std::uint8_t nibble(const char value) {
  if (value >= '0' && value <= '9') {
    return static_cast<std::uint8_t>(value - '0');
  }
  if (value >= 'a' && value <= 'f') {
    return static_cast<std::uint8_t>(value - 'a' + 10);
  }
  if (value >= 'A' && value <= 'F') {
    return static_cast<std::uint8_t>(value - 'A' + 10);
  }
  throw std::runtime_error("invalid test carrier");
}

std::vector<std::uint8_t> decode_hex(const std::string& value) {
  if ((value.size() % 2U) != 0U || value.size() > kMaximumStateCarrierSize * 2U) {
    throw std::runtime_error("invalid test carrier");
  }
  std::vector<std::uint8_t> result;
  result.reserve(value.size() / 2U);
  for (std::size_t index = 0; index < value.size(); index += 2U) {
    result.push_back(static_cast<std::uint8_t>((nibble(value[index]) << 4U) | nibble(value[index + 1U])));
  }
  return result;
}

}  // namespace

A0FixtureAdapter::A0FixtureAdapter(std::filesystem::path state_path) : state_path_(std::move(state_path)) {}

CredentialMaterial A0FixtureAdapter::load() const {
  const auto file_size = std::filesystem::file_size(state_path_);
  if (file_size > kMaximumStateCarrierSize) {
    throw std::runtime_error("invalid test carrier");
  }
  std::ifstream stream(state_path_, std::ios::binary);
  if (!stream) {
    throw std::runtime_error("invalid test carrier");
  }
  const std::string carrier{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
  const auto document = nlohmann::json::parse(carrier);
  const auto state = document.at("state").get<std::string>();
  if (state == "absent") {
    return {};
  }
  if (state != "present") {
    throw std::runtime_error("invalid test carrier");
  }
  CredentialMaterial material{.present = true, .artifact = decode_hex(document.at("artifact_hex").get<std::string>())};
  for (const auto& entry : document.at("trusted_keys")) {
    const auto raw_key = decode_hex(entry.at("public_key_xy_hex").get<std::string>());
    if (raw_key.size() != 64U) {
      throw std::runtime_error("invalid test carrier");
    }
    wire::TrustedPublicKey key{.key_id = entry.at("key_id").get<std::string>()};
    std::copy(raw_key.begin(), raw_key.end(), key.public_key_xy.begin());
    material.trusted_keys.push_back(std::move(key));
  }
  if (material.trusted_keys.empty()) {
    throw std::runtime_error("invalid test carrier");
  }
  return material;
}

}  // namespace axlic::core
