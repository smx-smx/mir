# output JSON file
set(OUT_KB_JSON ${GENDIR}/kb.json)
# output header file
set(OUT_DECL_H ${GENDIR}/decl_generated.h)

add_custom_command(
	OUTPUT ${OUT_KB_JSON} ${OUT_DECL_H}
	DEPENDS metadata
	COMMAND $<TARGET_FILE:metadata> -code -data -types
			-out-json ${OUT_KB_JSON}
			-out-hdr ${OUT_DECL_H}
)
add_custom_target(metadata_kb ALL
	DEPENDS ${OUT_KB_JSON} ${OUT_DECL_H})

