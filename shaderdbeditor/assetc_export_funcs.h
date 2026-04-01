#pragma once
#include "assetc.h"
#include "assetc_structs.h"

HWND init_tab(HWND hwnd);

typedef struct
{
	RECT rc_wrapper;
	HWND open_meshset_btn;
	HWND meshset_path_text;
	HWND btn_wrapper;
} export_fopen_box;

extern export_fopen_box open_box;

typedef struct
{
	uint32_t x_pos;
	uint32_t y_pos;
	uint32_t width;
	uint32_t height;
	HWND wrapper;
	HWND enable;
	HWND file_text;
	HWND file_open_btn;
	HWND vbuffer1;
	HWND vbuffer2;
	HWND vbuffer1_text;
	HWND vbuffer2_text;
}lod_wnd_sec;


typedef struct
{
	uint32_t selected_lods[4]; // 1 = extract, 0 = skip
	char* meshdata_files[4];  
	vxbuffer vx_buffers[4][2]; 
	char* output_file[4];
	char* meshset_file;
	uint32_t mesh_type;
	meshset* meshset_ptr;
	meshdata* meshdata_ptrs[4];
} export_settings;

extern lod_wnd_sec* lod_sections;
extern export_settings export_data;


void init_open_meshset(HWND hwndparent, HWND hwndtab, export_fopen_box* open_box, int cxChar, int cyChar,HFONT hfont);
void init_lod_section(HWND hwndparent, HWND hwndtab, int cxChar, int cyChar, HFONT hfont, lod_wnd_sec* lod_section, int i);
char* file_load(char* file_name,uint32_t* file_size);
HWND init_export_btn(HWND hwndparent, HWND hwndtab, lod_wnd_sec* lod_section);
LRESULT CALLBACK assetc_export_wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);