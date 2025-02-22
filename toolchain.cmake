set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR STM8L)

set(CMAKE_C_FLAGS "-lstm8 -mstm8 --opt-code-size --std-sdcc99 --nogcse --all-callee-saves --debug --verbose --stack-auto --fverbose-asm --float-reent --no-peep")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
# which compilers to use for C and C++
set(CMAKE_C_COMPILER sdcc)

# here is the target environment is located
SET(CMAKE_FIND_ROOT_PATH  /opt/sdcc)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)