sknano_add_analysis_module(
    NAME EleCheck
    HEADERS include/EleCheck/EleCheck.h
    SOURCES src/EleCheck.cc
    LINKDEF include/EleCheck/LinkDef.hpp
    ANALYZERS EleCheck)
target_include_directories(EleCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
