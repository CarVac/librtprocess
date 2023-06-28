# We have to create a label variable if we want to display it in AboutThisBuild.txt...

# This first choice should be used for official releases
set(PROC_TARGET_1_LABEL generic x86 CACHE STRING "Processor-1 label - should be used for official Windows release")
set(PROC_TARGET_1_FLAGS "-mtune=generic" CACHE STRING "Processor-1 flags")

# This second choice should be used for your own build only
set(PROC_TARGET_2_LABEL native CACHE STRING "Processor-2 label - use it for your own build")
if (CMAKE_SYSTEM_PROCESSOR MATCHES "ppc|ppc64|powerpc|powerpc64" OR (APPLE AND CMAKE_OSX_ARCHITECTURES MATCHES "ppc|ppc64"))
  set(PROC_TARGET_2_FLAGS "-mtune=native" CACHE STRING "Processor-2 flags")
else()
  set(PROC_TARGET_2_FLAGS "-march=native" CACHE STRING "Processor-2 flags")
endif()
