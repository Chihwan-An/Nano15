sknano_add_analysis_module(
    NAME FatJetCheck
    HEADERS include/FatJetCheck/FatJetCheck.h
    SOURCES src/FatJetCheck.cc
    LINKDEF include/FatJetCheck/LinkDef.hpp
    ANALYZERS FatJetCheck)
target_include_directories(FatJetCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
