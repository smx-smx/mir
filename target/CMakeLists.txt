add_executable(target
	src/target.c
)
target_compile_options(target PRIVATE
	-fno-builtin
	-include ${TOP}/common.h
)
# to include decl_generated.h
target_include_directories(target PRIVATE ${GENDIR})
add_dependencies(target metadata_kb)