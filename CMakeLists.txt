set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DMYDEBUG -Wall -Wextra -Wno-missing-field-initializers -pipe" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O3 -DNDEBUG -march=core2 -Wuninitialized -fno-keep-inline-functions -fno-keep-static-consts -fmerge-all-constants" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=core2 -fno-keep-inline-functions -fno-keep-static-consts -fmerge-all-constants -flto" CACHE STRING "")
#set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto-partition=none -fwhole-program" CACHE STRING "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-fwhole-program" CACHE STRING "")
# SSSE3 (Note the additional S) is mandatory because of the pshufb instruction.
SET(CMAKE_CXX_FLAGS "-std=c++0x -mssse3 -flax-vector-conversions -fdiagnostics-show-option $ENV{CXXFLAGS}" CACHE STRING "")

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")

cmake_minimum_required(VERSION 2.6)

PROJECT (hayabusa)

#if (NOT CMAKE_BUILD_TYPE)
 #   set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
#endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR})
#enable_language(ASM-FASM)

#FIND_PACKAGE(PrecompiledHeaders)

SET(CMAKE_VERBOSE_MAKEFILE ON)

#set_source_files_properties(setpiece.asm OBJECT_DEPENDS offsets.inc)
SET(common_SRCS rootboard.cpp boardbase.cpp console.cpp workthread.cpp eval.cpp testpositions.cpp
    move.cpp testgame.cpp stringlist.cpp)

SET(USE_QTGUI FALSE CACHE BOOL "Build the QT interface")
SET(USE_QTNETWORK FALSE CACHE BOOL "Build the server")

if (USE_QTNETWORK)
    set(packages ${packages} QtNetwork)
    SET(hayabusa_MOC_H console.h)
endif(USE_QTNETWORK)

if (USE_QTGUI)
    set(packages ${packages} QtGUI)
    SET(hayabusa_UIS stats.ui)
    SET(hayabusa_MOC_H console.h rootboard.h jobs.h statwidget.h nodemodel.h nodedelegate.h)
    SET(common_SRCS ${common_SRCS} statwidget.cpp  nodeitem.cpp nodemodel.cpp nodedelegate.cpp)
else(USE_QTGUI)
    SET(QT_DONT_USE_QTGUI TRUE)
endif(USE_QTGUI)    

if (USE_QTGUI OR USE_QTNETWORK)
    FIND_PACKAGE(Qt4 COMPONENTS QtCore ${packages} REQUIRED)
ENDIF(USE_QTGUI OR USE_QTNETWORK)

SET(hayabusa_SRCS main.cpp)
SET(hayabusa_MOC_H ${hayabusa_MOC_H})
SET(hayabusa_RCCS hayabusa.qrc)

SET(test_SRCS test.cpp)
SET(test_MOC_H test.h ${hayabusa_MOC_H})

#SET(offsets_SRCS offsets.cpp)
#set_source_files_properties(offsets.cpp PROPERTIES COMPILE_FLAGS "-Wno-invalid-offsetof")
#add_executable(offsets ${offsets_SRCS})

#set_source_files_properties(offsets.inc PROPERTIES generated true)
#add_custom_command(
#    OUTPUT offsets.inc
#    COMMAND offsets > offsets.inc
#    DEPENDS ${offsets_SRCS} offsets boardbase.h)

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

IF (USE_QTGUI OR USE_QTNETWORK)
    ADD_DEFINITIONS(${QT_DEFINITIONS})
    INCLUDE(${QT_USE_FILE})
    QT4_WRAP_CPP( hayabusa_MOC_SRCS ${hayabusa_MOC_H} )
    QT4_WRAP_CPP( test_MOC_SRCS ${test_MOC_H} )
    QT4_WRAP_UI(hayabusa_UIS_H ${hayabusa_UIS})
    QT4_ADD_RESOURCES(hayabusa_RCC_SRCS ${hayabusa_RCCS})
    ADD_EXECUTABLE(hayabusa ${common_SRCS} ${hayabusa_SRCS} ${hayabusa_UIS_H} ${hayabusa_RCC_SRCS} ${hayabusa_MOC_SRCS})
    ADD_EXECUTABLE(test ${common_SRCS} ${test_SRCS} ${test_MOC_SRCS} ${hayabusa_UIS_H} ${hayabusa_RCC_SRCS} )
    set_source_files_properties(test.cpp PROPERTIES COMPILE_FLAGS "-DQT_TEST")
    TARGET_LINK_LIBRARIES (hayabusa ${QT_LIBRARIES})
    TARGET_LINK_LIBRARIES (test ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY})
#    target_link_libraries(offsets ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY})
ELSE(USE_QTGUI OR USE_QTNETWORK)
    ADD_EXECUTABLE(hayabusa ${common_SRCS} ${hayabusa_SRCS})
    IF (CMAKE_SYSTEM_NAME STREQUAL "Windows")
        FIND_PACKAGE(Boost COMPONENTS Thread)
        TARGET_LINK_LIBRARIES (hayabusa -lboost_thread )
    ELSE(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        FIND_PACKAGE(Threads REQUIRED)
#       disable static linking for now        
#        SET(CMAKE_CXX_LINK_EXECUTABLE ld.gold)
#        TARGET_LINK_LIBRARIES (hayabusa -static -Wl,--whole-archive ${CMAKE_THREAD_LIBS_INIT} -Wl,--no-whole-archive)
        TARGET_LINK_LIBRARIES (hayabusa ${CMAKE_THREAD_LIBS_INIT})
    ENDIF(CMAKE_SYSTEM_NAME STREQUAL "Windows")    
ENDIF(USE_QTGUI OR USE_QTNETWORK)

#if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
#  add_precompiled_header(hayabusa pch.h)
#  add_dependencies(hayabusa_gch constants.h)
#endif()
find_library(HUGE_PAGES hugetlbfs)

if(HUGE_PAGES)
    SET(HUGE_MEM TRUE CACHE BOOL "Use huge pages for transposition tables")
    SET(HUGE_BDT FALSE CACHE BOOL "Link BSS, DATA, TEXT into huge pages")
    if (HUGE_MEM)
        ADD_DEFINITIONS(-DHAVE_HUGE_PAGES)
        TARGET_LINK_LIBRARIES(hayabusa hugetlbfs dl)
        IF (USE_QTGUI OR USE_QTNETWORK)
            TARGET_LINK_LIBRARIES(test hugetlbfs dl)
        ENDIF(USE_QTGUI OR USE_QTNETWORK)    
        find_path(HUGETLBFS_LD_PATH ld.hugetlbfs PATH_SUFFIXES share/libhugetlbfs)
        if(HUGE_BDT)
            SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -B ${HUGETLBFS_LD_PATH} -Wl,-hugetlbfs-align")
        endif(HUGE_BDT)
    endif()
ENDIF(HUGE_PAGES)
