enable_testing()
set(CMAKE_CTEST_COMMAND ctest -V)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

set(BIN $<TARGET_FILE:sjasmplus>)
set(TESTDRIVER "${CMAKE_CURRENT_SOURCE_DIR}/testDriver.hs")

function(testOutput SUBDIR)
  set(SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}")
  file(GLOB files "${SRCDIR}/*.asm")
  foreach(file ${files})
    file(RELATIVE_PATH f ${CMAKE_CURRENT_SOURCE_DIR} ${file})
    string(REPLACE "/" "_" f ${f})
    add_test(NAME "Test_${f}" COMMAND ${TESTDRIVER} raw ${BIN} ${file}
      "${PROJECT_BINARY_DIR}/tests")
  endforeach()
endfunction()

testOutput("dup")

add_dependencies(check sjasmplus)
