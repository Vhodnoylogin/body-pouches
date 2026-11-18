cmake_minimum_required(VERSION 3.24)
project(BodyPouches2Prototype VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(RC4_NATIVE "" CACHE PATH "Completed VR Interaction Framework RC4 native build")
set(FRAMEWORK_ROOT "" CACHE PATH "Pinned public VR Interaction Framework source")
set(HIGGS_ROOT "" CACHE PATH "Pinned public HIGGS API source")
set(JSON_INCLUDE "" CACHE PATH "Optional directory containing nlohmann/json.hpp")
set(FRAMEWORK_COMMIT "fac67fdedb1591bf20bebf61016402a7e7b45f26")
set(HIGGS_COMMIT "ec452367c01617c4eb0e9e039697d0221f788d13")

find_package(Git REQUIRED)
foreach(dependency IN ITEMS FRAMEWORK HIGGS)
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${${dependency}_ROOT}" rev-parse HEAD
        OUTPUT_VARIABLE actual_revision OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE revision_result)
    if(NOT revision_result EQUAL 0 OR NOT actual_revision STREQUAL "${${dependency}_COMMIT}")
        message(FATAL_ERROR "${dependency}_ROOT must be at ${${dependency}_COMMIT}")
    endif()
endforeach()
if(NOT EXISTS "${JSON_INCLUDE}/nlohmann/json.hpp")
    message(FATAL_ERROR "JSON_INCLUDE must identify the pinned nlohmann single_include directory")
endif()

if(NOT EXISTS "${RC4_NATIVE}/CMakeCache.txt")
    message(FATAL_ERROR "RC4_NATIVE must identify the completed Release native build")
endif()
file(STRINGS "${RC4_NATIVE}/CMakeCache.txt" dependency_paths
    REGEX "^FETCHCONTENT_SOURCE_DIR_(COMMONLIBSSE|FMT|SPDLOG|RAPIDCSV):PATH=")
foreach(line IN LISTS dependency_paths)
    if(line MATCHES "^FETCHCONTENT_SOURCE_DIR_([A-Z]+):PATH=(.*)$")
        set(DEP_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")
    endif()
endforeach()
foreach(dependency IN ITEMS COMMONLIBSSE FMT SPDLOG RAPIDCSV)
    if(NOT IS_DIRECTORY "${DEP_${dependency}}")
        message(FATAL_ERROR "RC4 cache has no usable ${dependency} source path")
    endif()
endforeach()

# Consume public contracts and the pinned SDK helper, never provider internals.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/contracts")
foreach(header IN ITEMS higgsinterface001.h higgsabitypes.h higgshandapi.h higgsinput.h
        higgstransactions.h higgsphysics.h higgsobservations.h higgsnativeinterop.h)
    configure_file("${HIGGS_ROOT}/include/${header}"
        "${CMAKE_CURRENT_BINARY_DIR}/contracts/${header}" COPYONLY)
endforeach()

get_filename_component(POUCHES_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
file(GLOB potion_core_sources CONFIGURE_DEPENDS "${POUCHES_ROOT}/src/core/*.cpp")
add_library(BodyPouches2Prototype SHARED runtime.cpp potion_policy.cpp potion_observer.cpp settings.cpp assignment_store.cpp
    ${potion_core_sources} "${POUCHES_ROOT}/src/game/Forms.cpp"
    "${FRAMEWORK_ROOT}/examples/ordinary_inventory_provider.cpp")
set_target_properties(BodyPouches2Prototype PROPERTIES OUTPUT_NAME BodyPouches SUFFIX .dll)
target_include_directories(BodyPouches2Prototype PRIVATE
    "${POUCHES_ROOT}/src"
    "${FRAMEWORK_ROOT}/src/compat" "${FRAMEWORK_ROOT}/include" "${FRAMEWORK_ROOT}/examples"
    "${CMAKE_CURRENT_BINARY_DIR}/contracts" "${DEP_COMMONLIBSSE}/include"
    "${DEP_FMT}/include" "${DEP_SPDLOG}/include" "${DEP_RAPIDCSV}/src" "${JSON_INCLUDE}")
target_compile_definitions(BodyPouches2Prototype PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    ENABLE_SKYRIM_SE=1 ENABLE_SKYRIM_AE=1 ENABLE_SKYRIM_VR=1 HAS_SKYRIM_MULTI_TARGETING=1
    SPDLOG_COMPILED_LIB SPDLOG_FMT_EXTERNAL _CRT_SECURE_NO_WARNINGS
    BODYPOUCHES_PROTO_VERSION="2.0.0-prototype.6")
target_compile_options(BodyPouches2Prototype PRIVATE
    /W4 /permissive- /EHsc /utf-8 /wd4200 /wd4201 /wd4324)
target_link_libraries(BodyPouches2Prototype PRIVATE
    "${RC4_NATIVE}/_deps/commonlibsse-build/Release/CommonLibSSE.lib"
    "${RC4_NATIVE}/_deps/fmt-build/Release/fmt.lib"
    "${RC4_NATIVE}/_deps/spdlog-build/Release/spdlog.lib"
    version bcrypt dbghelp)
