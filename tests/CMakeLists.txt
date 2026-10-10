# The checks of the core.
#
# They link against BodyPouchesCore and nothing else - no CommonLib, no SKSE, no
# game - which is the point: the rule of the pouches can be run on any machine in
# a second, and a mistake in it is found before a headset is ever put on.
project(BodyPouchesTests LANGUAGES CXX)

add_executable(bodypouches_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/Harness.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/FakePack.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_filter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_pouchset.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_slotplan.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_pendinggrab.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_sharedinteraction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_currentupdaterelease.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_buttonedges.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_trace.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_assignments.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

target_include_directories(bodypouches_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(bodypouches_tests PRIVATE BodyPouchesCore)

if(MSVC)
    target_compile_options(bodypouches_tests PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
endif()

enable_testing()
add_test(NAME core COMMAND bodypouches_tests)

# Compile the production form adapter against a small engine boundary fixture.
# It deliberately needs neither the game nor the CommonLib binary.
add_executable(bodypouches_form_tests main.cpp test_forms.cpp ../src/game/Forms.cpp)
target_include_directories(bodypouches_form_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/form-fixture" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(bodypouches_form_tests PRIVATE BodyPouchesCore)
if(MSVC)
    target_compile_options(bodypouches_form_tests PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
endif()
add_test(NAME forms COMMAND bodypouches_form_tests)

add_executable(bodypouches_body_tests main.cpp test_body.cpp ../src/game/Body.cpp)
target_include_directories(bodypouches_body_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/body-fixture" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(bodypouches_body_tests PRIVATE BodyPouchesCore)
if(MSVC)
    target_compile_options(bodypouches_body_tests PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
endif()
add_test(NAME body COMMAND bodypouches_body_tests)
