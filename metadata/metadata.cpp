/**
 * @copyright Copyright (c) 2024 Stefano Moioli <smxdev4@gmail.com>
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define MSVC_SUPPORT

#ifdef __cplusplus
extern "C" {
#endif

#include "metadata.h"

extern uint8_t *metadata_begin();
extern uint8_t *metadata_end();

#ifdef __cplusplus
}
#endif

#include <string>
#include <unordered_map>
static std::unordered_map<const char *, struct __meta_struct *> struct_pointers;

static bool gen_data = 0;
static bool gen_code = 0;
static bool gen_types = 0;

static FILE *fh_json = NULL;
static FILE *fh_hdr = NULL;
static FILE *fh_debug = NULL;

#define OUT_JSON(fmt, ...) fprintf(fh_json, fmt, ##__VA_ARGS__)
#define OUT_HDR(fmt, ...) fprintf(fh_hdr, fmt, ##__VA_ARGS__)
#define DEBUG(fmt, ...) fprintf(fh_debug, fmt, ##__VA_ARGS__)

const char *meta_type_name(enum __meta_item_type type)
{
  switch (type) {
  case META_FREF:
    return "function";
  case META_DREF:
    return "data";
  case META_STRUCT:
    return "struct";
  default:
    return "unknown";
  }
}

ssize_t handle_func_ref(struct __meta_function_item *ref){
  if (!ref->name) return -1;
  ssize_t size = sizeof(*ref);
  if(!gen_code) return size;

  //DEBUG("ref %d; %p; %s\n", ref->item_type, ref->orig_addr, ref->name);

  OUT_JSON("{\n"
           "  \"item_type\": \"%s\",\n"
           "  \"addr\": \"0x%x\"",
           meta_type_name(ref->item_type), ref->orig_addr);

  if (ref->ret_type && ref->name && ref->arg_types) {
    OUT_JSON(",\n"
             "  \"ret\": \"%s\",\n"
             "  \"args\": \"%s\",\n"
             "  \"name\": \"%s\"",
             ref->ret_type,
             ref->arg_types,
             ref->name);
  }
  if (ref->regs) {
    OUT_JSON(",\n"
             "  \"regs\": \"%s\"",
             ref->regs);
  }
  if (ref->stack_bytes >= 0){
    OUT_JSON(",\n"
             "  \"stack_bytes\": %d", ref->stack_bytes);
  }
  OUT_JSON("\n}");

#ifdef MSVC_SUPPORT
  OUT_HDR(
    "#ifdef _MSC_VER\n"
    "TFUNC %s %s(%s);\n"
    "#endif\n",
    ref->ret_type, ref->name, ref->arg_types
  );
#endif
  return size;
}

ssize_t handle_data_ref(struct __meta_data_item *ref){
  if (!ref->name) return -1;
  ssize_t size = sizeof(*ref);
  if(!gen_code) return size;

  //DEBUG("ref %d; %p; %s\n", ref->item_type, ref->orig_addr, ref->name);

  OUT_JSON("{\n"
           "  \"item_type\": \"%s\",\n"
           "  \"addr\": \"0x%x\"",
           meta_type_name(ref->item_type), ref->orig_addr);
  if (ref->type && ref->name) {
    OUT_JSON(",\n"
             "  \"type\": \"%s\",\n"
             "  \"name\": \"%s\"",
             ref->type, ref->name);
  }
  OUT_JSON("\n}");

#ifdef MSVC_SUPPORT
  OUT_HDR("#ifdef _MSC_VER\n");
  if(strchr(ref->type, '[')){
    OUT_HDR("TDATA %s;\n", ref->type);
  } else {
    OUT_HDR("TDATA %s %s;\n", ref->type, ref->name);
  }
  OUT_HDR("#endif\n");
#endif

  return size;
}

#define MAX_STRUCT_FIELDS 1024

int struct_field_compare(const void *a, const void *b)
{
	const struct __meta_struct_field *f1 = (struct __meta_struct_field *)a;
	const struct __meta_struct_field *f2 = (struct __meta_struct_field *)b;
	if (f1->offset <= f2->offset) {
		if (f1->offset == f2->offset) return 0;
		return -1;
	}
	return 1;
}

//$FIXME: add array property to construct on the fly
static char initial_pad_namebuf[1024];

#define IS_END_FIELD(f) ((f)->offset == -1 && (f)->size == -1)

static void dispose_fh(FILE *fh){
	if(fh == NULL) return;
	fflush(fh);
	if(fh == stdout || fh == stderr) return;
	fclose(fh);
}

ssize_t update_struct_size(struct __meta_struct *st){
	//DEBUG("update_struct_size for %s\n", st->name);
	struct __meta_struct_field *f;

	struct __meta_struct_field last;
	last.offset = 0;


	for(f = &st->fields[0]; !IS_END_FIELD(f); f++){
		if((f->name || f->decl) && (f->offset >= last.offset)){
			last = *f;
		}
	}
	f = &last;


	ssize_t size = f->offset;
	if(f->size > 0){
		// primitive or known
		size += f->size;
	} else if(f->size == 0) {
		// unknown
		if(struct_pointers.find(f->type) == struct_pointers.end()){
			DEBUG("ERROR: couldn't find type %s\n", f->type);
			return -1;
		} else {
			struct __meta_struct *sub_st = struct_pointers.at(f->type);
			if(sub_st->size == 0 && update_struct_size(sub_st) < 0){
				return -1;
			}
			size += sub_st->size;
		}
	}

	st->size = size;
	return 0;
}

void update_field_size(struct __meta_struct_field *f){
	if(struct_pointers.find(f->type) == struct_pointers.end()) {
		DEBUG("ERROR: couldn't find type %s\n", f->name);
		return;
	}
	struct __meta_struct *st = struct_pointers.at(f->type);
	update_struct_size(st);

	// had `f` been a pointer, its size would be known.
	// if we get this far, it means `f` was a child struct itself, so we can safely do this
	f->size = st->size;
}



ssize_t handle_struct(struct __meta_struct *st)
{
	struct_pointers[st->name] = st;

  unsigned char *begin = (unsigned char *)st;


  if (gen_types) {
    OUT_JSON(
      "{\n"
      "  \"item_type\": \"struct\"\n"
      "}"
    );
    OUT_HDR(
      "#ifdef _MSC_VER\n"
      "#pragma pack(push, 1)\n"
      "#endif\n"
    );
	  OUT_HDR("typedef struct PACK %s {\n", st->name);
    //OUT_HDR("\nPACK(typedef struct %s {\n", st->name);
    DEBUG("st %s\n", st->name);
  }

  int num_fields = 0;
  struct __meta_struct_field *f = st->fields;
  while(!IS_END_FIELD(f)){
	  f++;
	  num_fields++;
  }

  //`f` points to the last field, skip it to know the end
  ++f;

  // always insert 0-sized field at the start, for initial padding
  ++num_fields;

  unsigned char *end = (unsigned char *)f;

  struct __meta_struct_field *fields = (struct __meta_struct_field *)
    calloc(num_fields, sizeof(struct __meta_struct_field));
  memcpy(&fields[1], st->fields, sizeof(struct __meta_struct_field) * (num_fields-1));
  qsort(&fields[1], (num_fields-1), sizeof(struct __meta_struct_field),
        struct_field_compare);

  int pad_field_num = 0;

  // set the initial padding size
  int initial_pad_size = 0;
  if(num_fields == 1){
    // insert dummy byte to make MSVC 6.x happy
    initial_pad_size = 1;
  } else if(num_fields > 1 && fields[1].offset > 0){
    initial_pad_size = fields[1].offset;
  }
  if(initial_pad_size > 0){
    snprintf(initial_pad_namebuf, sizeof(initial_pad_namebuf),
      "uint8_t __padding0[%d]", initial_pad_size);
	struct __meta_struct_field initial_padding = { 0 };
	  {
		  struct __meta_struct_field *f = &initial_padding;
		  f->name = "__padding0";
		  f->decl = initial_pad_namebuf;
		  f->type = "uint8_t";
		  f->size = initial_pad_size;
	  }

    fields[0] = initial_padding;
    ++pad_field_num;
  }

  int cumulative_size = 0;
  for (int i = 0; i < num_fields; i++) {
	f = &fields[i];
    if(f->size < 1){
     /**
 	  * allow unknown-sized field (structure in structure)
 	  * ONLY if it's the only member
 	  * (NOTE: first field is initial padding)
 	  */
      if(num_fields == 2 && i == 1){
	  }
	  /**
	   * allow undefined or 0-length struct member if it's the last item before the end
	   */
	  else if((f->name || f->decl) && (i + 1) >= num_fields){
		  if(f->name && f->decl){} // flexible array member
	  }
	  else {
		  bool skip = false;
		  if(f->name || f->decl) {
			  update_field_size(f);
			  if(f->size == 0) {
				  skip = true;
				  DEBUG("skipping 0-sized field %s (%s)\n",
				        ((f->name) ? f->name : "(null)"),
				        ((f->decl) ? f->decl : "(null)"));
			  }
		  } else {
			  skip = true;
		  }
		  if(skip) {
			  continue;
		  }
	  }
    }

    int padding = 0;
    // distance to the next struct field
    int distance = 0;
    int next = i + 1;
    
    // if there is a field after us
    if (next < num_fields) {
      distance = fields[next].offset - f->offset;
    }

    if (distance > 0) {
      padding = distance - f->size;
    }

    #define FIELD_DECL(f) (f->decl) ? f->decl : f->type

    if (gen_types) {
      if(cumulative_size != f->offset){
        fprintf(stderr, "OFFSET MISMATCH for %s. Expected %d, actual %d\n",
          f->name, f->offset, cumulative_size);
        return -1;
      }

      OUT_HDR("  %s%s%s; ///< offset=0x%x\n",
        FIELD_DECL(f),
        (f->decl) ? "" : " ",
        (f->decl) ? "" : f->name,
        f->offset
      );
      
      if (padding > 0) {
        OUT_HDR("  uint8_t __padding%d[%d]; ///< offset=0x%x\n", pad_field_num++,
                padding, f->offset + f->size);
        cumulative_size += padding;
      }

      DEBUG(" f [%d-%d] %s %s (%d)\n", f->offset, f->offset + f->size, f->type,
            f->name, f->size);
      cumulative_size += f->size;
    }
  }

  // trailing padding
  if(st->size > 0 && cumulative_size < st->size){
    int padding = st->size - cumulative_size;
    OUT_HDR("  uint8_t __padding%d[%d]; ///< offset=0x%x\n", pad_field_num++,
                padding, f->offset + f->size);
    cumulative_size += padding;
  }

  if(gen_types) {
    OUT_HDR("} %s;\n", st->name);
    //OUT_HDR("}) %s;\n", st->name);
    OUT_HDR(
      "#ifdef _MSC_VER\n"
      "#pragma pack(pop)\n"
      "#endif\n"
    );

    if(st->size > 0){
      OUT_HDR("\nstatic_assert(sizeof(%s) == %d);\n", st->name, st->size);
    }
  }

  for(int i=0; i<num_fields; i++){
    f = &fields[i];
    if(f->size < 1) continue;
    OUT_HDR("static_assert(offsetof(%s, %s) == %d);\n", st->name, f->name, f->offset);
  }

  free(fields);

  ssize_t size = end - begin;
  return size;
}

