#pragma once
#include "framework.h"
#include <stdint.h>
#include "DirectXMesh.h"

HMENU init_menu(HWND hwnd, HMENU& submenu, HMENU& meshdatamenu);
HMENU init_meshdata_menu(HWND hwnd);
HMENU init_meshset_menu(HWND hwnd);
OPENFILENAMEA open_dialog(HWND hwnd, const char* filters);
char* show_file_name_only(char* src);

extern HWND hwndExportTab;
extern HWND hwndImportTab;
extern HWND hwndMain;
extern int base_x, base_y;
extern HFONT hfont;
extern int cxChar, cyChar;

typedef struct
{
	char* pool;
	uint32_t current_pos;
}mem_pool;

extern mem_pool* ac_structs_pool;
extern mem_pool* ac_data_pool;
extern mem_pool* ac_import_pool;
extern mem_pool* ac_edit_pool;
extern WNDPROC ASSETC_WND_PROC;
char* assetc_guid_convert(wchar_t* src, mem_pool* pool);

typedef struct
{
	uint32_t hash;
	char* name;
	float bounding_box[6];
	float transforms[12]; // 9,10,11 are shape position in game 
} shape_struct;

typedef struct
{
	char* data;
} rigid_mesh_data;

typedef struct
{
	char* data;
	uint32_t n_shapes;
	shape_struct* shapes;
	char* unknown_data; // size = n_shaders_per_set * 24;
	char* billboard; // DOES NOT ALWAYS appears. check for null byte after unknown data ONLY ON TREE MESH SET
} comp_n_tree_data;

typedef struct
{
	char* data; // "data" here is 4 null bytes. next uint32 is n_bones.
	uint32_t n_bones;
	uint32_t* bone_ids;
	uint16_t* unknown_data;
} skinned_mesh_data;

typedef struct
{
	char* mesh_dbx_path;
	char* shaders[75];
}shader_set;


typedef struct
{
	bool has_billboard;
	uint8_t unknown; // always 0
	uint8_t n_shader_sets;
	uint8_t n_shaders_per_set;
	shader_set* shader_sets; 
	float bounding_box[6];
	uint32_t mesh_size;
	comp_n_tree_data* comp_data;         // obviously only one of them is used at once. comp and tree use the same 
	skinned_mesh_data* skinned_data;
	rigid_mesh_data* rigid_data;
}meshset_lod;

typedef struct
{
	char* magic;
	char filepath[400];
	char* lods_file;
	uint32_t n_lods;
	meshset_lod* lods;
}meshset;


typedef struct 
{
	char* name;
	uint32_t n_faces;
	uint32_t n_vertices;
	uint32_t indices_start;
	uint32_t vertices_start;
	uint8_t vertex_stride;
	uint8_t primitive_type;
	uint8_t bones_per_vx;
	uint8_t shapes_in_node; //bones or shapes in this node 
	uint16_t shapes_in_node2; //same as above
	uint16_t* shape_indices;
	char* vx_buffer_pos;
	char* idx_buffer_pos;
} meshdata_node;

typedef struct
{
	uint32_t ro[4]; // "render order" info or whatever it is, cant find another word to name it
	uint8_t* ro_vals[4];
	uint32_t vbuffer_size;
	uint32_t indices_size;
	uint32_t null;
} meshdata_plus_info;

typedef struct
{
	uint32_t type;
	uint16_t unknown;
	uint8_t n_nodes;
	meshdata_node* nodes;
	meshdata_plus_info* info;
	char* vx_buffer_start;
	char* idx_buffer_start;
}meshdata;

typedef struct // auxiliar struct for individual shape data
{
	int idx_array_size;
	uint16_t n_vx; // number of unique vertex
	uint16_t* n_vx_array; // array of unique vertices
	uint16_t* indices_in; // the index values as they came from meshdata
	uint16_t* indices_out; // the values that we will assign when creating an fbxmesh 
} shape_geo;

typedef struct // necessary because of all the different types and shit 
{
	uint32_t alloc_size;
	char* start_address;
	uint32_t n_uvs;
	bool export_normals;
	bool export_vx_colors;
	float vx_coords[196605];
	float normals[196605];
	float tangents[196605];
	float uv_channels[5][131070];
	uint8_t bone_id[65535][4];
	uint8_t bone_w[65535][4];
	uint16_t* indices;
}node_geometry;

typedef struct {                       // fbx sucks at computing tangents and normals. use this.
	DirectX::XMFLOAT3 normals[65535];
	DirectX::XMFLOAT3 verts[65535];
	DirectX::XMFLOAT4 tangents[65535];
	DirectX::XMFLOAT2 dxuvs[65535];
} dx_data;