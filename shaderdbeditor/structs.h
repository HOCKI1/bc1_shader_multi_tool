#define _CRT_SECURE_NO_WARNINGS
#pragma once
#include "framework.h"
#include <stdint.h>

extern const uint32_t txe_e_size;
extern const uint32_t txe_p_size;
extern const uint32_t txe_t_size;
extern const int shader_type;
extern char blank[90];
extern bool shader_view;
extern const int txe_type;
extern const int vs_type;
extern const int ps_type;
extern WNDPROC ORIGINAL_EDIT_PROC;
extern WNDPROC PARENT_WND_PROC;
extern int iItem, iSubItem;
extern HWND hwndEdit, hwndList,hwndTree, hwndAsset;
extern HANDLE hIcon;

typedef struct {
	bool edit_enable;
	int type;  // types include texture list, vertex shader, etc
	int subtype; // subtypes are all of the members of each struct, ie, txe->texture name, table_parent->vertex shader ref, etc
	void* struct_ptr; // necessary to call "show" functions to update the view, not used by all the types
	void* block_data; // block specific pointer, use to point to additional necessary data
} lv_item_data;

typedef struct {
	char* pool;
	int current_pos;
} edit_pool;

extern edit_pool file_edit_pool;

typedef struct {
	char* mem_blocks[16];
	int used_slots;
} allocated_dbs;

extern allocated_dbs allocated_db;

typedef struct {       // a texture name with index 
	uint32_t* index;
	char* name;
} txe_e;

typedef struct {   // game parameter that the shader will use 
	char* name;
} txe_p;

typedef struct {  // internal texture type 
	char* name;
} txe_t;

typedef struct {
	int32_t hti_type;
	int32_t hti_index;
	int32_t shader_index;
	bool selected;
} hti_lparam;

typedef struct {
	hti_lparam* lprms;
	uint32_t lprms_counter;
	uint32_t max_val;
} lprm_storage;

extern lprm_storage* lprm_childs;
extern lprm_storage* lprm_parents;

typedef struct {   // individual struct for each texture list of the database
	uint32_t* eptr;  // pointer to the entity, also the offset of the total size 
	uint32_t esize; //size of the entity
	uint32_t* hsize; // "header size" (also the offset where data starts)
	uint32_t* gameparamsoffset; // game parameter that the shader uses 
	uint32_t* filetypeoffset; // file types that the shader uses 
	uint32_t* unknowndata; // unknown data offset
	uint8_t* ntxt; // number of textures 
	uint8_t* nparams; // number of params 
	uint8_t* ntypes; // number of types
	uint8_t* ff7ffstring; // number of times that string appears
	txe_e* txtptrs;
	txe_p* paramptrs;
	txe_t* typeptrs; 
	HTREEITEM txeitem; 
	hti_lparam lparam_data; 
	LVITEM* lv_items; // dont initialize manually
	lv_item_data* lv_data; // dont initialize manually
	bool initialize_items;
} TxE;


typedef struct {   // vertex shader struct. has 16 byte id, main shader size, params index and "obfuscator" shader at the end
	char* vsid;  // 16 byte id
	uint32_t* vs_size;  // size of main shader 
	char* shader; 
	uint32_t* txeindex; // points to a txe member
	uint32_t* d3d1p;
	uint32_t* d3d2p;
	uint32_t* obf_size; // size of the cheater shader they append at the end, also used to get nnumber of TEXCOORD strings and traverse the file
	char* obf_shader;
	uint32_t* n_txcoord;
	uint32_t total_size;
	HTREEITEM vs_item;
	hti_lparam lparam_data;
	TxE* txeptr;
	LVITEM* lv_items; // dont initialize manually
	lv_item_data* lv_data; // dont initialize manually
	bool initialize_items;
} bc2_vs;

typedef struct {   // vertex shader struct. has 16 byte id, main shader size, params index and "obfuscator" shader at the end
	char* psid;  // 16 byte id
	uint32_t* ps_size;  // size of main shader 
	char* shader;
	uint32_t* txeindex; // points to a txe member
	uint32_t* d3d1p;
	uint32_t* d3d2p;
	uint32_t total_size;
	HTREEITEM ps_item; 
	hti_lparam lparam_data;
	TxE* txeptr;
	LVITEM* lv_items; // dont initialize manually
	lv_item_data* lv_data; // dont initialize manually
	bool initialize_items;
} bc2_ps;


typedef struct {
	char* shader_id;
	char* system_id;
	char* vbuffer_id;
	char* unknown_data;
	bool has_parent;
} table_child;

