/**
 * @copyright Copyright (c) 2024 Stefano Moioli <smxdev4@gmail.com>
 */

#pragma once

#ifndef METADATA_BUILD
/**
 * @brief
 * we're not building metadata so we want to generate actual code
 * don't emit weak stubs
 */
#define META_WEAK
#define META_IMPL
#endif


#ifdef METADATA_BUILD
#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

/**
 * @brief 
 * define data structures used by metadata
 */

enum __meta_item_type {
    META_FREF = 1,
    META_DREF,
    META_STRUCT
};

PACK(struct __meta_function_item {
    enum __meta_item_type item_type;
    unsigned long orig_addr;
    const char *name;
    const char *ret_type;
    const char *arg_types;
    const char *regs;
    int stack_bytes; // for stdcall
});

PACK(struct __meta_data_item {
    enum __meta_item_type item_type;
    unsigned long orig_addr;
    const char *name;
    const char *type;
});

PACK(struct __meta_struct_field {
    const char *name;
    const char *type;
    const char *decl;
    int offset;
    int size;
});

PACK(struct __meta_struct {
    enum __meta_item_type item_type;
    const char *name;
    int size;
    struct __meta_struct_field fields[];
});

#ifdef _METADATA_INC
/**
 * @brief 
 * we're building the metadata stubs, so we need to make data concrete
 * and provide default implementation for functions
 */
#undef TDATA
#define TDATA 
// define weak alias for all code functions
#define META_WEAK __attribute__((weak)) 
#define META_IMPL {}
#else
#define META_IMPL
#define META_WEAK
#endif

#endif

#define _PRN_ARRAY_DIMENSION(field) "[" #field "]"
#define _ARRAY_DIMENSION(field) [ field ]

#if defined(_METADATA_INC)
/**
 * @brief
 * store all metadata information as a contiguous array
 * in .metadata section
 */
#pragma data_seg("metadata")

#ifdef _MSC_VER
#define META_DECL \
    __declspec(dllexport) \
    __declspec(allocate("metadata")) \
    __declspec(align(1))
#else /** _MSC_VER */
#define META_DECL \
    __declspec(dllexport) \
    __attribute__ ((section ("metadata"))) \
    __attribute__ ((aligned (1)))
#endif /** _MSC_VER */

#define DECLARE_META_FUNC(addr, name, ret_type, arg_types, regs, stack_bytes) \
    META_DECL struct __meta_function_item __meta_ ## name = { META_FREF, addr, #name, ret_type, arg_types, regs, stack_bytes }
#define DECLARE_META_DATA(addr, name, type) \
    META_DECL struct __meta_data_item __meta_ ## name = { META_DREF, addr, #name, type }

#define META_STRUCT_FIELD(offset, type, name) { #name, #type, 0, offset, sizeof(type) },
#define META_STRUCT_FIELD_ARRAY(offset, type, name, ...) { \
    #name, \
    #type MAP(_PRN_ARRAY_DIMENSION, __VA_ARGS__), \
    #type " " #name MAP(_PRN_ARRAY_DIMENSION, __VA_ARGS__), \
    offset, sizeof( type MAP(_ARRAY_DIMENSION, __VA_ARGS__) ) },


#define META_STRUCT_FIELD_DECL(offset, decl, type, name) { #name, #type, #decl, offset, sizeof(type) },

#define BEGIN_META_STRUCT(name, size) \
    PACK(typedef struct { unsigned char __buffer[size]; }) name; \
    META_DECL struct __meta_struct __meta_struct_ ## name = { META_STRUCT, #name, size, {

#define END_META_STRUCT(name) \
    { 0, 0, 0, -1, -1 } \
}};

#else /** _METADATA_INC */

#define DECLARE_META_FUNC(addr, name, ret_type, arg_types, regs, stack_bytes)
#define DECLARE_META_DATA(addr, name, type)

/** (speculatively) make the IDE see the type early before metadata-gen **/
#ifndef _MSC_VER
#define BEGIN_META_STRUCT(name, size) typedef struct {
#define META_STRUCT_FIELD_DECL(offset, decl, type, name) decl;
#define META_STRUCT_FIELD(offset, type, name) type name;
#define META_STRUCT_FIELD_ARRAY(offset, type, name, ...) type name MAP(_ARRAY_DIMENSION, __VA_ARGS__);
#define END_META_STRUCT(name) } name ;
#endif

#endif /** _METADATA_INC */

#if 0 && (defined(_MSC_VER) && !defined(__INTELLISENSE__))
#pragma warning(disable:4003)
#define DECLARE_TARGET_FUNCTION(addr, ret, name, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
#define DECLARE_FASTCALL_FUNCTION(addr, ret, name, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
#define DECLARE_STDCALL_FUNCTION(addr, ret, name, stack_bytes, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
#define DECLARE_TARGET_FUNCTION_THUNK(addr, ret, name, regs, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
#define DECLARE_TARGET_DATA(addr, decl, name)
#define DECLARE_TARGET_DATA_DECL(addr, decl, name)
#define DECLARE_TARGET_DATA_ARRAY(addr, type, name, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)

#else
#include "map_macro.h"

#define DECLARE_TARGET_FUNCTION(addr, ret, name, ...) \
    TFUNC META_WEAK ret name (__VA_ARGS__); \
    DECLARE_META_FUNC(addr, name, #ret, #__VA_ARGS__, "", -1); \
    ret META_WEAK name (__VA_ARGS__) META_IMPL

#define DECLARE_FASTCALL_FUNCTION(addr, ret, name, ...) \
    TFUNC META_WEAK ret __fastcall name (__VA_ARGS__); \
    DECLARE_META_FUNC(addr, name, #ret, #__VA_ARGS__, "", -1); \
    ret META_WEAK __fastcall name (__VA_ARGS__) META_IMPL

#define DECLARE_STDCALL_FUNCTION(addr, ret, name, stack_bytes, ...) \
    TFUNC META_WEAK ret __stdcall name (__VA_ARGS__); \
    DECLARE_META_FUNC(addr, name, #ret, #__VA_ARGS__, "", stack_bytes); \
    ret META_WEAK __stdcall name (__VA_ARGS__) META_IMPL


#define DECLARE_TARGET_FUNCTION_THUNK(addr, ret, name, regs, ...) \
	TFUNC META_WEAK ret name (__VA_ARGS__);                     \
    DECLARE_META_FUNC(addr, name, #ret, #__VA_ARGS__, regs, -1); \
	ret META_WEAK name (__VA_ARGS__) META_IMPL

#define DECLARE_TARGET_DATA(addr, decl, name) \
    TDATA META_WEAK decl name; \
    DECLARE_META_DATA(addr, name, #decl)

#define DECLARE_TARGET_DATA_DECL(addr, decl, name) \
    TDATA META_WEAK decl; \
    DECLARE_META_DATA(addr, name, #decl)

#define DECLARE_TARGET_DATA_ARRAY(addr, type, name, ...) \
    TDATA META_WEAK type name MAP(_ARRAY_DIMENSION, __VA_ARGS__); \
    DECLARE_META_DATA(addr, name, #type " " #name MAP(_PRN_ARRAY_DIMENSION, __VA_ARGS__))

#endif /** _MSC_VER */
