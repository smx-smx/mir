## stage 1: generate a special "instrumented" build of the code
## the resulting binary, "metadata", will generate json/header files

set(SRCDIR ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(metadata 
	${SRCDIR}/metadata_inc.c
	${SRCDIR}/metadata.cpp
)
target_include_directories(metadata PRIVATE ${TOP})


# required to respect the ordering in "meta_types.h"
# (unless someone wants to add an explicit depedency field between structs?)
if(IS_GCC)
	target_compile_options(metadata PRIVATE
		-fno-toplevel-reorder
	)
endif()
target_compile_options(metadata PRIVATE
	# don't warn about stubbed functions
	-Wno-return-type
)
# flags to enable stubs generation
set_source_files_properties(${SRCDIR}/metadata.cpp PROPERTIES COMPILE_DEFINITIONS "_METADATA_MAIN")
set_source_files_properties(${SRCDIR}/metadata_inc.c PROPERTIES COMPILE_DEFINITIONS "_METADATA_INC")

target_compile_definitions(metadata PUBLIC PC_TARGET METADATA_BUILD)
target_compile_options(metadata PRIVATE
	-fno-builtin
	-include ${TOP}/common.h
)

