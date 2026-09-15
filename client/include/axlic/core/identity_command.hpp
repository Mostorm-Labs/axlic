#pragma once

#include "axlic/core/identity.hpp"

#include <string>

namespace axlic::core {

struct RenderedCommand {
  std::string json;
  int exit_code{};
};

[[nodiscard]] RenderedCommand render_identity_result(const IdentityResult& result);

}  // namespace axlic::core
