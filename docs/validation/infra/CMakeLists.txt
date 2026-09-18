sknano_add_analysis_module(
    NAME InfraCheck
    HEADERS include/InfraCheck/InfraCheck.h
    SOURCES src/InfraCheck.cc
    LINKDEF include/InfraCheck/LinkDef.hpp
    ANALYZERS InfraCheck)
target_include_directories(InfraCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
