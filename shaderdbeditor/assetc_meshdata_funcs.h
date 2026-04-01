#pragma once
#include "structs.h"
#include "assetc.h"
#include "assetc_structs.h"
#include "assetc_export_funcs.h"
#include "assetc_fbx_funcs.h"
#include "assetc_import_funcs.h"
#include "assetc_txt_funcs.h"

void mesh_extractor_main(FbxManager* pSdkManager, FbxScene* pScene, meshdata* mesh_data, meshset* mesh_set, vxbuffer* vbuffer, int mesh_type, int current_lod, char* file_name, uint8_t* shapes_subdivision);
void meshdata_to_txt(meshdata* meshdata, char* output_filename);

meshset_lod* mesh_generator_main(FbxNode* root, import_settings* import_data, shape_struct* shapes, char* buffer, int buffer_pos, int n_shapes, int c_lod, char* base_name, uint8_t* shape_div);
meshdata* meshdata_reader(char* src);

extern FILE* import_asset_txt;