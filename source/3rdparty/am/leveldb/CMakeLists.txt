FIND_PACKAGE(Boost ${Boost_FIND_VERSION}
  COMPONENTS thread)

MACRO_CHECK_SHOULD_BUILD(leveldb Boost_FOUND)

IF(leveldb_SHOULD_BUILD)

  FILE(GLOB_RECURSE
    leveldb_SRC
    RELATIVE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "*.cc"
    )


  INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
    ${IZENELIB_SOURCE_DIR}/include/3rdparty/am/
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

  ADD_DEFINITIONS("-fPIC")

  MACRO_ADD_LIBRARY(leveldb SRCS ${leveldb_SRC})

ENDIF(leveldb_SHOULD_BUILD)
