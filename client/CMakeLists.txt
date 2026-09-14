include(FetchContent)
include(${PROJECT_SOURCE_DIR}/cmake/AxlicWarnings.cmake)

FetchContent_Declare(
  nlohmann_json
  URL https://codeload.github.com/nlohmann/json/tar.gz/55f93686c01528224f448c19128836e7df245f72
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(axlic_core STATIC
  src/core/runtime_license.cpp
  src/wire/credential.cpp
)
target_compile_features(axlic_core PUBLIC cxx_std_20)
target_include_directories(axlic_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(axlic_core PRIVATE bcrypt)
axlic_enable_warnings(axlic_core)

add_library(axlic_a0_fixture_adapter STATIC src/core/a0_fixture_adapter.cpp)
target_compile_features(axlic_a0_fixture_adapter PUBLIC cxx_std_20)
target_include_directories(axlic_a0_fixture_adapter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(axlic_a0_fixture_adapter PUBLIC axlic_core PRIVATE nlohmann_json::nlohmann_json)
axlic_enable_warnings(axlic_a0_fixture_adapter)

add_executable(axlic app/main.cpp)
target_link_libraries(axlic PRIVATE axlic_core axlic_a0_fixture_adapter nlohmann_json::nlohmann_json)
target_compile_features(axlic PRIVATE cxx_std_20)
target_compile_definitions(axlic PRIVATE AXLIC_A0_FIXTURE_ADAPTER=1)
axlic_enable_warnings(axlic)

if(BUILD_TESTING)
  FetchContent_Declare(
    Catch2
    URL https://codeload.github.com/catchorg/Catch2/tar.gz/8b08d4d79514f45f7e4ce2a607ac9c94e920d1bb
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(Catch2)

  add_executable(axlic_unit_tests
    tests/test_credential.cpp
    tests/test_runtime_license.cpp
  )
  target_link_libraries(axlic_unit_tests PRIVATE axlic_core Catch2::Catch2WithMain nlohmann_json::nlohmann_json)
  target_compile_features(axlic_unit_tests PRIVATE cxx_std_20)
  target_compile_definitions(axlic_unit_tests PRIVATE AXLIC_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
  axlic_enable_warnings(axlic_unit_tests)

  include(Catch)
  catch_discover_tests(axlic_unit_tests)

  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(
    NAME a0_cli_contract
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cli.py
      --exe $<TARGET_FILE:axlic>
      --vectors ${PROJECT_SOURCE_DIR}/reference/vectors/a0/corpus.json
      --valid-state ${PROJECT_SOURCE_DIR}/reference/vectors/a0/valid_state.json
      --absent-state ${PROJECT_SOURCE_DIR}/reference/vectors/a0/absent_state.json
  )
  add_test(
    NAME a0_offline_boundary
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_offline_boundary.py
      --exe $<TARGET_FILE:axlic>
      --compiler ${CMAKE_CXX_COMPILER}
      --state ${PROJECT_SOURCE_DIR}/reference/vectors/a0/valid_state.json
  )
endif()
