SET(CMAKE_C_FLAGS_RELEASE "-Wall -O2")
SET(CMAKE_C_FLAGS_DEBUG "-g")
SET(CMAKE_BUILD_TYPE Release)
SET(CMAKE_C_FLAGS "-DGDEV_SCHED_DISABLED")

SET(serial "1.0.0")
SET(soserial "1")
include(UseDoxygen OPTIONAL)

ADD_DEFINITIONS("-Wno-unused-but-set-variable")

## Each source files
set(common_src
    ${PROJECT_SOURCE_DIR}/common/gdev_api.c
    ${PROJECT_SOURCE_DIR}/common/gdev_device.c
    ${PROJECT_SOURCE_DIR}/common/gdev_nvidia.c
    ${PROJECT_SOURCE_DIR}/common/gdev_nvidia_compute.c
    ${PROJECT_SOURCE_DIR}/common/gdev_nvidia_fifo.c
    ${PROJECT_SOURCE_DIR}/common/gdev_nvidia_mem.c
    ${PROJECT_SOURCE_DIR}/common/gdev_nvidia_nvc0.c
    ${PROJECT_SOURCE_DIR}/common/gdev_nvidia_nve4.c
    ${PROJECT_SOURCE_DIR}/common/gdev_nvidia_shm.c
    ${PROJECT_SOURCE_DIR}/common/gdev_sched.c
)
file(GLOB util_src "${PROJECT_SOURCE_DIR}/util/*.c")
file(GLOB nvrm_src "user/nvrm/*.c")
file(GLOB user_src "user/gdev/*.c")
file(GLOB nouveau_src "user/nouveau/*.c")
file(GLOB kernel_src "kernel/*.c")


## include dir
SET(gdev_inc /usr/local/gdev/include)

## select source 
IF(user)
    MESSAGE(Selected\ User-space!)
    SET(gdev_src ${gdev_src} ${user_src} ${common_src} ${util_src})
    SET(gdev_inc ${gdev_inc} 
	/usr/include/libdrm 
	${PROJECT_SOURCE_DIR}/common/ 
	${PROJECT_SOURCE_DIR}/lib/user/gdev/
	)

    IF(driver STREQUAL nouveau)
	SET(gdev_src ${gdev_src} ${nouveau_src})
	SET(gdev_inc ${gdev_inc} ${PROJECT_SOURCE_DIR}/lib/user/gdev/nouveau)
	SET(link_lib drm drm_nouveau)
    ELSE(driver STREQUAL nouveau)
	IF(driver STREQUAL nvrm)
	    SET(gdev_src ${gdev_src} ${nvrm_src} )
	    SET(gdev_inc ${gdev_inc} ${PROJECT_SOURCE_DIR}/lib/user/gdev/nvrm)
	ELSE(driver STREQUAL nvrm)
	    IF(driver STREQUAL pscnv)
		SET(gdev_src ${gdev_src} ${pscnv_src})
		SET(gdev_inc ${gdev_inc} ${PROJECT_SOURCE_DIR}/lib/user/gdev/pscnv)
	    ELSE(driver STREQUAL  pscnv)
		MESSAGE( FATAL_ERROR "Not selected GPU Driver.")
		MESSAGE( FATAL_ERROR "ex: driver=nouveau.")
	    ENDIF(driver STREQUAL pscnv)
	ENDIF(driver STREQUAL nvrm)
    ENDIF(driver STREQUAL nouveau)
    SET(gdev_install_headers 
	${PROJECT_SOURCE_DIR}/common/gdev_api.h 
	${PROJECT_SOURCE_DIR}/common/gdev_nvidia_def.h 
	${PROJECT_SOURCE_DIR}/common/gdev_list.h 
	${PROJECT_SOURCE_DIR}/common/gdev_time.h)
ELSE(user)
    MESSAGE(Selected\ Kernel-space!)
    SET(gdev_src ${gdev_src} ${kernel_src})
    SET(gdev_inc ${gdev_inc} ${PROJECT_SOURCE_DIR}/lib/kernel ${PROJECT_SOURCE_DIR}/common)
    SET(gdev_install_headers 
	${PROJECT_SOURCE_DIR}/common/gdev_api.h 
	${PROJECT_SOURCE_DIR}/common/gdev_nvidia_def.h 
	${PROJECT_SOURCE_DIR}/common/gdev_list.h 
	${PROJECT_SOURCE_DIR}/common/gdev_time.h)
ENDIF(user)

INCLUDE_DIRECTORIES(gdev ${gdev_inc})
ADD_LIBRARY(gdev SHARED ${gdev_src})
SET_TARGET_PROPERTIES(gdev PROPERTIES VERSION ${serial} SOVERSION ${soserial})
TARGET_LINK_LIBRARIES(gdev ${link_lib})
INSTALL(TARGETS gdev LIBRARY DESTINATION gdev/lib64)
INSTALL(FILES ${gdev_install_headers} DESTINATION gdev/include)

