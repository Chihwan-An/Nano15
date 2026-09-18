sknano_add_analysis_module(
    NAME MuonCheck
    HEADERS include/MuonCheck/MuonCheck.h
    SOURCES src/MuonCheck.cc
    LINKDEF include/MuonCheck/LinkDef.hpp
    ANALYZERS MuonCheck)
target_include_directories(MuonCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
