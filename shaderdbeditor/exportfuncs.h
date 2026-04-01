#pragma once
#include "structs.h"
#include "framework.h"

void export_db(HWND hwnd, database_export* new_db, char* output_name, int force_export);
void export_str(HWND hwnd, database_export* new_db, char* output_name);
void export_hlsl(database_export* new_db, char* dst_folder);
uint32_t seek_guid(uint32_t src_cont_size, string_ref* src_container, char* target);
uint32_t seek_guid_vx(uint32_t src_cont_size, vxbuffer* src_container, char* target);