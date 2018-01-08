cmake_minimum_required(VERSION 3.2)

set(CMAKE_C_FLAGS "-Wall -std=c99 -fno-asynchronous-unwind-tables -ffunction-sections -Wl,--gc-sections -fPIC")

# 3rd party libraries (duktape)
include(CMakeLists-ExternalProjects.txt)

include_directories(${DUK_DIR}/src ${DUK_DIR}/extras/print-alert)

add_library(machines SHARED
        machines.c
        objects.c
        ${DUK_DIR}/src/duktape.c
        ${DUK_DIR}/extras/print-alert/duk_print_alert.c)

set_target_properties(machines PROPERTIES PUBLIC_HEADER "machines.h")

add_dependencies(machines duktape)

target_include_directories(machines PUBLIC ${CMAKE_SOURCE_DIR})

add_executable(mdemo main.c)
target_link_libraries(mdemo machines m)

function(cat IN_FILE OUT_FILE)
    file(READ ${IN_FILE} CONTENTS)
    file(APPEND ${OUT_FILE} "${CONTENTS}")
endfunction()

# combine our js files into the big dude
set(JSFILES "js/match.js" "js/sandbox.js" "js/step.js" "driver.js")
foreach(JSFILE ${JSFILES})
    cat(${JSFILE} machines.js)
endforeach()

install(TARGETS machines
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include)

install(TARGETS mdemo
        DESTINATION bin)