typedef struct {
	char* element_id;
	uint32_t* unknown1;
	uint32_t* unknown2;
	uint32_t* temp_hash;
	uint32_t* null_int;
	uint32_t* vs_ref;
	uint32_t* ps_ref;
	uint32_t* txe_ref1;
	uint32_t* txe_ref2;
	table_child* child;
	bc2_vs* vs_shader;
	bc2_ps* ps_shader;
	TxE* txeptr_null;
	TxE* txeptr_2;
	HTREEITEM table_item;
	HTREEITEM hti_table_childs[4]; // 1 = VERTEX SHADER, 2 = PIXEL SHADER, 3 = EMPTY TXE, 4 = TXE 
	hti_lparam lparam_data;
	LVITEM* lv_items; // dont initialize manually
	lv_item_data* lv_data; // dont initialize manually
	bool initialize_items;
} table_parent;

typedef struct {
	char* vxbuffer_id;
	uint16_t** types_array;
	uint32_t* type;
	uint8_t* n_data_types;
	uint8_t* vx_stride;
	char* unknown_data;
	HTREEITEM vx_item;
	int n_wnds;
	hti_lparam lparam_data;
	LVITEM* lv_items; // dont initialize manually
	lv_item_data* lv_data; // dont initialize manually
	bool initialize_items;
} vxbuffer;

typedef struct 
{
	char* name;
	char* guid;
	HTREEITEM str_item;
	vxbuffer* vx_buffer; // put this here cause I didnt anticipate; only used by mesh string references
	bool selected;
	LVITEM* lv_items; // dont initialize manually
	lv_item_data* lv_data; // dont initialize manually
	bool initialize_items;
} string_ref;

typedef struct { // the members of the last part of the database 
	char* id;
	uint32_t n_table_refs;
	uint16_t* table_refs;
} shader_ref_struct;

typedef struct { // this struct will probably go away in the future (or merge with database_export) for some stupid reason I made 2 different ones. dumbass
	TxE* txeptr; //pointer to the first txe
	uint32_t ntxe; //number of txe elements 
	uint32_t d3dparams_size;
	char* d3dparams_block;
	uint32_t n_d3d_p1;
	uint32_t d3d_p1_size;
	bc2_vs* vs_shader;
	uint32_t n_vs_shader;
	bc2_ps* ps_shader;
	uint32_t n_ps_shader;
	table_parent* table;
	uint32_t n_table_elements;
	vxbuffer* vx_buffers;
	uint32_t n_vx_buffers;
	string_ref** strings;         // the 3 sections. 0 = shader names, 1 = system names, 2 = mesh nodes
	uint32_t n_strings[3]; // number of strings in each section. 0 = shader names, 1 = system names, 2 = mesh nodes
	HTREEITEM str_parent[3];
	shader_ref_struct* shader_refs;
} database;

typedef struct {
	HWND* hwndtxts;
	HWND* hwndprms;
	HWND* hwndtypes;
	int8_t prevhwndst, prevhwndsp, prevhwndsty;
} txe_hwnds;

typedef struct {
	HWND* w_hwnds;
	uint32_t w_hwnds_size;
	bool first_draw;
	bool destroy;
} generic_hwnds;

typedef struct {
	bool is_child;
	uint32_t* root_node_idx; // if this shader is under Systems we use this 
	string_ref* root_nodes;  // if this shader is under Systems we use this 
	string_ref* shader_ref;
	shader_ref_struct* tables_ref;
	table_parent* table_members;
	HTREEITEM hti_shader;
	database* associated_db;
	hti_lparam lparam_data;
	bool selected; // unused
	LVITEM* lv_items; // dont initialize manually
	lv_item_data* lv_data; // dont initialize manually
	bool initialize_items;
} shader_struct;

typedef struct {
	shader_struct* shaders;
	string_ref* mesh_refs;
	string_ref* systems_ref;
	char* d3dparams_block;
	vxbuffer* vx_buffers;
	uint32_t n_shaders;
	uint32_t n_systems;
	uint32_t n_mesh_refs;
	uint32_t n_vx_buffers;
	uint32_t n_txe;
	uint32_t n_vs;
	uint32_t n_ps;
	uint32_t n_d3d_p1;
	uint32_t n_table_elements;
	uint32_t d3d_p1_size;
	uint32_t d3dparams_size;
	HTREEITEM shader_p_node;
	HTREEITEM mesh_p_node;
} database_export;

