#pragma once
#include "assetc.h"
#include "assetc_structs.h"
#include "Resource.h"
#include "show_lv_funcs.h"
#include "exportfuncs.h"
#include "assetc_fbx_funcs.h"
#include "assetc_meshset_funcs.h"
#include "assetc_txt_funcs.h"

#define RIGID_MESH 0
#define SKINNED_MESH 1 
#define COMPOSITE_MESH 2
#define TREE_MESH 4



extern TCITEMA** import_tabs;
extern int active_import_tab;
extern bool recompute_normals;
extern bool apply_scale;
LRESULT CALLBACK assetc_import_generic_data_wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
HWND init_import_tab(HWND hwnd);

typedef struct {
	HWND wrapper;
	HWND output_file_editctrl;
	HWND enable_bbox;
	HWND bbox[6];
	HWND compute_n; // compute normals (directx math)
	HWND scale_toggle; // apply scale 
	HWND nlods_combo;
	HWND meshdbx_file_editctrl;
	HWND lods_file_editctrl;
	HWND text_hwnd[5];
}generic_import_sec;

typedef struct {
	HWND wrapper;
	HWND tg_select_combo;
	HWND fbx_file_open_btn;
	HWND fbx_file_name;
	HWND vbuffer1;
	HWND vbuffer2;
	HWND ro_vals[4];
	HWND enable_billboard;
	HWND billboard_text;
	HWND text_hwnds[5];
}import_lod_sec;

typedef struct {
	HWND tab_wrapper;
	HWND start_button;
	generic_import_sec generic_settings_hwnds;
	import_lod_sec* lods_settings_hwnds;
	uint8_t mesh_type;
	uint8_t n_lods;
	char* output_name; // output name, this name is used in rigid mesh sets as the havok part data
	char* input_fbx_file[4];
	vxbuffer vx_buffers[4][2];
	float bounding_box[6]; // mandatory. user specified in case of composite assets. automatic on the rest 
	uint8_t uv_for_tangents[4];
}import_settings;

TCITEMA** init_import_tabs(HWND hwndtab, import_settings* import_data);
void init_generic_settings_wnd(HWND hwndtab, HWND wrapper, import_settings* import_data, int cxChar, int cyChar);
void init_import_lod_sections(HWND hwndtab, import_settings* import_data, int i, int cxChar, int cyChar, bool move_start_btn);
extern import_settings import_data[5];
