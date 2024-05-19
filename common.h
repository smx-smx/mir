/**
 * @copyright Copyright (c) 2024 Stefano Moioli <smxdev4@gmail.com>
 */
#pragma once

#ifdef _MSC_VER
#pragma runtime_checks("scu", off)
#endif

#include "target/macros.h"

/**
 * @brief 
 * define bare metal types, unless we're building the metadata generator
 * (which uses windows types)
 */
#include "target/types_common.h"

#ifndef _METADATA_MAIN
#include "target/types_pre.h"
#endif

#if defined(_METADATA_INC) || defined(__INTELLISENSE__)
/**
 * if this is set, we're being included by the metadata generator tool
 * define all concrete metadata structures that will be included in the resulting .exe
 **/
#include "target/meta_types.h"
#endif

#include "target/defs.h"

#if !defined(METADATA_BUILD) && !defined(__INTELLISENSE__)
/**
 * we're building the actual game target
 * include all generated types (generated by the metadata tool)
 **/
#include "decl_generated.h"
#endif

// must come AFTER types.h
#include "metadata/metadata.h"

#ifndef _METADATA_MAIN
#include "target/types_post.h"
#include "target/inlines.h"
#endif

/**
 * @brief 
 * common.h is included in all .c files
 * when building metadata, all function/data macros will define metadata structs
 * since we don't want to define N copies of metadata structures (to avoid a linker error)
 * we must guard against this
 */
#if !defined(_METADATA_MAIN)// && !defined(_METADATA_INC)
#include "target/target.h"
#endif

