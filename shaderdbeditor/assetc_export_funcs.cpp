#include "assetc_export_funcs.h"
#include "assetc_meshset_funcs.h"
#include "assetc_fbx_funcs.h"
#include "assetc_meshdata_funcs.h"
#include "exportfuncs.h"
#include "Resource.h"
#include "showfuncs.h"
#include "show_lv_funcs.h"
#include "assetc_txt_funcs.h"

#define RIGID_MESH 0
#define SKINNED_MESH 1 
#define COMPOSITE_MESH 2
#define TREE_MESH 4

bool run_checks(lod_wnd_sec* lod_sections);
void destroy_lod_sec(lod_wnd_sec* lod_section);
FILE* export_asset_txt = nullptr;

LRESULT CALLBACK assetc_export_wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HDC hdc;
	static char temp_file_name[400];
	static OPENFILENAMEA ofn;
	static uint32_t active_func = 0;
	static TEXTMETRICA tm;
	static int cxChar, cyChar, active_lod, n_sections_prev;
	static NONCLIENTMETRICS metrics;
	static bool meshdata_btn_enable = FALSE;
	static lod_wnd_sec* lod_sections = nullptr;
	static HWND export_start_btn;
	switch (message)
	{	
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case ID_ASSETC_OPEN_SET: // reads the mesh sets, fills all the structs which will be used to generate the next windows, or to display the file if "Edit meshset" is clicked
		{
			ofn = open_dialog(hwnd, "All files (*.*)\0*.*\0(*.rigidmeshset)\0*.rigidmeshset\0(*.skinnedmeshset)\0*.skinnedmeshset\0(*.compositemeshset)\0*.compositemeshset\0(*.treemeshset)\0*.treemeshset\0\0 ");
			int type;
			if (GetOpenFileNameA(&ofn))
			{
				strncpy(temp_file_name, ofn.lpstrFile, 400);
				strncpy(&ac_structs_pool->pool[ac_structs_pool->current_pos], ofn.lpstrFile, 400);
				export_data.meshset_file = &ac_structs_pool->pool[ac_structs_pool->current_pos];
				ac_structs_pool->current_pos += 400;
				SetWindowTextA(open_box.meshset_path_text, show_file_name_only(export_data.meshset_file));
				export_data.meshset_ptr = meshset_handler(temp_file_name, &export_data.mesh_type);
				PostMessage(hwnd, WM_COMMAND, (WPARAM)SHOW_EXPORT_LOD, 0);
			}
			return 0;
		}
		case SHOW_EXPORT_LOD:
		{
			if (!(lod_sections))
			{
				lod_sections = (lod_wnd_sec*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
				ac_structs_pool->current_pos += sizeof(lod_wnd_sec) * 4 + 2;
				n_sections_prev = export_data.meshset_ptr->n_lods;
				for (int i = 0; i < export_data.meshset_ptr->n_lods; i++)
				{
					init_lod_section(hwndAsset, hwnd, base_x, base_y, hfont, &lod_sections[i], i);
					if (i == (export_data.meshset_ptr->n_lods - 1))
					{
						HWND hwnd_export_start_btn = (HWND)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
						ac_structs_pool->current_pos += sizeof(HWND) + 2;
						hwnd_export_start_btn = init_export_btn(hwndAsset, hwnd, &lod_sections[(export_data.meshset_ptr->n_lods - 1)]);
						ShowWindow(hwnd_export_start_btn, 1);
						export_start_btn = hwnd_export_start_btn;
					}
				}
			}
			else 
			{
				if (export_data.meshset_ptr->n_lods > n_sections_prev)
				{
					for (int i = n_sections_prev; i < export_data.meshset_ptr->n_lods; i++)
					{
						init_lod_section(hwndAsset, hwnd, base_x, base_y, hfont, &lod_sections[i], i);
					}
					MoveWindow(export_start_btn, (lod_sections[0].width / 2) - 35, lod_sections[0].height + lod_sections[export_data.meshset_ptr->n_lods - 1].y_pos + 20, 70, 30,1);
				}
				else
				{
					for (int i = n_sections_prev; i > export_data.meshset_ptr->n_lods; i--)
					{
						destroy_lod_sec(&lod_sections[(i-1)]);
						memset(&lod_sections[(i-1)], 0, sizeof(lod_wnd_sec));
					}
					MoveWindow(export_start_btn, (lod_sections[0].width / 2) - 35, lod_sections[0].height + lod_sections[export_data.meshset_ptr->n_lods - 1].y_pos + 20, 70, 30, 1);
				}
			}
			meshdata_btn_enable = TRUE;
			return 0;
		}
		case ID_ASSETC_OPEN_MESHDATA_LOD0:
		{
			active_lod = 0;
			SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_ASSETC_OPEN_MESHDATA, 0);
			return 0;
		}
		case ID_ASSETC_OPEN_MESHDATA_LOD1:
		{
			active_lod = 1;
			SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_ASSETC_OPEN_MESHDATA, 0);
			return 0;
		}
		case ID_ASSETC_OPEN_MESHDATA_LOD2:
		{
			active_lod = 2;
			SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_ASSETC_OPEN_MESHDATA, 0);
			return 0;
		}
		case ID_ASSETC_OPEN_MESHDATA_LOD3:
		{
			active_lod = 3;
			SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_ASSETC_OPEN_MESHDATA, 0);
			return 0;
		}
		case ID_ASSETC_EXPORT_START:
		{
			wchar_t temp[50];
			char guid[34];
			char meshset_txt_file[384] = { 0 };
			char* hex;
			char* zero_offset;
			char* meshdata_file;
			char* dot_index;
			char* slash_index;
			char* txt_file_name;
			int file_name_len;
			int data_pool_rewind = ac_data_pool->current_pos;
			if (!run_checks(lod_sections))
			{
				return 0;
			}
			EnableWindow(export_start_btn, false);
			uint32_t* fsize = (uint32_t*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
			ac_structs_pool->current_pos += sizeof(uint32_t*);
			FbxManager* lSdkManager = NULL;
			FbxScene* lScene = NULL;
			bool lResult = TRUE;
			dot_index = strstr(export_data.meshset_file, ".");
			slash_index = show_file_name_only(export_data.meshset_file);
			file_name_len = dot_index - slash_index;
			strncpy(&ac_structs_pool->pool[ac_structs_pool->current_pos], slash_index, file_name_len);
			strncpy(&meshset_txt_file[0], slash_index, file_name_len);
			txt_file_name = &ac_structs_pool->pool[ac_structs_pool->current_pos];
			ac_structs_pool->current_pos += file_name_len;
			strncpy(&ac_structs_pool->pool[ac_structs_pool->current_pos], ".txt", 5);
			ac_structs_pool->current_pos += 5;
			uint8_t* shapes_subdivision;
			if ((export_data.mesh_type == COMPOSITE_MESH) || (export_data.mesh_type == TREE_MESH))
			{
				shapes_subdivision = (uint8_t*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
				ac_structs_pool->current_pos += sizeof(uint8_t) * export_data.meshset_ptr->lods[0].comp_data->n_shapes;
			}
			else
			{
			    shapes_subdivision = nullptr;
			}
			for (int i = 0; i < export_data.meshset_ptr->n_lods; i++) //the pool is zeroed each pass since the space needed is irrelevant once extraction is made 
			{
				if (Button_GetCheck(lod_sections[i].enable) == BST_CHECKED)
				{
					dot_index = strstr(export_data.meshdata_files[i], ".");
					slash_index = show_file_name_only(export_data.meshdata_files[i]);
					file_name_len = dot_index - slash_index;
					strncpy(&ac_structs_pool->pool[ac_structs_pool->current_pos], slash_index, file_name_len);
					export_data.output_file[i] = (&ac_structs_pool->pool[ac_structs_pool->current_pos]);
					ac_structs_pool->current_pos += file_name_len + 1;
					meshdata_file = file_load(export_data.meshdata_files[i], fsize);
					zero_offset = meshdata_file;
					InitializeSdkObjects(lSdkManager, lScene);
					export_data.meshdata_ptrs[i] = meshdata_reader(meshdata_file);
					ComboBox_GetText(lod_sections[i].vbuffer1, temp, 49);
					hex = assetc_guid_convert(&temp[0],ac_data_pool);
					int index = seek_guid_vx(assetc_n_vx_buffers, assetc_vx_buffers, hex);
					export_data.vx_buffers[i][0] = assetc_vx_buffers[index];
					ComboBox_GetText(lod_sections[i].vbuffer2, temp, 49);
					hex = assetc_guid_convert(&temp[0],ac_data_pool);
					index = seek_guid_vx(assetc_n_vx_buffers, assetc_vx_buffers, hex);
					export_data.vx_buffers[i][1] = assetc_vx_buffers[index];
					mesh_extractor_main(lSdkManager, lScene, export_data.meshdata_ptrs[i], export_data.meshset_ptr, export_data.vx_buffers[i], export_data.mesh_type, 0, export_data.output_file[i],shapes_subdivision);
					DestroySdkObjects(lSdkManager, lResult);
					lSdkManager = NULL;
					lScene = NULL;
					lResult = TRUE;
					if (i == 0) // txt funcs here
					{
						FILE* asset_txt = create_txt_file(txt_file_name);
						write_config(asset_txt);
						write_type_only(asset_txt, export_data.mesh_type);
						if ((export_data.mesh_type == COMPOSITE_MESH) || (export_data.mesh_type == TREE_MESH))
						{
						//	write_meshset_info(asset_txt, export_data.mesh_type, export_data.meshset_ptr->lods[0].comp_data->n_shapes, &export_data.meshset_ptr->lods[0], export_data.meshset_ptr->lods_file);
						//	write_meshdata_info(asset_txt, export_data.meshdata_ptrs[0]->nodes, export_data.meshdata_ptrs[0]->info, export_data.meshdata_ptrs[0]->n_nodes);
							write_shapes_composite(asset_txt, shapes_subdivision, export_data.meshset_ptr->lods[0].comp_data->n_shapes, export_data.meshset_ptr->lods[0].comp_data->shapes);
						}
						else if (export_data.mesh_type == RIGID_MESH)
						{		
						//	write_meshset_info(asset_txt, export_data.mesh_type, 1, &export_data.meshset_ptr->lods[0], export_data.meshset_ptr->lods_file);
						//	write_meshdata_info(asset_txt, export_data.meshdata_ptrs[0]->nodes, export_data.meshdata_ptrs[0]->info, export_data.meshdata_ptrs[0]->n_nodes);
							write_shapes_rigid(asset_txt, &export_data.meshset_ptr->lods[0].bounding_box[0], txt_file_name);
						}
						fclose(asset_txt);
					}
					ZeroMemory(zero_offset, *fsize + sizeof(node_geometry));
					ac_data_pool->current_pos = data_pool_rewind;
				}
			}
			EnableWindow(export_start_btn, true);
			return 0;
		}
		case ID_ASSETC_OPEN_MESHDATA:
		{
			ofn = open_dialog(hwnd, "Mesh Data(*.meshdata)\0*.meshdata\0\0 ");
			if (GetOpenFileNameA(&ofn))
			{
				strncpy(&ac_structs_pool->pool[ac_structs_pool->current_pos], ofn.lpstrFile, 400);
				export_data.meshdata_files[active_lod] = &ac_structs_pool->pool[ac_structs_pool->current_pos];
				SetWindowTextA(lod_sections[active_lod].file_text, show_file_name_only(export_data.meshdata_files[active_lod]));
				ac_structs_pool->current_pos += 400;
			}
		}
		break;
		}
	break;
	}
	case WM_DESTROY: 
		lod_sections = nullptr;
		break;
    }	
	return CallWindowProc(ASSETC_WND_PROC, hwnd, message, wParam, lParam);
}


