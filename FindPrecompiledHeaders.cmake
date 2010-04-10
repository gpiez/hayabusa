# Assume pch enabled gcc, because on older
# Once done this will define:
#
# Variable:
#   PCHSupport_FOUND
#
# Macro:
#   ADD_PRECOMPILED_HEADER

SET(PCHSupport_FOUND TRUE)

MACRO(ADD_PRECOMPILED_HEADER _targetName _name)

IF(NOT CMAKE_BUILD_TYPE)
  MESSAGE(FATAL_ERROR 
    "This is the ADD_PRECOMPILED_HEADER macro. " 
    "You must set CMAKE_BUILD_TYPE!"
  )
ENDIF(NOT CMAKE_BUILD_TYPE)

set(_path ${CMAKE_CURRENT_SOURCE_DIR})
set(_input ${_path}/${_name})

SET(_output "${CMAKE_CURRENT_BINARY_DIR}/${_name}.gch")
    
STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_var_name)
STRING(TOUPPER "COMPILE_DEFINITIONS_${CMAKE_BUILD_TYPE}" _def_var_name)

GET_DIRECTORY_PROPERTY(DIRDEF COMPILE_DEFINITIONS)
FOREACH(item ${DIRDEF})
	LIST(APPEND _compile_FLAGS -D${item})
ENDFOREACH(item)

GET_DIRECTORY_PROPERTY(DIRDEF ${_def_var_name})
FOREACH(item ${DIRDEF})
	LIST(APPEND _compile_FLAGS -D${item})
ENDFOREACH(item)

LIST(APPEND _compile_FLAGS ${CMAKE_CXX_FLAGS} )

LIST(APPEND _compile_FLAGS ${${_flags_var_name}})
#MESSAGE(${_compile_FLAGS})

GET_DIRECTORY_PROPERTY(DIRINC INCLUDE_DIRECTORIES)
#MESSAGE(${DIRINC})
FOREACH(item ${DIRINC})
	LIST(APPEND _compile_FLAGS -I${item})
#MESSAGE(${_compile_FLAGS})
ENDFOREACH(item)

LIST(APPEND _compile_FLAGS ${_directory_flags})

SEPARATE_ARGUMENTS(_compile_FLAGS)

ADD_CUSTOM_COMMAND(
  OUTPUT ${_output}       
  COMMAND ${CMAKE_CXX_COMPILER}
                                ${_compile_FLAGS}
                                -x c++-header
                                -o ${_output} 
                                ${_input}
  DEPENDS ${_input}
)
ADD_CUSTOM_TARGET(${_targetName}_gch 
  DEPENDS ${_output}      
)
ADD_DEPENDENCIES(${_targetName} ${_targetName}_gch )
SET_TARGET_PROPERTIES(${_targetName} 
  PROPERTIES COMPILE_FLAGS "-Winvalid-pch"
)
ENDMACRO(ADD_PRECOMPILED_HEADER)
