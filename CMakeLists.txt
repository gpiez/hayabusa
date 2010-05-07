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

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DMYDEBUG -Wall -Wextra -Wno-missing-field-initializers -Wno-parentheses")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=core2 -fno-exceptions -Winline")

# SSSE3 (Note the additional S) is mandatory because of the pshufb instruction.
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mssse3")

FIND_PACKAGE(Qt4 COMPONENTS QtCore REQUIRED)
FIND_PACKAGE(PrecompiledHeaders)

find_library(HUGE_PAGES hugetlbfs)

if(HUGE_PAGES)
    SET(HUGE_BDT FALSE CACHE BOOL "Link BSS, DATA, TEXT into huge pages")
	ADD_DEFINITIONS(-DHAVE_HUGE_PAGES)
	TARGET_LINK_LIBRARIES(hayabusa hugetlbfs)
	find_path(HUGETLBFS_LD_PATH ld.hugetlbfs PATH_SUFFIXES share/libhugetlbfs)
	if(HUGE_BDT)
	    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -B ${HUGETLBFS_LD_PATH} -Wl,--hugetlbfs-link=BDT")
	endif(HUGE_BDT)
ENDIF(HUGE_PAGES)

#IF(CMAKE_COMPILER_IS_GNUCC)
#  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0 -fdiagnostics-show-location=every-line")
#ENDIF(CMAKE_COMPILER_IS_GNUCC)
#IF(CMAKE_COMPILER_IS_GNUCXX)
#  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0 -fdiagnostics-show-location=every-line")
#ENDIF(CMAKE_COMPILER_IS_GNUCXX)

SET(USE_QTGUI FALSE CACHE BOOL "Build the QT interface")
IF (USE_QTGUI)
	SET(QT_DONT_USE_QTGUI FALSE)
ELSE(USE_QTGUI)
	SET(QT_DONT_USE_QTGUI TRUE)
ENDIF(USE_QTGUI)

ADD_DEFINITIONS(${QT_DEFINITIONS})
INCLUDE(${QT_USE_FILE})

SET(CMAKE_VERBOSE_MAKEFILE ON)

SET(hayabusa_SRCS rootboard.cpp joblist.cpp boardbase.cpp console.cpp workthread.cpp eval.cpp
                  move.cpp transpositiontable.cpp setpiece.asm offsets.inc statwidget.cpp)
SET(hayabusa_MOC_H jobs.h console.h statwidget.h)
SET(hayabusa_UIS stats.ui)
SET(hayabusa_RCCS hayabusa.qrc)

SET(test_SRCS test.cpp ${hayabusa_SRCS})
SET(hayabusa_SRCS main.cpp ${hayabusa_SRCS})
SET(test_MOC_H test.h ${hayabusa_MOC_H})

SET(offsets_SRCS offsets.cpp)
set_source_files_properties(offsets.cpp PROPERTIES COMPILE_FLAGS "-Wno-invalid-offsetof")
add_executable(offsets ${offsets_SRCS})
target_link_libraries(offsets ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY})

#INCLUDE_DIRECTORIES(${QT_LIBRARY_DIR})

set_source_files_properties(offsets.inc PROPERTIES generated true)
add_custom_command(
    OUTPUT offsets.inc 
    COMMAND offsets > offsets.inc
    DEPENDS ${offsets_SRCS} offsets boardbase.h)

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

QT4_WRAP_CPP( hayabusa_MOC_SRCS ${hayabusa_MOC_H} )
QT4_WRAP_CPP( test_MOC_SRCS ${test_MOC_H} )
QT4_WRAP_UI(hayabusa_UIS_H ${hayabusa_UIS})
QT4_ADD_RESOURCES(hayabusa_RCC_SRCS ${hayabusa_RCCS})
# QT4_AUTOMOC(${hayabusa_SRCS})
ADD_EXECUTABLE(hayabusa ${hayabusa_SRCS} ${hayabusa_UIS_H} ${hayabusa_RCC_SRCS} ${hayabusa_MOC_SRCS})
add_precompiled_header(hayabusa pch.h)

TARGET_LINK_LIBRARIES (hayabusa ${QT_LIBRARIES})

ADD_EXECUTABLE(test ${test_SRCS} ${test_MOC_SRCS})
set_source_files_properties(test.cpp PROPERTIES COMPILE_FLAGS "-DQT_TEST")
TARGET_LINK_LIBRARIES (test ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY})