bool run_checks(lod_wnd_sec* lod_sections)
{
	wchar_t temp[64] = { 0 };
	uint32_t strlenw;
	for (int i = 0; i < export_data.meshset_ptr->n_lods; i++) //the pool is zeroed each pass since the space needed is irrelevant once extraction is made 
	{
		if (Button_GetCheck(lod_sections[i].enable) == BST_CHECKED)
		{
			if (!export_data.meshdata_files[i])
			{
				MessageBoxA(lod_sections[i].wrapper, "No Meshdata File Selected", "Error", MB_OK);
				goto exit;
			}
			else
			{
				if ((!(strstr(export_data.meshdata_files[i], "meshdata"))) || (!(strstr(export_data.meshdata_files[i], "MeshData"))))
				{
		//			MessageBoxA(lod_sections[i].wrapper, "Incorrect Mesh File Type", "Error", MB_OK);
		//			goto exit;
				}
			}
			ComboBox_GetText(lod_sections[i].vbuffer1, temp, 64);
			strlenw = wcslen(&temp[0]);
			if (strlenw == 0)
			{
				MessageBoxA(lod_sections[i].wrapper, "No Vertex Buffer 1 Selected", "Error", MB_OK);
				goto exit;
			}
			ComboBox_GetText(lod_sections[i].vbuffer2, temp, 64);
			strlenw = wcslen(&temp[0]);
			if (strlenw == 0)
			{
				MessageBoxA(lod_sections[i].wrapper, "No Vertex Buffer 2 Selected", "Error", MB_OK);
				goto exit;
			}
		}	
	}
	return true;

exit:
    return false;
}