int main(int argc, const char **argv, const char **envp)
{
  fh_json = stdout;
  fh_hdr = stdout;
  fh_debug = stderr;

  setvbuf(fh_debug, NULL, _IONBF, 0);


	uint8_t *start = metadata_begin();
	uint8_t *end = metadata_end();
	size_t metadata_length = end - start;

	uint8_t *metadata_buffer = (uint8_t *)calloc(metadata_length, 1);
	if(!metadata_buffer){
		fprintf(stderr, "calloc(%zu) failed\n", metadata_length);
		return EXIT_FAILURE;
	}
	memcpy(metadata_buffer, start, metadata_length);

	start = metadata_buffer;
	end = start + metadata_length;

  const char *filename = NULL;
  for (int i = 1; i < argc;) {
    const char *arg = argv[i++];
    if (!strcmp(arg, "-data")) {
      gen_data = true;
      continue;
    }
    if (!strcmp(arg, "-code")) {
      gen_code = true;
      continue;
    }
    if (!strcmp(arg, "-types")) {
      gen_types = true;
      continue;
    }
    if (!strcmp(arg, "-out-hdr")) {
      filename = argv[i++];
      FILE *out = fopen(filename, "w");
      if (!out) {
        fprintf(stderr, "Failed to open file '%s' for writing\n", filename);
      } else {
        fh_hdr = out;
      }
    }
    if (!strcmp(arg, "-out-json")) {
      filename = argv[i++];
      FILE *out = fopen(filename, "w");
      if (!out) {
        fprintf(stderr, "Failed to open file '%s' for writing\n", filename);
      } else {
        fh_json = out;
      }
    }
  }

  if(gen_types){
    OUT_HDR("#include <stddef.h>\n"
      "#include <stdint.h>\n"
      "#ifdef __GNUC__\n"
      "#define PACK __attribute__((__packed__))\n"
      "#define TFUNC extern\n"
      "#define TDATA extern\n"
      "#endif\n"
      "#ifdef _MSC_VER\n"
      "#define TFUNC __declspec(dllexport)\n"
      "#define TDATA __declspec(dllimport)\n"
      "#define PACK\n"
      "#endif\n"
      "#define FLEXIBLE_ARRAY 1\n"
    );
  }

  OUT_JSON("[");
  bool json_first = true;

  bool cont = true;
  bool emitted = false;
  int exitCode = 0;

  for (; start < end && cont;) {
    if (json_first)
      json_first = false;
    else if (emitted) {
      OUT_JSON(",");
      emitted = false;
    }
    enum __meta_item_type type = *(enum __meta_item_type *)start;

    ssize_t size = 0;
    switch (type) {
    case META_FREF:
      size = handle_func_ref((struct __meta_function_item *)start);
      emitted = size > 0 && gen_code;
      break;
    case META_DREF:
      size = handle_data_ref((struct __meta_data_item *)start);
      emitted = size > 0 && gen_data;
      break;
    case META_STRUCT:
      size = handle_struct((struct __meta_struct *)start);
      emitted = size > 0 && gen_types;
      break;
    default:
      cont = false;
      break;
    }
    if (cont && size <= 0) {
      DEBUG("Error while handling meta item %d\n", type);
      exitCode = 1;
      goto end;
    }
    start += size;
  }
  OUT_JSON("]\n");

end:
  dispose_fh(fh_json);
  dispose_fh(fh_hdr);
  dispose_fh(fh_debug);
  free(metadata_buffer);
  return exitCode;
}
