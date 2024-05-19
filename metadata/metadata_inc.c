/**
 * @copyright Copyright (c) 2024 Stefano Moioli <smxdev4@gmail.com>
 */
 
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

void catchall(){}

// generate all metadata
#include "target/target.h"

extern unsigned char __start_metadata;
extern unsigned char __stop_metadata;

unsigned char *metadata_begin(){
	return &__start_metadata;
}
unsigned char *metadata_end(){
	return &__stop_metadata;
}

#ifdef __cplusplus
}
#endif
