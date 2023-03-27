cmake_minimum_required(VERSION 3.20)

project(idl-serial VERSION 1.0 LANGUAGES C)

set(SRC_DIR "src")
set(PARSER_DIR "${CMAKE_CURRENT_BINARY_DIR}")

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

set(LEXER_OUT "${PARSER_DIR}/scanner.c")
set(PARSER_OUT "${PARSER_DIR}/parser.c")
set(SRC "cJSON.c" "idl_serial.c" "utils.c")


option(ASAN "Build with address sanitizer" OFF)

if(ASAN) 
    set(CMAKE_BUILD_TYPE Debug)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif(ASAN)

flex_target(LEXER "scanner.l" "${LEXER_OUT}" DEFINES_FILE "${PARSER_DIR}/scanner.h")
bison_target(PARSER "parser.y" "${PARSER_OUT}" DEFINES_FILE "${PARSER_DIR}/parser.h")
add_flex_bison_dependency(LEXER PARSER)

add_library(idl-serial SHARED "${LEXER_OUT}" "${PARSER_OUT}" "${SRC}")

target_include_directories(idl-serial PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(idl-serial PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

add_executable(idl-serial-code-gen ${CMAKE_CURRENT_SOURCE_DIR}/main.c)
target_include_directories(idl-serial-code-gen PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(idl-serial-code-gen PRIVATE idl-serial)

install(TARGETS idl-serial LIBRARY DESTINATION lib)
install(TARGETS idl-serial-code-gen DESTINATION bin)