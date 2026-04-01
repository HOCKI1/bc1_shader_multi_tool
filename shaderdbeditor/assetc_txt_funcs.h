#pragma once
#include "assetc.h"
#include "assetc_structs.h"
#include "assetc_export_funcs.h"
#include "assetc_meshset_funcs.h"

typedef uint32_t u32;
typedef uint8_t u8;


FILE* create_txt_file(char* filename);
void write_config(FILE* file);
void write_meshset_info(FILE* file, uint32_t mesh_type, uint32_t n_shapes, meshset_lod* lod, char* lod_file);
void write_shapes_composite(FILE* file, uint8_t* shapes_subdivision, uint32_t n_shapes, shape_struct* shapes);
void write_shapes_rigid(FILE* file, float* bbox, char* txt_file_name);
void write_meshdata_info(FILE* file, meshdata_node* nodes, meshdata_plus_info* meshdata_info, int n_nodes);
void write_type_only(FILE* file, int mesh_type);
void write_ro_only(FILE* file, meshdata_plus_info* meshdata_info);
char* get_line_end(char* src);
char* skip_space(char* src);
char* skip_text(char* src);
char** parse_txt_generic(char* txt_file, char* buffer, char* line_ptrs, uint32_t* n_lines);
shape_struct* get_shapes(char** strings_array, u32 line_count, u32* n_shapes);
