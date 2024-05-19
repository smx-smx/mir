#include "metadata/metadata.h"
#include "defs.h"

BEGIN_META_STRUCT(sample_struct, 12)
	META_STRUCT_FIELD(0, int, foo)
	META_STRUCT_FIELD(11, unsigned char, bar)
END_META_STRUCT(sample_struct)
