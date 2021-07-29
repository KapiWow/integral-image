add_executable(
  image_integrator_test
  image_integrator_test.cc
)

target_link_libraries(
  image_integrator_test
  gtest_main
  gtest
  image_integrator
)

include(GoogleTest)
gtest_discover_tests(image_integrator_test)

