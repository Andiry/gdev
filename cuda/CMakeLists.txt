SET(CMAKE_C_FLAGS_RELEASE "-Wall -O2")
SET(CMAKE_C_FLAGS_DEBUG "-g")
SET(CMAKE_BUILD_TYPE Release)
SET(CMAKE_C_FLAGS "-DGDEV_SCHED_DISABLED")

EXECUTE_PROCESS(COMMAND ${CMAKE_C_COMPILER} -dumpversion
                OUTPUT_VARIABLE GCC_VERSION)
if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
    set(CMAKE_CXX_FLAGS "-std=c++11")
else()
    set(CMAKE_CXX_FLAGS "-std=c++0x")
endif()
ADD_DEFINITIONS("-Wno-unused-but-set-variable")

SET(serial "1.0.0")
SET(soserial "1")
include(UseDoxygen OPTIONAL)


# Each source files

## Generate ptxgrammar
FIND_PACKAGE(FLEX)
FIND_PACKAGE(BISON)

## CUDUMP
FILE(GLOB cudump_src "cudump/*.c")
SET(cudump_src ${cudump_src} driver/gdev_cuda.c )
## Driver API
FILE(GLOB util_src "${PROJECT_SOURCE_DIR}/util/*.c")
FILE(GLOB common_src "driver/*.c")
FILE(GLOB common_ext_src "driver/extension/*.c")

FILE(GLOB kcuda_src "kcuda/*.c")
FILE(GLOB ucuda_src "libucuda/*.c")

## Runtime API
### Ocelot each sources
#### analysis
FILE(GLOB analysis_src "runtime/ocelot/analysis/implementation/*.cpp")
#### api
FILE(GLOB api_src "runtime/ocelot/api/implementation/*.cpp")
#### cuda
FILE(GLOB cuda_src "runtime/ocelot/cuda/implementation/*.cpp")
#### executive
FILE(GLOB executive_src "runtime/ocelot/executive/implementation/*.cpp")
#### ir
FILE(GLOB ir_src "runtime/ocelot/ir/implementation/*.cpp")
#### parser
FILE(GLOB parser_src "runtime/ocelot/parser/implementation/*.cpp")
#### transforms
FILE(GLOB transforms_src "runtime/ocelot/transforms/implementation/*.cpp")
### Ocelot all sources
SET(ocelot_src
    ${analysis_src}
    ${api_src}
    ${cuda_src}
    ${executive_src}
    ${ir_src}
    ${parser_src}
    ${transforms_src}
)
### Hydrazine sources
FILE(GLOB hydrazine_src "runtime/hydrazine/implementation/*.cpp")

# select source
SET(ucuda_src
    ${ucuda_src} ${util_src} ${common_src} ${common_ext_src}
    )

IF(runtime)
    SET(ucuda_src
	${ucuda_src}
	${ocelot_src}
	${hydrazine_src}
	)
    MESSAGE("Use CUDA Runtime API")
ELSE(runtime)
    MESSAGE("Not use CUDA Runtime API")
ENDIF(runtime)

# include dir
SET(gdev_inc /usr/local/gdev/include ./ libucuda kcuda ./runtime ${PROJECT_BINARY_DIR} cudump driver)

SET(ucuda_install_headers ${PROJECT_SOURCE_DIR}/cuda/driver/cuda.h )

# parser adjust
ADD_CUSTOM_TARGET(cov
    COMMAND sh ${PROJECT_SOURCE_DIR}/cuda/autogen_cmake.sh ${PROJECT_BINARY_DIR}/ptxgrammar.hpp
    DEPENDS ${PROJECT_BINARY_DIR}/ptxgrammar.cpp ${PROJECT_BINARY_DIR}/ptx.cpp
    COMMENT "Generate ptxgrammar.hpp"
)

FLEX_TARGET(ptx runtime/ocelot/parser/implementation/ptx.ll ${PROJECT_BINARY_DIR}/ptx.cpp)
BISON_TARGET(ptxgrammar runtime/ocelot/parser/implementation/ptxgrammar.yy  ${PROJECT_BINARY_DIR}/ptxgrammar.cpp)
ADD_FLEX_BISON_DEPENDENCY(ptx ptxgrammar)


# execute
INCLUDE_DIRECTORIES(gdev ${gdev_inc})
INCLUDE_DIRECTORIES(cudump ${gdev_inc})
ADD_LIBRARY(ucuda SHARED ${ucuda_src})#  ${FLEX_ptx_OUTPUTS} ${BISON_ptxgrammar_OUTPUTS})
ADD_DEPENDENCIES(ucuda cov)
ADD_EXECUTABLE(cudump ${cudump_src})

SET_TARGET_PROPERTIES(ucuda PROPERTIES VERSION ${serial} SOVERSION ${soserial})
INSTALL(TARGETS ucuda LIBRARY DESTINATION gdev/lib64)
INSTALL(TARGETS cudump DESTINATION gdev/bin)
TARGET_LINK_LIBRARIES(ucuda rt)
INSTALL(FILES ${ucuda_install_headers} DESTINATION gdev/include)
