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
    get_filename_component(file_we ${file} NAME_WE)
    if(EXISTS "${SRCDIR}/${file_we}.out")
      set(TEST_TYPE "raw")
    else()
      set(TEST_TYPE "sna")
    endif()
    add_test(NAME "Test_${f}" COMMAND ${TESTDRIVER} ${TEST_TYPE} ${BIN} ${file}
      "${PROJECT_BINARY_DIR}/tests")
  endforeach()
endfunction()

testOutput("dup")
testOutput("sna")

add_dependencies(check sjasmplus)
