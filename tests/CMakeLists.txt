enable_testing()

find_package(Catch2 3 REQUIRED)

set(GENERATING_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/gen_z80tests.py)
set(GENERATED_FILE ${CMAKE_CURRENT_BINARY_DIR}/z80_gen.inc.h)

add_custom_command(
        OUTPUT ${GENERATED_FILE}
        COMMAND python3 ${GENERATING_SCRIPT} > ${GENERATED_FILE}
        DEPENDS ${GENERATING_SCRIPT}
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(z80tests z80tests.cpp ${GENERATED_FILE})
target_link_libraries(z80tests PRIVATE Catch2::Catch2WithMain sjasmplus_lib ${LINK_LIBS})

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS z80tests)

include(CTest)
include(Catch)
catch_discover_tests(z80tests)
