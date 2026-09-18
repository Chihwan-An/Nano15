sknano_add_analysis_module(
    NAME FixCheck
    HEADERS include/FixCheck/FixCheck.h
    SOURCES src/FixCheck.cc
    LINKDEF include/FixCheck/LinkDef.hpp
    ANALYZERS FixCheck)
target_include_directories(FixCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
