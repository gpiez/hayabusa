set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DMYDEBUG -Wall -Wextra -Wno-missing-field-initializers -pipe" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O3 -DNDEBUG -march=native -Wuninitialized -fno-keep-inline-functions -fno-keep-static-consts -fmerge-all-constants" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -fno-keep-inline-functions -fno-keep-static-consts -fmerge-all-constants -flto --param max-inline-insns-single=2000" CACHE STRING "")
#set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto-partition=none -fwhole-program" CACHE STRING "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-fwhole-program" CACHE STRING "")
SET(CMAKE_CXX_FLAGS "-std=c++0x -flax-vector-conversions -fdiagnostics-show-option $ENV{CXXFLAGS}" CACHE STRING "")

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")

cmake_minimum_required(VERSION 2.6)

PROJECT (hayabusa)

#if (NOT CMAKE_BUILD_TYPE)
 #   set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
#endif(NOT CMAKE_BUILD_TYPE)

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR})
#enable_language(ASM-FASM)

#FIND_PACKAGE(PrecompiledHeaders)

SET(CMAKE_VERBOSE_MAKEFILE ON)

#set_source_files_properties(setpiece.asm OBJECT_DEPENDS offsets.inc)
SET(common_SRCS console.cpp evolution.cpp parameters.cpp selfgame.cpp rootboard.cpp boardbase.cpp workthread.cpp eval.cpp testpositions.cpp
    move.cpp stringlist.cpp history.cpp book.cpp evalinit.cpp)

SET(USE_QTGUI FALSE CACHE BOOL "Build the QT interface")
SET(USE_GENETIC FALSE CACHE BOOL "Use genetic training algorithms")

if (USE_QTGUI)
    set(packages ${packages} QtNetwork)
    SET(hayabusa_MOC_H console.h)
    set(packages ${packages} QtGUI)
    SET(hayabusa_UIS stats.ui)
    SET(hayabusa_MOC_H console.h rootboard.h jobs.h statwidget.h nodemodel.h nodedelegate.h)
    SET(common_SRCS ${common_SRCS} statwidget.cpp  nodeitem.cpp nodemodel.cpp nodedelegate.cpp)
    FIND_PACKAGE(Qt4 COMPONENTS QtCore ${packages} REQUIRED)
else(USE_QTGUI)
    SET(QT_DONT_USE_QTGUI TRUE)
endif(USE_QTGUI)    

SET(hayabusa_SRCS sortedmovelist.cpp main.cpp)
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

IF (USE_QTGUI)
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
ELSE(USE_QTGUI)
    ADD_EXECUTABLE(hayabusa ${common_SRCS} ${hayabusa_SRCS})
    FIND_PACKAGE(Threads REQUIRED)
    TARGET_LINK_LIBRARIES (hayabusa ${CMAKE_THREAD_LIBS_INIT})
ENDIF(USE_QTGUI)

IF (USE_GENETIC)
    ADD_DEFINITIONS(-DUSE_GENETIC)
ENDIF (USE_GENETIC)

#if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
#  add_precompiled_header(hayabusa pch.h)
#  add_dependencies(hayabusa_gch constants.h)
#endif()