HWND init_tab(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    HWND hwndresult = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        WC_TABCONTROLA,
        "TITLE",
       WS_CHILD | WS_EX_RIGHTSCROLLBAR,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right-rc.left, rc.bottom-rc.top,
        hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
    if (hwndresult == NULL)
    {
        MessageBoxA(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
	return hwndresult;
}

void init_open_meshset(HWND hwndparent, HWND hwndtab, export_fopen_box* open_box, int cxChar, int cyChar, HFONT font)
{
    RECT rc_tab;
	HDC hdc = GetDC(hwndtab);
    GetClientRect(hwndtab, &rc_tab);
    int x = (rc_tab.right - (rc_tab.right * 0.95))/2;
    int y = x;
	open_box->btn_wrapper = CreateWindowExA(WS_EX_WINDOWEDGE,WC_BUTTONA,"Mesh Set",WS_CHILD | BS_GROUPBOX,x,x, rc_tab.right * 0.95, rc_tab.right * 0.132, hwndtab, NULL, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
	SendMessage(hwndtab, WM_SETFONT, (WPARAM)font, MAKELPARAM(TRUE, 0));
	SendMessage(open_box->btn_wrapper,WM_SETFONT,(WPARAM)font, MAKELPARAM(TRUE, 0));
	ShowWindow(open_box->btn_wrapper, 1);
	open_box->meshset_path_text = CreateWindowExA(WS_EX_WINDOWEDGE,WC_EDITA,0,WS_CHILD | ES_READONLY,rc_tab.right*0.06, rc_tab.right *0.08, cxChar*50, cyChar+4, hwndtab, NULL, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
	ShowWindow(open_box->meshset_path_text, 1);
	RECT rc_text;
	GetClientRect(open_box->meshset_path_text, &rc_text);
	open_box->open_meshset_btn = CreateWindowExA(
		WS_EX_WINDOWEDGE,
		WC_BUTTONA,
		"Open",
		WS_CHILD| BS_CENTER | BS_PUSHBUTTON,
		rc_text.right+34, // x
		rc_tab.right * 0.08-1, // y
		cxChar * 10, // width
		cyChar + 6, // height
		hwndtab, (HMENU)ID_ASSETC_OPEN_SET, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
	SendMessage(open_box->open_meshset_btn, WM_SETFONT, (WPARAM)font, MAKELPARAM(TRUE, 0));
	ShowWindow(open_box->open_meshset_btn, 1);
    return;
}

void init_lod_section(HWND hwndparent, HWND hwndtab,int cxChar, int cyChar, HFONT hfont, lod_wnd_sec* lod_section, int i)
{
	HMENU btn_id = nullptr;
	const char* lod_title[] = {"Lod 0", "Lod 1", "Lod 2", "Lod 3"};
	switch (i)
	{
	case 0:
		btn_id = (HMENU)ID_ASSETC_OPEN_MESHDATA_LOD0;
		break;
	case 1:
		btn_id = (HMENU)ID_ASSETC_OPEN_MESHDATA_LOD1;
		break;
	case 2:
		btn_id = (HMENU)ID_ASSETC_OPEN_MESHDATA_LOD2;
		break;
	case 3:
		btn_id = (HMENU)ID_ASSETC_OPEN_MESHDATA_LOD3;
		break;

	}
	wchar_t guid[33];
	RECT rc_tab;
	HDC hdc = GetDC(hwndtab);
	GetClientRect(hwndtab, &rc_tab);
	int x = (rc_tab.right - (rc_tab.right * 0.95)) / 2;
	int y = (x + (rc_tab.right * 0.132))*(i+1);
	int offset = (x + (rc_tab.right * 0.14));
	int wrapper_pos_y = ((cyChar + 7) * 5.9) * (i)+offset;
	lod_section->wrapper = CreateWindowExA(WS_EX_WINDOWEDGE, WC_BUTTONA, lod_title[i], WS_CHILD | BS_GROUPBOX, x, wrapper_pos_y, rc_tab.right * 0.95, (cyChar + 7) * 5.5, hwndtab, NULL, (HINSTANCE)GetWindowLongPtr(hwndparent, GWLP_HINSTANCE), NULL);
	SendMessage(lod_section->wrapper, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
	ShowWindow(lod_section->wrapper, 1);
	lod_section->enable = CreateWindowExA(WS_EX_WINDOWEDGE, WC_BUTTONA, "Extract", WS_CHILD | BS_AUTOCHECKBOX | BS_PUSHBUTTON, x + 8, wrapper_pos_y + 20, (cxChar * 20), (cyChar + 4), hwndtab, NULL, (HINSTANCE)GetWindowLongPtr(hwndparent, GWLP_HINSTANCE), NULL);
	SendMessage(lod_section->enable, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
	ShowWindow(lod_section->enable, 1);
	lod_section->file_text = CreateWindowExA(WS_EX_WINDOWEDGE, WC_EDITA, 0, WS_CHILD | ES_READONLY, x+8, wrapper_pos_y +46, cxChar * 50, cyChar + 4, hwndtab, NULL, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
	SendMessage(lod_section->file_text, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
	ShowWindow(lod_section->file_text, 1);
	lod_section->file_open_btn = CreateWindowExA(WS_EX_WINDOWEDGE, WC_BUTTONA, "Open", WS_CHILD | BS_CENTER, (cxChar*50) + 32, wrapper_pos_y + 45, cxChar * 10, cyChar + 5, hwndtab, btn_id, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
	ShowWindow(lod_section->file_open_btn, 1);
	SendMessage(lod_section->file_open_btn, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
	lod_section->vbuffer1_text = CreateWindowExA(WS_EX_WINDOWEDGE, WC_EDITA, "VX Buffer 1", WS_CHILD | ES_READONLY, x+8, wrapper_pos_y +76, cxChar * 12, cyChar + 2, hwndtab, NULL, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
	SendMessage(lod_section->vbuffer1_text, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
	ShowWindow(lod_section->vbuffer1_text, 1);
	lod_section->vbuffer2_text = CreateWindowExA(WS_EX_WINDOWEDGE, WC_EDITA, "VX Buffer 2 (ZOnly)", WS_CHILD | ES_READONLY, x + 8, wrapper_pos_y +102, cxChar * 20, cyChar + 2, hwndtab, NULL, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
	SendMessage(lod_section->vbuffer2_text, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
	ShowWindow(lod_section->vbuffer2_text, 1);
	lod_section->vbuffer1 = CreateWindowExA(WS_EX_WINDOWEDGE, WC_COMBOBOXA, 0, WS_CHILD | CBS_DROPDOWN | CBS_HASSTRINGS | WS_VSCROLL, x + 8 +(cxChar*22), wrapper_pos_y + 72, cxChar * 40, cyChar + 50, hwndtab, NULL, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
	SendMessage(lod_section->vbuffer1, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
	ShowWindow(lod_section->vbuffer1, 1);
	lod_section->vbuffer2 = CreateWindowExA(WS_EX_WINDOWEDGE, WC_COMBOBOXA, 0, WS_CHILD | CBS_DROPDOWN | CBS_HASSTRINGS | WS_VSCROLL, x + 8 + (cxChar *22), wrapper_pos_y + 98, cxChar * 40, cyChar + 50, hwndtab, NULL, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
	SendMessage(lod_section->vbuffer2, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
	ShowWindow(lod_section->vbuffer2, 1);
	for (uint32_t i = 0; i < assetc_n_vx_buffers; i++)
	{
		hex_to_wchar(guid, assetc_vx_buffers[i].vxbuffer_id,16);
		ComboBox_AddString(lod_section->vbuffer1, guid);
		ComboBox_AddString(lod_section->vbuffer2, guid);
	}	
	lod_section->x_pos = x;
	lod_section->y_pos = wrapper_pos_y;
	lod_section->width = rc_tab.right * 0.95;
	lod_section->height = (cyChar + 7) * 5.5;
	return;
}


char* file_load(char* file_name,uint32_t* file_size)
{
	FILE* file = fopen(file_name, "r+b");
	int fsize;
	fseek(file, SEEK_SET, SEEK_END);
	*file_size = ftell(file);
	fsize = *file_size;
	fseek(file, 0, SEEK_SET);
	char* file_buffer = &ac_data_pool->pool[ac_data_pool->current_pos];
	fread(file_buffer, 1, fsize, file);
	ac_data_pool->current_pos += fsize;
	fclose(file);
	return file_buffer;
}

HWND init_export_btn(HWND hwndparent, HWND hwndtab, lod_wnd_sec* lod_section)
{
	RECT rc;
	HDC hdc = GetDC(hwndtab);
	GetClientRect(lod_section->wrapper, &rc);
	HWND btn = CreateWindowExA(WS_EX_WINDOWEDGE, WC_BUTTONA, "EXTRACT", WS_CHILD | BS_CENTER, (lod_section->width/2)-35, lod_section->height+lod_section->y_pos+20, 70, 30, hwndtab, (HMENU)ID_ASSETC_EXPORT_START, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
	return btn; 
}


void destroy_lod_sec(lod_wnd_sec* lod_section)
{
	DestroyWindow(lod_section->wrapper);
	DestroyWindow(lod_section->enable);
	DestroyWindow(lod_section->file_text);
	DestroyWindow(lod_section->file_open_btn);
	DestroyWindow(lod_section->vbuffer1);
	DestroyWindow(lod_section->vbuffer2);
	DestroyWindow(lod_section->vbuffer1_text);
	DestroyWindow(lod_section->vbuffer2_text);
	return;
}
