#pragma once

#include "axlic/core/identity.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace axlic::windows {

enum class ProtectionOutcome { ok, corrupt, unexpected_failure };

struct ProtectionResult {
  ProtectionOutcome outcome{ProtectionOutcome::unexpected_failure};
  std::vector<std::uint8_t> bytes;
};

class IDataProtector {
 public:
  virtual ~IDataProtector() = default;
  virtual ProtectionResult protect(const std::vector<std::uint8_t>& plaintext) = 0;
  virtual ProtectionResult unprotect(const std::vector<std::uint8_t>& protected_bytes) = 0;
};

class DpapiProtector final : public IDataProtector {
 public:
  ProtectionResult protect(const std::vector<std::uint8_t>& plaintext) override;
  ProtectionResult unprotect(const std::vector<std::uint8_t>& protected_bytes) override;
};

struct StateCodecResult {
  core::StateStoreOutcome outcome{core::StateStoreOutcome::unexpected_failure};
  std::optional<core::IdentityRecord> identity;
  std::string carrier;
};

class MachineStateCodec {
 public:
  static constexpr std::size_t maximum_carrier_size = 1024U * 1024U;

  explicit MachineStateCodec(IDataProtector& protector);
  [[nodiscard]] StateCodecResult encode(const core::IdentityRecord& identity);
  [[nodiscard]] StateCodecResult decode(std::string_view carrier);

 private:
  IDataProtector& protector_;
};

}  // namespace axlic::windows
