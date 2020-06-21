include(GoogleTest)

macro(package_add_test TESTNAME)
    # create an exectuable in which the tests will be stored
    add_executable(${TESTNAME} ${ARGN})
    # link the Google test infrastructure, mocking library, and a default main fuction to
    # the test executable.  Remove g_test_main if writing your own main function.
    target_link_libraries(${TESTNAME} gtest gmock gtest_main)
    # gtest_discover_tests replaces gtest_add_tests,
    # see https://cmake.org/cmake/help/v3.10/module/GoogleTest.html for more options to pass to it
    gtest_discover_tests(${TESTNAME}
        # set a working directory so your project root so that you can find test data via paths relative to the project root
        WORKING_DIRECTORY ${PROJECT_DIR}
        PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
    )
    set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
endmacro()

# ide layout
set(CMAKE_FOLDER foundation/test)

package_add_test(MovieTest
	movie_test.cpp)

target_link_libraries(MovieTest
	CodecFoundationSession
)


# add_executable(MovieTest
#	MovieUnitTest.cpp
#)

#gtest_add_tests(TARGET      MovieTest
#                TEST_SUFFIX .noArgs
#                TEST_LIST   noArgsTests
#)
#set_tests_properties(${noArgsTests}   PROPERTIES TIMEOUT 10)
#set_tests_properties(${withArgsTests} PROPERTIES TIMEOUT 20)

set_target_properties(gtest PROPERTIES FOLDER extern)
set_target_properties(gtest_main PROPERTIES FOLDER extern)
set_target_properties(gmock PROPERTIES FOLDER extern)
set_target_properties(gmock_main PROPERTIES FOLDER extern)
