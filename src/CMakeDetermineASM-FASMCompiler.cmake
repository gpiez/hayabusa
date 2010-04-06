# determine the compiler to use for ASM using fasm

SET(ASM_DIALECT "-FASM")
SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT ${_CMAKE_TOOLCHAIN_PREFIX}fasm)
INCLUDE(CMakeDetermineASMCompiler)
SET(ASM_DIALECT)
