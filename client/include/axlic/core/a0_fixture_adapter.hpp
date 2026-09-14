#pragma once

#include "axlic/wire/credential.hpp"

#include <filesystem>
#include <vector>

namespace axlic::core {

struct CredentialMaterial {
  bool present{};
  std::vector<std::uint8_t> artifact;
  std::vector<wire::TrustedPublicKey> trusted_keys;
};

class A0FixtureAdapter {
 public:
  explicit A0FixtureAdapter(std::filesystem::path state_path);
  [[nodiscard]] CredentialMaterial load() const;

 private:
  std::filesystem::path state_path_;
};

}  // namespace axlic::core
