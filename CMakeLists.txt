cmake_minimum_required(VERSION 2.6)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

PROJECT (hayabusa)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR})
enable_language(ASM-FASM)

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DMYDEBUG -Wall -Wextra -Wno-missing-field-initializers -pipe")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=core2 -fno-exceptions -Wuninitialized")

# SSSE3 (Note the additional S) is mandatory because of the pshufb instruction.
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -mssse3 -flax-vector-conversions -fdiagnostics-show-option $ENV{CXXFLAGS}")

FIND_PACKAGE(PrecompiledHeaders)

SET(CMAKE_VERBOSE_MAKEFILE ON)

set_source_files_properties(setpiece.asm OBJECT_DEPENDS offsets.inc)
SET(common_SRCS rootboard.cpp boardbase.cpp console.cpp workthread.cpp eval.cpp testpositions.cpp
    move.cpp testgame.cpp stringlist.cpp)

SET(USE_QTGUI FALSE CACHE BOOL "Build the QT interface")
IF (USE_QTGUI)
    FIND_PACKAGE(Qt4 COMPONENTS QtCore REQUIRED)
    SET(QT_DONT_USE_QTGUI FALSE)
    SET(hayabusa_UIS stats.ui)
    SET(hayabusa_MOC_H rootboard.h)
    SET(common_SRCS ${common_SRCS} statwidget.cpp  nodeitem.cpp nodemodel.cpp nodedelegate.cpp)
ELSE(USE_QTGUI)
    SET(QT_DONT_USE_QTGUI TRUE)
ENDIF(USE_QTGUI)

SET(hayabusa_SRCS main.cpp)
SET(hayabusa_MOC_H ${hayabusa_MOC_H} jobs.h console.h statwidget.h nodemodel.h nodedelegate.h)
SET(hayabusa_RCCS hayabusa.qrc)

SET(test_SRCS test.cpp)
SET(test_MOC_H test.h ${hayabusa_MOC_H})

SET(offsets_SRCS offsets.cpp)
set_source_files_properties(offsets.cpp PROPERTIES COMPILE_FLAGS "-Wno-invalid-offsetof")
add_executable(offsets ${offsets_SRCS})

set_source_files_properties(offsets.inc PROPERTIES generated true)
add_custom_command(
    OUTPUT offsets.inc
    COMMAND offsets > offsets.inc
    DEPENDS ${offsets_SRCS} offsets boardbase.h)

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
    target_link_libraries(offsets ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY})
ELSE(USE_QTGUI)
    FIND_PACKAGE(Threads REQUIRED)
    ADD_EXECUTABLE(hayabusa ${common_SRCS} ${hayabusa_SRCS})
    TARGET_LINK_LIBRARIES (hayabusa ${CMAKE_THREAD_LIBS_INIT})
ENDIF(USE_QTGUI)

add_precompiled_header(hayabusa pch.h)

find_library(HUGE_PAGES hugetlbfs)

if(HUGE_PAGES)
    SET(HUGE_MEM TRUE CACHE BOOL "Use huge pages for transposition tables")
    SET(HUGE_BDT FALSE CACHE BOOL "Link BSS, DATA, TEXT into huge pages")
    if (HUGE_MEM)
        ADD_DEFINITIONS(-DHAVE_HUGE_PAGES)
        TARGET_LINK_LIBRARIES(hayabusa hugetlbfs dl)
        find_path(HUGETLBFS_LD_PATH ld.hugetlbfs PATH_SUFFIXES share/libhugetlbfs)
        if(HUGE_BDT)
            SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -B ${HUGETLBFS_LD_PATH} -Wl,-hugetlbfs-align")
        endif(HUGE_BDT)
    endif()
ENDIF(HUGE_PAGES)
