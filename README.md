# mir

mir (Metadata Iterative Reversing tool) is a set of macros and tools which can be used to reverse engineer complex programs and data structures.

it's particularly useful to reconstruct the code with the bottom-up method, also known as [ship of theseus](https://en.wikipedia.org/wiki/Ship_of_Theseus)

the process works in the following sequence:

## 1. instrumented (metadata) build
We first build an instrumented version of the code base, called `metadata`.
In this instrumented version, all functions are stubs and certain macros (structs, function and data declarations) are converted into binary form. (more details in [metadata](#metadata))

This compiled data is inserted into the generated program in the `.metadata` section.
Some code is added alongside this metadata (see `metadata.cpp`) in order to print the compiled data in json and header forms.

## 2. print the generated metadata (`metadata_kb`)
We can now just invoke this generated `metadata` tool to print the data out.
The output is called the "knowledge base", or "kb" for short

The json file (`gen/kb.json`) can be used by external programs, for example to:
- patch the target program (e.g. to replace the original functions with absolute jumps to the reimplemented version)
- generate statistics about reimplemented functions
- etc..

The header file (`gen/decl_generated.h`) is instead used by the target/reimplementation code to call the original code or use the original data structures.

## 3. use the produced metadata
The code in `target` can now use the generated structures/functions/data

# metadata
metadata is divided into 3 types:

- functions 
- data
- structures


## functions/data
functions and data are declared with the following macros:

```
DECLARE_TARGET_FUNCTION(orig_addr, return_type, name, ...args)
DECLARE_TARGET_DATA(orig_addr, type, name);
```

JSON output:

```json
{
  "item_type": "function",
  "addr": "0xdeadbeef",
  "ret": "int",
  "args": "int arg1",
  "name": "target_sample_func",
  "regs": ""
},{
  "item_type": "data",
  "addr": "0xbeefdead",
  "type": "int",
  "name": "target_sample_data"
}
```

Header output: none (the macro is expanded directly to the function prototype)

## structures

structures are declared with multiple macros in order to encode each member offset and size.
this makes it possible to auto-generate padding for unknown members

the following code in `meta_types.h`

```c
BEGIN_META_STRUCT(sample_struct, 12)
	META_STRUCT_FIELD(0, int, foo)
	META_STRUCT_FIELD(11, unsigned char, bar)
END_META_STRUCT(sample_struct)
```

will generate the following header code:

```c
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
typedef struct PACK sample_struct {
  int foo; ///< offset=0x0
  uint8_t __padding0[7]; ///< offset=0x4
  unsigned char bar; ///< offset=0xb
} sample_struct;
#ifdef _MSC_VER
#pragma pack(pop)
#endif

static_assert(sizeof(sample_struct) == 12);
static_assert(offsetof(sample_struct, foo) == 0);
static_assert(offsetof(sample_struct, bar) == 11);

```

# code structure
The following is a description of the structure of the code.

The scope indicates if some file is valid within `metadata`, `target`, or both

| file | description | scope |
| -- | -- | -- |
| target/macros.h | common macros | metadata+target |
| target/types_common.h | common/shared types | metadata+target |
| target/types_pre.h | manually defined types to be included before metadata (i.e. well known target types that other types depend upon) | target |
| target/meta_types.h | metadata-defined structures, to be converted to headers. the target code will instead use the generated `decl_generated.h` | metadata |
| target/defs.h | common defines use by the target | metadata+target |
| decl_generated.h | the converted/generated version of `target/meta_types.h` | metadata+target |
| metadata/metadata.h | This file serves a dual purpose. When included in `metadata` scope, it converts all function declarations to stubs and expands the metadata macros to binary structures that will be used by the metadata generation tool. When included in `target` scope, it instead generates `extern` entries for `functions` and `data` | metadata+target |
| target/types_post.h | types that depend on `meta_types.h` (and so can be considered only after generation) | target |
| target/inlines.h | collection of inline functions/macros in the target | target |
| target/target.h | This file must include all target header files, containing `DECLARE_TARGET_FUNC` and `DECLARE_TARGET_DATA` calls. In `metadata` scope, it will emit the respective function and data metadata entries. in `target` scope, it will emit the respective `extern` declarations | metadata+target |
