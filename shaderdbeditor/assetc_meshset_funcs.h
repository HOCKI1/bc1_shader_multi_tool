#pragma once
#include "assetc.h"
#include "assetc_structs.h"
#include "assetc_txt_funcs.h"
#include <stdio.h>
#include <string>

meshset* meshset_handler(const char* src_file, uint32_t* type);
uint32_t shapenamehasher(std::string strhashable);
int write_shapes(shape_struct* shapes, int n_nodes, int n_shapes, int buffer_pos, char* buffer);
void meshset_writer(meshset* meshset_file, char* output_filename, int mesh_type);
void meshset_to_txt(meshset* meshset_file, char* output_filename, int mesh_type);
void meshset_txt_to_binary(char** lines, char* buffer, char* filename, uint32_t n_lines);