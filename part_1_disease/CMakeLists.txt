cmake_minimum_required(VERSION 3.21)
project(hpc_disease_simulation)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(hpc_disease_simulation
        simulation/main.cpp
        simulation/simulation.cpp)


enable_testing()

add_executable(all_tests
        simulation/test.cpp
        simulation/simulation.cpp)

add_test(NAME run_doctest_all COMMAND all_tests)

if(ENABLE_COVERAGE)
  # set compiler flags
  set(CMAKE_CXX_FLAGS "-O0 -coverage")

  # find required tools
  find_program(LCOV lcov REQUIRED)
  find_program(GENHTML genhtml REQUIRED)

  # add coverage target
  add_custom_target(coverage
            # gather data
            COMMAND make test
            COMMAND ${LCOV} --directory . --capture --output-file coverage.info
            # generate report
            COMMAND ${GENHTML} --demangle-cpp -o coverage coverage.info
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
