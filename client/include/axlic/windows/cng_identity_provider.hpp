#pragma once

#include "axlic/core/identity.hpp"

namespace axlic::windows {

enum class CngOperation { probe, establish, load };

[[nodiscard]] core::ProviderOutcome map_cng_status(long status, CngOperation operation) noexcept;

class CngIdentityProvider final : public core::IIdentityProvider {
 public:
  explicit CngIdentityProvider(core::ProviderKind kind);

  [[nodiscard]] core::ProviderKind provider_kind() const noexcept override;
  [[nodiscard]] std::string_view provider_name() const noexcept override;
  [[nodiscard]] std::string_view scheme_id() const noexcept override;
  core::ProviderOutcome probe() override;
  core::ProviderResponse establish(const std::string& key_name) override;
  core::ProviderResponse load(const std::string& key_name) override;
  void remove(const std::string& key_name) noexcept override;

 private:
  core::ProviderKind kind_;
};

}  // namespace axlic::windows
