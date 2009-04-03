PROJECT(ProcMemInfo)

include_directories ( 
		../../include/util
		)
# This section build the libraries
SET(CPP ProcMemInfo.cpp)
        
SET(LIBRARY_OUTPUT_PATH  ../../lib)

ADD_LIBRARY(procmeminfo_shared SHARED ${CPP})
SET_TARGET_PROPERTIES(procmeminfo_shared PROPERTIES OUTPUT_NAME "procmeminfo")
SET_TARGET_PROPERTIES(procmeminfo_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)

ADD_LIBRARY(procmeminfo_static STATIC ${CPP})
SET_TARGET_PROPERTIES(procmeminfo_static PROPERTIES OUTPUT_NAME "procmeminfo")
SET_TARGET_PROPERTIES(procmeminfo_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

