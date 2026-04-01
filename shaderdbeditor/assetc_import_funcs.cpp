#include "assetc_import_funcs.h"
#include "assetc_meshdata_funcs.h"
#include <vector>
#include <algorithm>

#define N_TABS 3
#define N_LODS_COMBO 3 
#define MESHDATA_POOL_SIZE 12000000
#define MESHSET_POOL_SIZE 128000
#define MESHDATA_HEADER_POOL_SIZE 48000
#define MESHSET_STRUCTS_POOL_SIZE 64000

WNDPROC OLD_WRAPPER_PROC;
int active_tab; // this is mesh type internally. rigid = 0, etc
FILE* import_asset_txt;
bool recompute_normals = false;
bool apply_scale = false; 
bool setup_pool = true;


void write_material_names(FbxNode* root, int* meshset_pool_cp, int n_nodes);
void write_material_names_v2(FbxNode* root, shader_set* shaderset, int* meshset_structs_cp, int n_nodes, char* meshset_structs);
void destroy_lod_sec(import_settings* import_data, int j);
shape_struct* get_shapes(FbxNode* root, int n_nodes,int* n_shapes_ptr);
bool run_checks_import();

// 2 procedures. 1 for the generic data, one for import_data->tab_wrapper. the only thing that needs to be done is set the active tab 
LRESULT CALLBACK assetc_import_generic_data_wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)                                                                                                    
{
    active_tab = active_import_tab;
    static int n_lods_array[5] = {-1,-1,-1,-1,-1};
	switch (message)
	{
    case WM_COMMAND:
    {
        switch(HIWORD(wParam))
        {
        case CBN_SELENDOK:
        {
            int i = 5;
            break;
        }
        case CBN_SELCHANGE:
        {
            case ID_ASSETC_IMPORT_DATA_LODS_COMBO:
            {                
				int i = ComboBox_GetCurSel((HWND)lParam);
                if (n_lods_array[active_tab] == -1)
                {
                    n_lods_array[active_tab] = i;
                    import_data[active_tab].n_lods = i + 1;
                    import_data[active_tab].lods_settings_hwnds = (import_lod_sec*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
                    ac_structs_pool->current_pos += sizeof(import_lod_sec) * 4;
                    for (int j = 0; j < import_data[active_tab].n_lods; j++)
                    {
                        init_import_lod_sections(hwndImportTab, &import_data[active_tab], j, cxChar, cyChar, 0);
                    }
                    break;
                }
                else if (n_lods_array[active_tab] < i)
                {
                    int lods_to_add = i - n_lods_array[active_tab];
                    import_data[active_tab].n_lods += lods_to_add;
                    for (int j = (n_lods_array[active_tab] + 1); j < (i + 1); j++)
                    {
                        init_import_lod_sections(hwndImportTab, &import_data[active_tab], j, cxChar, cyChar, 1);
                    }
                    n_lods_array[active_tab] = i;
                    break;
                }
                else if (n_lods_array[active_tab] > i)
                {
                    RECT tab; 
                    GetWindowRect(import_data[active_tab].tab_wrapper, &tab);
                    HRGN tab_rgn = CreateRectRgn(tab.left,tab.top,tab.right,tab.bottom);
                    for (int j = 0; j < import_data[active_tab].n_lods; j++)
                    {
                        destroy_lod_sec(&import_data[active_tab], j);
                    }
                    ZeroMemory(import_data[active_tab].lods_settings_hwnds, sizeof(import_lod_sec)*4);
                    RedrawWindow(import_data[active_tab].tab_wrapper, &tab, tab_rgn, RDW_INVALIDATE| RDW_UPDATENOW | RDW_ALLCHILDREN);
                    n_lods_array[active_tab] = i;
                    import_data[active_tab].n_lods = i + 1;
                    for (int j = 0; j < import_data[active_tab].n_lods; j++)
                    {
                        init_import_lod_sections(hwndImportTab, &import_data[active_tab], j, cxChar, cyChar, 1);
                    }
                    break;
                }
                else break;
            }
            break;
        }
        case BN_CLICKED:
        {
            if (LOWORD(wParam) == ID_ASSETC_IMPORT_DATA_BBOX_ENABLE)
            {
				if (Button_GetCheck((HWND)lParam) == BST_CHECKED)
				{
					for (int j = 0; j < 6; j++)
					{
						EnableWindow(import_data[active_tab].generic_settings_hwnds.bbox[j], 1);
					}
				}
				else
				{
					for (int j = 0; j < 6; j++)
					{
						EnableWindow(import_data[active_tab].generic_settings_hwnds.bbox[j], 0);
					}
				}
            }
            break;
        }
        
        }          
        break;
    }
    case WM_DESTROY:
        for (int i = 0; i < 5; i++)
        {
            n_lods_array[i] = -1;
        }
        setup_pool = true;
        return 0;
    }
	return CallWindowProc(OLD_WRAPPER_PROC, hwnd, message, wParam, lParam);
}


LRESULT CALLBACK assetc_import_lod_wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) //this procedure is for the lods tabs wrapper. name is misleading 
{
    static int active_lod;
    static const char* uvs_selection[5] = { "0","1","2","3","4"};
    static OPENFILENAMEA ofn;
    switch (message)
    {  
    case WM_COMMAND:
    {
        switch (HIWORD(wParam))
        {
        case CBN_SELCHANGE:
        {
            switch (LOWORD(wParam))
            {
            case ID_ASSETC_IMPORT_DATA_VX1:
            {
                wchar_t temp[50];
                char guid[34];
                char* hex;
                int uv_channel=0;
                active_lod = 0;
                while (((HWND)lParam != import_data[active_tab].lods_settings_hwnds[active_lod].vbuffer1) && (active_lod < import_data[active_tab].n_lods))
                {
                    active_lod++;
                }
                // get vbuffer index
                // check how much uv channels it has 
                // insert items to tangent combo box 
                int index = ComboBox_GetCurSel(import_data[active_tab].lods_settings_hwnds[active_lod].vbuffer1);
                import_data[active_tab].vx_buffers[active_lod][0] = assetc_vx_buffers[index];
                if (*import_data[active_tab].vx_buffers[active_lod][0].type == 4294967040)
                {
                    ComboBox_ResetContent(import_data[active_tab].lods_settings_hwnds[active_lod].tg_select_combo);
                    for (int i = 0; i < *import_data[active_tab].vx_buffers[active_lod][0].n_data_types; i++)
                    {
                        if (*import_data[active_tab].vx_buffers[active_lod][0].types_array[i] == 6)
                        {
                            ComboBox_AddString(import_data[active_tab].lods_settings_hwnds[active_lod].tg_select_combo, uvs_selection[uv_channel]);
                            uv_channel++;
                        }
                    }
                }
                break;
            }
            case ID_ASSETC_IMPORT_DATA_VX2:
            {
                wchar_t temp[50];
                char guid[34];
                char* hex;
                active_lod = 0;
                while (((HWND)lParam != import_data[active_tab].lods_settings_hwnds[active_lod].vbuffer2) && (active_lod < import_data[active_tab].n_lods))
                {
                    active_lod++;
                }
                int index = ComboBox_GetCurSel(import_data[active_tab].lods_settings_hwnds[active_lod].vbuffer2);
                import_data[active_tab].vx_buffers[active_lod][1] = assetc_vx_buffers[index];
              //  ComboBox_ResetContent(import_data[active_tab].lods_settings_hwnds[active_lod].tg_select_combo);
                break;
            }
            }
        }
        case BN_CLICKED:
        {
            switch (LOWORD(wParam))
            {
            case ID_ASSETC_OPEN_FBX:
            {
				int j = 0;
				bool stay = true;
				while (((HWND)lParam != import_data[active_tab].lods_settings_hwnds[j].fbx_file_open_btn) && (j < import_data[active_tab].n_lods))
				{
					j++;
				}
				if (j < 5)
				{
                    ofn = open_dialog(hwnd, "FBX Scene(*.fbx)\0*.fbx\0\0");
                    int type;
                    if (GetOpenFileNameA(&ofn))
                    {
                        strncpy(&ac_structs_pool->pool[ac_structs_pool->current_pos], ofn.lpstrFile, 400);
                        import_data[active_tab].input_fbx_file[j] = &ac_structs_pool->pool[ac_structs_pool->current_pos];
                        ac_structs_pool->current_pos += 400;
                        SetWindowTextA(import_data[active_tab].lods_settings_hwnds[j].fbx_file_name, show_file_name_only(import_data[active_tab].input_fbx_file[j]));
                    }
                }
                break;
            }
            case ID_ASSETC_IMPORT_START:
            {
                if (!run_checks_import())
                {
                    break;
                }
                EnableWindow(import_data[active_tab].start_button, false);
				wchar_t temp_wc[400];
				wchar_t mesh_dbx_wc[400];
				char temp[400];
				char mesh_dbx[400];
                char lods_file[200];
                char file_name_base[100];
				int temp_strlen, mesh_dbx_strlen, meshset_pool_cp, meshset_start, header_pool_cp, file_pool_cp, meshset_cp;
                static char* data_pool_meshset = nullptr;
                static char* data_pool_header = nullptr;
                static char* data_pool_file = nullptr;
                static char* meshset_structs = nullptr; 
                float bbox_rigid[6];
				uint32_t* temp_int;
				uint8_t* tbyte;
                uint8_t* shape_division = nullptr;
                if (setup_pool)
                {
                    data_pool_meshset = &ac_import_pool->pool[ac_import_pool->current_pos]; // this is the beginning of the meshet file. do not write unecessary shit here.
                    meshset_pool_cp = 0;
                    meshset_start = meshset_pool_cp;
                    ac_import_pool->current_pos += MESHSET_POOL_SIZE;
                    data_pool_header = &ac_import_pool->pool[ac_import_pool->current_pos];
                    header_pool_cp = 0;
                    ac_import_pool->current_pos += MESHDATA_HEADER_POOL_SIZE;
                    meshset_structs = &ac_import_pool->pool[ac_import_pool->current_pos];
                    meshset_cp = 0;
                    ac_import_pool->current_pos += MESHSET_STRUCTS_POOL_SIZE;
                    data_pool_file = &ac_import_pool->pool[ac_import_pool->current_pos];
                    file_pool_cp = 0;
                    ac_import_pool->current_pos += MESHDATA_POOL_SIZE;
                    setup_pool = false;
                }
                else
                {
                    meshset_pool_cp = 0;
                    header_pool_cp = 0;
                    meshset_cp = 0;
                    file_pool_cp = 0;
                }
                GetWindowTextA(import_data[active_tab].generic_settings_hwnds.output_file_editctrl,file_name_base,100);              
				// make loop. meshset file remains alive, fbx scene opens and closes for each lod 
				// mesh type 0 is rigid, is a composite mesh with only 1 shape per node. same for tree.  only one function needed. 
				// mesh type skinned (3 and 4) is a rigid mesh with bones. so use type 0 with a bool for bones. 
				// allocate a part of the data pool to be alive during the loop, and one that is gonna be cleaned each iteration. 
				// if no mesh dbx is specified, we assume its the weird meshset type without mesh dbx property
				// meshset header 
                meshset* meshset_file = (meshset*)(&meshset_structs[meshset_cp]);
                meshset_cp += sizeof(meshset);
                meshset_file->magic = &meshset_structs[meshset_cp];
                meshset_file->magic[0] = 0x2A;
                meshset_cp += 5;
                meshset_file->lods_file = &meshset_structs[meshset_cp];
                meshset_file->n_lods = import_data[active_tab].n_lods;
				char magic[5] = { 0x2A,0x00,0x00,0x00,0x00 };
				memcpy(&ac_import_pool->pool[meshset_pool_cp], magic, 5);
				meshset_pool_cp += 5;
				ComboBox_GetText(import_data[active_tab].generic_settings_hwnds.lods_file_editctrl, temp_wc, 400);
				temp_strlen = wcstombs(lods_file, temp_wc, 400);
				strncpy(&ac_import_pool->pool[meshset_pool_cp], lods_file, temp_strlen + 1);
                strncpy(&meshset_structs[meshset_cp], lods_file, temp_strlen + 1);
                meshset_cp += temp_strlen + 1; 
                meshset_file->lods = (meshset_lod*)(&meshset_structs[meshset_cp]);
                meshset_cp += sizeof(meshset_lod) * meshset_file->n_lods;
				meshset_pool_cp += temp_strlen + 1;
				temp_int = (uint32_t*)(&ac_import_pool->pool[meshset_pool_cp]);
				*temp_int = import_data[active_tab].n_lods;
				meshset_pool_cp += 4;
				ZeroMemory(&temp_wc[0], 400);
				GetWindowText(import_data[active_tab].generic_settings_hwnds.meshdbx_file_editctrl, mesh_dbx_wc, 400);
				mesh_dbx_strlen = wcstombs(mesh_dbx, mesh_dbx_wc, 400);
				FbxManager* lSdkManager = NULL;
				FbxScene* lScene = NULL;
				bool lResult;
                bool computebbox;
                if (Button_GetCheck(import_data[active_tab].generic_settings_hwnds.enable_bbox) == BST_UNCHECKED) // read bbox from hwnds only once 
                {
                    computebbox = true;
                }
                if (Button_GetCheck(import_data[active_tab].generic_settings_hwnds.compute_n) == BST_CHECKED) // read bbox from hwnds only once 
                {
                    recompute_normals = true;
                }
                if (Button_GetCheck(import_data[active_tab].generic_settings_hwnds.scale_toggle) == BST_CHECKED) // read bbox from hwnds only once 
                {
                    apply_scale = true;
                }
                char txt_file_name[200];
                GetWindowTextA(import_data[active_tab].generic_settings_hwnds.output_file_editctrl, txt_file_name, 200);
                temp_strlen = strlen(&txt_file_name[0]);
                strcpy(&txt_file_name[temp_strlen], ".txt");
                import_asset_txt = create_txt_file(&txt_file_name[0]);
				// main loop. write to meshset buffer, then convert to meshdata. both things need to be done in the same loop
				for (int lod = 0; lod < import_data[active_tab].n_lods; lod++)
				{
                    meshset_file->lods[lod].has_billboard = false;
                    meshset_file->lods[lod].shader_sets = (shader_set*)(&meshset_structs[meshset_cp]);
                    meshset_cp += sizeof(shader_set);
                    meshset_file->lods[lod].shader_sets->mesh_dbx_path = &mesh_dbx[0];
                    meshset_file->lods[lod].n_shader_sets = 1;
					InitializeSdkObjects(lSdkManager, lScene);                   
					ZeroMemory(&temp[0], 400);
					GetWindowTextA(import_data[active_tab].lods_settings_hwnds[lod].fbx_file_name, temp, 400);
					lResult = LoadScene(lSdkManager, lScene, import_data[active_tab].input_fbx_file[lod]);
                    FbxNode* root = lScene->GetRootNode();
                    int n_nodes = root->GetChildCount();
                    meshset_file->lods[lod].n_shaders_per_set = n_nodes;
                    write_material_names_v2(root, meshset_file->lods[lod].shader_sets, &meshset_cp, n_nodes,meshset_structs);
                    shape_struct* shapes = nullptr;
                    meshset_lod* ms_lod = nullptr;
                    int n_shapes;
                    if (import_data[active_tab].mesh_type == RIGID_MESH)
                    {
                        meshset_file->lods[lod].rigid_data = (rigid_mesh_data*)(&meshset_structs[meshset_cp]);
                        meshset_cp += sizeof(rigid_mesh_data);
                        meshset_file->lods[lod].rigid_data->data = &meshset_structs[meshset_cp];
                    // create rigid here. no shape structs needed for meshset, no sorting of names
                        ms_lod = mesh_generator_main(root, &import_data[active_tab], shapes, data_pool_file, file_pool_cp, 0, lod, &file_name_base[0],nullptr);
                        meshset_file->lods[lod].mesh_size = ms_lod->mesh_size;
                    }
                    else if (((import_data[active_tab].mesh_type == COMPOSITE_MESH)) || ((import_data[active_tab].mesh_type == TREE_MESH)))
                    {
                    // composite and tree       
                        meshset_file->lods[lod].comp_data = (comp_n_tree_data*)(&meshset_structs[meshset_cp]);
                        meshset_cp += sizeof(comp_n_tree_data);
                        meshset_file->lods[lod].comp_data->data = &meshset_structs[meshset_cp];
                        meshset_file->lods[lod].comp_data->data[4] = 0x01;
                        int* n_shapes_ptr = &n_shapes;
                        shapes = get_shapes(root, n_nodes, n_shapes_ptr);
                        meshset_file->lods[lod].comp_data->shapes = shapes;
                        meshset_file->lods[lod].comp_data->n_shapes = n_shapes;
                        meshset_file->lods[lod].comp_data->unknown_data = (&meshset_structs[meshset_cp]);
                        meshset_cp += 24* n_shapes;
                        shape_division = (uint8_t*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
                        ac_structs_pool->current_pos += sizeof(uint8_t) * n_shapes;
                        ms_lod = mesh_generator_main(root,&import_data[active_tab],shapes, data_pool_file, file_pool_cp,n_shapes,lod,&file_name_base[0],shape_division);
                        meshset_file->lods[lod].mesh_size = ms_lod->mesh_size;
                    }
                    else
                    {
                    // skinned
                    
                    }

                    if (computebbox)
                    {
                        FbxVector4 bboxmin, bboxmax, bboxcenter;
                        lScene->ComputeBoundingBoxMinMaxCenter(bboxmin, bboxmax, bboxcenter,0);
                        ms_lod->bounding_box[0] = bboxmin[0];
                        ms_lod->bounding_box[1] = bboxmin[1];
                        ms_lod->bounding_box[2] = bboxmin[2];
                        ms_lod->bounding_box[3] = bboxmax[0];
                        ms_lod->bounding_box[4] = bboxmax[1];
                        ms_lod->bounding_box[5] = bboxmax[2];
                        meshset_file->lods[lod].bounding_box[0] = bboxmin[0];
                        meshset_file->lods[lod].bounding_box[1] = bboxmin[1];
                        meshset_file->lods[lod].bounding_box[2] = bboxmin[2];
                        meshset_file->lods[lod].bounding_box[3] = bboxmax[0];
                        meshset_file->lods[lod].bounding_box[4] = bboxmax[1];
                        meshset_file->lods[lod].bounding_box[5] = bboxmax[2];
                        memcpy(&ac_import_pool->pool[meshset_pool_cp], ms_lod->bounding_box, 24);
                        if ((import_data[active_tab].mesh_type == RIGID_MESH) && (lod == 0))
                        {
                            bbox_rigid[0] = bboxmin[0];
                            bbox_rigid[1] = bboxmin[1];
                            bbox_rigid[2] = bboxmin[2];
                            bbox_rigid[3] = bboxmax[0];
                            bbox_rigid[4] = bboxmax[1];
                            bbox_rigid[5] = bboxmax[2];                     
                        }
                    }
                    else
                    {
                        float bbox[6]; 
                        char float_val[50];
                        for (int i = 0; i < 6; i++)
                        {
                            GetWindowTextA(import_data[active_tab].generic_settings_hwnds.bbox[i], float_val, 50);
                            bbox_rigid[i] = atof(&float_val[i]);
                        }
                        memcpy(&ac_import_pool->pool[meshset_pool_cp], &bbox[0], 24);
                        memcpy(&meshset_file->lods[lod].bounding_box[0], &bbox[0], 24);
                    }
                    meshset_pool_cp += 24;
                    temp_int = (uint32_t*)(&ac_import_pool->pool[meshset_pool_cp]);
                    *temp_int = ms_lod->mesh_size;
                    meshset_pool_cp += 9;
                    switch (import_data[active_tab].mesh_type)
                    {
                    case RIGID_MESH:
                        break;
                    case SKINNED_MESH:
                        break;
                    case COMPOSITE_MESH:
                        meshset_pool_cp += write_shapes(shapes,n_nodes,n_shapes,meshset_pool_cp, data_pool_meshset);
                        if (Button_GetCheck(import_data[active_tab].lods_settings_hwnds[lod].enable_billboard) == BST_CHECKED)
                        {
                            int str_len;
                            GetWindowTextA(import_data[active_tab].lods_settings_hwnds[lod].billboard_text, temp, 400);
                            str_len = strlen(&temp[0]);
                            strncpy(&data_pool_meshset[meshset_pool_cp], temp, str_len + 1);
                            strcpy(&meshset_structs[meshset_cp],&temp[0]);
                            meshset_file->lods[lod].comp_data->billboard = &meshset_structs[meshset_cp];
                            meshset_cp += strlen(&meshset_structs[meshset_cp]) + 1;
                            meshset_file->lods[lod].has_billboard = true;
                            meshset_pool_cp += str_len + 1;
                        }
                        break;
                    case TREE_MESH:
                        meshset_pool_cp += write_shapes(shapes, n_nodes, n_shapes, meshset_pool_cp, data_pool_meshset);
                        if (Button_GetCheck(import_data[active_tab].lods_settings_hwnds[lod].enable_billboard) == BST_CHECKED)
                        {
                            int str_len;
                            GetWindowTextA(import_data[active_tab].lods_settings_hwnds[lod].billboard_text, temp, 400);
                            str_len = strlen(&temp[0]);
                            strncpy(&data_pool_meshset[meshset_pool_cp], temp, str_len + 1);
                            strcpy(&meshset_structs[meshset_cp], &temp[0]);
                            meshset_file->lods[lod].comp_data->billboard = &meshset_structs[meshset_cp];
                            meshset_cp += strlen(&meshset_structs[meshset_cp]) + 1;
                            meshset_file->lods[lod].has_billboard = true;
                            meshset_pool_cp += str_len + 1;
                        }
                        break;
                    }
                    DestroySdkObjects(lSdkManager, lResult);
                    memset(data_pool_file, 0, 12000000);
                    file_pool_cp = 0;
				}         
                meshset_cp = 0;
                meshset_pool_cp = 0;
                header_pool_cp = 0;
                write_config(import_asset_txt);
                write_type_only(import_asset_txt, import_data[active_tab].mesh_type);
                if (import_data[active_tab].mesh_type == RIGID_MESH)
                {
                    write_shapes_rigid(import_asset_txt, &bbox_rigid[0], file_name_base);
                }
                else if ((import_data[active_tab].mesh_type == COMPOSITE_MESH)|| (import_data[active_tab].mesh_type == TREE_MESH))
                {
                    write_shapes_composite(import_asset_txt, shape_division, meshset_file->lods[0].comp_data->n_shapes, meshset_file->lods[0].comp_data->shapes);
                }
                fclose(import_asset_txt);
                meshset_writer(meshset_file,&file_name_base[0], import_data[active_tab].mesh_type);
                EnableWindow(import_data[active_tab].start_button, true);
            }
                break;         
            case ID_ASSETC_IMPORT_DATA_BBOARD_ENABLE:
            {
                int j = 0;
				while (((HWND)lParam != import_data[active_tab].lods_settings_hwnds[j].enable_billboard) && (j < import_data[active_tab].n_lods))
				{
					j++;
				}
				bool isenabled = IsWindowEnabled(import_data[active_tab].lods_settings_hwnds[j].billboard_text);
				EnableWindow(import_data[active_tab].lods_settings_hwnds[j].billboard_text, (!isenabled));
            }
                break;
            }
            break;
        }
        return 0;
        }
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(hwnd, &ps);
        return 0;
    }

    }
    return CallWindowProc(OLD_WRAPPER_PROC, hwnd, message, wParam, lParam);
}

shape_struct* get_shapes(FbxNode* root, int n_nodes,int* n_shapes_ptr) // find unique shapes (blender-proof) then create shape structs 
{
    int n_shapes=0;
    int shapes_in_node;
    int temp_len;
    char temp[200];
    const char* temp_ptr;
    const char* dot_ptr;
    std::vector<std::string> names;
    std::vector<std::string>::iterator it;
    FbxNode* child;
    FbxNode* shape;
    FbxVector4 pos;
    shape_struct* shapes = nullptr;
    for (int i = 0; i < n_nodes; i++)
    {
        child = root->GetChild(i);
        shapes_in_node = child->GetChildCount();
        for (int j = 0; j < shapes_in_node; j++)
        {
            shape = child->GetChild(j);
            temp_ptr = shape->GetName();
            strcpy(&temp[0], temp_ptr);
            dot_ptr = strstr(&temp[0],".");
            if (dot_ptr)
            {
                temp_len = dot_ptr - &temp[0];
            }
            else
            {
                temp_len = strlen(&temp[0]);
            }
            strncpy(&ac_structs_pool->pool[ac_structs_pool->current_pos],temp_ptr,temp_len);
            names.push_back(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
            ac_structs_pool->current_pos += temp_len + 1;
        }
    }
    std::sort(names.begin(),names.end());
    it = std::unique(names.begin(),names.end());
    names.resize(std::distance(names.begin(),it));
    n_shapes = names.size();
    *n_shapes_ptr = n_shapes;
    shapes = (shape_struct*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
    ac_structs_pool->current_pos += sizeof(shape_struct) * n_shapes;
    for (int i = 0; i < n_shapes; i++)
    {
        for (int j = 0; j < 9; j++)
        {
            if ((j == 0) || (j == 4) || (j == 8))
            {
                shapes[i].transforms[j] = 1;
            }
            else
            {
                shapes[i].transforms[j] = 0;
            }
        }
        strcpy(&ac_structs_pool->pool[ac_structs_pool->current_pos],&names[i][0]);
        shapes[i].name = &ac_structs_pool->pool[ac_structs_pool->current_pos];
        ac_structs_pool->current_pos += strlen(&names[i][0]) + 1;
        shapes[i].hash = shapenamehasher(names[i]);
        for (int j = 0; j < n_nodes; j++)
        {
            child = root->GetChild(j);
            shape = child->FindChild(shapes[i].name);
            if (shape)
            {
                pos = shape->LclTranslation;
                shapes[i].transforms[9] = pos[0];
                shapes[i].transforms[10] = pos[1];
                shapes[i].transforms[11] = pos[2];
                break;
            }
        }    
    }
    return shapes;
}

void write_material_names(FbxNode* root, int* meshset_pool_cp, int n_nodes)
{
    int strlength;
    char temp[350];
    const char* nameptr;
    const char* dotptr;
    FbxSurfaceMaterial* material;
    FbxNode* child;
    for (int i = 0; i < n_nodes; i++)
    {
        child = root->GetChild(i);
        child = child->GetChild(0);
        material = child->GetMaterial(0);
        nameptr = material->GetName();
        dotptr = strstr(nameptr, ".");
        if (dotptr)
        {
            strlength = dotptr - nameptr;
        }
        else
        {    
            strlength = strlen(nameptr);
        }
        //fprintf(import_asset_txt, "Shader %u:", i);
        //fprintf(import_asset_txt, " %s \n", nameptr);
        strncpy(&ac_import_pool->pool[*meshset_pool_cp], nameptr, strlength);
        *meshset_pool_cp += strlength + 1;
    }
    return;
}

void write_material_names_v2(FbxNode* root, shader_set* shaderset, int* meshset_structs_cp, int n_nodes, char* meshset_structs)
{
    int strlength;
    char temp[350];
    const char* nameptr;
    const char* dotptr;
    FbxSurfaceMaterial* material;
    FbxNode* child;
    for (int i = 0; i < n_nodes; i++)
    {
        child = root->GetChild(i);
        child = child->GetChild(0);
        material = child->GetMaterial(0);
        nameptr = material->GetName();
        dotptr = strstr(nameptr, ".");
        if (dotptr)
        {
            strlength = dotptr - nameptr;
        }
        else
        {
            strlength = strlen(nameptr);
        }
        strncpy(&meshset_structs[*meshset_structs_cp], nameptr, strlength);
        shaderset->shaders[i] = &meshset_structs[*meshset_structs_cp];
        *meshset_structs_cp += strlength + 1;
    }
    return;
}

bool run_checks_import()
{
    char temptxt[384] = { 0 };
    GetWindowTextA(import_data[active_tab].generic_settings_hwnds.output_file_editctrl, &temptxt[0], 128);
    if (!(strlen(&temptxt[0])))
    {
        MessageBoxA(import_data[active_tab].generic_settings_hwnds.wrapper, "Output Name Field Empty", "Error", MB_OK);
        goto exit;
    }
    GetWindowTextA(import_data[active_tab].generic_settings_hwnds.lods_file_editctrl, &temptxt[0], 128);
    if (!(strlen(&temptxt[0])))
    {
        MessageBoxA(import_data[active_tab].generic_settings_hwnds.wrapper, "LOD File path Field Empty", "Error", MB_OK);
        goto exit;
    }
    for (int i = 0; i < import_data[active_tab].n_lods; i++)
    {
        GetWindowTextA(import_data[active_tab].lods_settings_hwnds[i].fbx_file_name, &temptxt[0], 384);
        if (!(strlen(&temptxt[0])))
        {
            MessageBoxA(import_data[active_tab].generic_settings_hwnds.wrapper, "FBX File not selected", "Error", MB_OK);
            goto exit;
        }
        else if (!(strstr(&temptxt[0], "fbx")))
        {
            MessageBoxA(import_data[active_tab].generic_settings_hwnds.wrapper, "Incorrect 3D format", "Error", MB_OK);
            goto exit;
        }
        GetWindowTextA(import_data[active_tab].lods_settings_hwnds[i].vbuffer1, &temptxt[0], 384);
        if (!(strlen(&temptxt[0])))
        {
            MessageBoxA(import_data[active_tab].generic_settings_hwnds.wrapper, "Vertex Buffer 1 not selected", "Error", MB_OK);
            goto exit;
        }
        GetWindowTextA(import_data[active_tab].lods_settings_hwnds[i].vbuffer2, &temptxt[0], 384);
        if (!(strlen(&temptxt[0])))
        {
            MessageBoxA(import_data[active_tab].generic_settings_hwnds.wrapper, "Vertex Buffer 2 not selected", "Error", MB_OK);
            goto exit;
        }
        GetWindowTextA(import_data[active_tab].lods_settings_hwnds[i].tg_select_combo, &temptxt[0], 384);
        if (!(strlen(&temptxt[0])))
        {
            MessageBoxA(import_data[active_tab].generic_settings_hwnds.wrapper, "UV Channel not selected", "Error", MB_OK);
            goto exit;
        }
    }
    return true;
exit:
    return false;
}




void init_generic_settings_wnd(HWND hwndtab, HWND wrapper, import_settings* import_data, int cxChar, int cyChar)
{
    RECT rc_tab;
    HDC hdc = GetDC(hwndtab);
    GetClientRect(hwndtab, &rc_tab);
    int x = (rc_tab.right - (rc_tab.right * 0.95)) / 2;
    int y = (x + (rc_tab.right * 0.132));
    int offset = (x + (rc_tab.right * 0.14));
    int wrapper_pos_y = rc_tab.top + 26;
    const char* text[5] = {"Output Name","Number of LODs","Mesh DBX","LODs setting file"};
    const char* lods_text[4] = {"1","2","3","4"};
    import_data->tab_wrapper = CreateWindowExA(WS_EX_WINDOWEDGE, WC_BUTTONA,0, WS_CHILD | BS_GROUPBOX,x, wrapper_pos_y, rc_tab.right * 0.95, rc_tab.bottom, hwndtab, NULL, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
    import_data->generic_settings_hwnds.wrapper = CreateWindowExA(WS_EX_WINDOWEDGE, WC_BUTTONA, 0, WS_CHILD | BS_GROUPBOX, 0, 0, rc_tab.right * 0.95, 158, import_data->tab_wrapper, NULL, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
    ShowWindow(import_data->generic_settings_hwnds.wrapper, 1);
    int y_pos = 12;
    for (int i = 0; i < 4; i++)
    {
        if (i == 2)
        {
            y_pos += 28;
        }
        import_data->generic_settings_hwnds.text_hwnd[i] = CreateWindowExA(WS_EX_WINDOWEDGE, WC_STATICA, text[i], WS_CHILD | ES_READONLY, 8, y_pos, 90, 18, import_data->tab_wrapper, NULL, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
        SendMessage(import_data->generic_settings_hwnds.text_hwnd[i], WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
        ShowWindow(import_data->generic_settings_hwnds.text_hwnd[i], 1);    
        y_pos += 28;
    }
    import_data->generic_settings_hwnds.output_file_editctrl = CreateWindowExA(WS_EX_WINDOWEDGE, WC_EDITA, 0, WS_CHILD |WS_BORDER | WS_VISIBLE | ES_WANTRETURN | ES_AUTOHSCROLL, 108, 10, rc_tab.right * 0.7, cyChar+6, import_data->tab_wrapper, (HMENU)ID_ASSETC_IMPORT_DATA_OUTPUT_EDIT, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
    import_data->generic_settings_hwnds.nlods_combo = CreateWindowExA(WS_EX_WINDOWEDGE, WC_COMBOBOXA, 0, WS_CHILD | CBS_DROPDOWN | CBS_HASSTRINGS | WS_VSCROLL, 108, 36, rc_tab.right * 0.082, cyChar+4, import_data->generic_settings_hwnds.wrapper, (HMENU)ID_ASSETC_IMPORT_DATA_LODS_COMBO, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
    SendMessage(import_data->generic_settings_hwnds.nlods_combo, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    ShowWindow(import_data->generic_settings_hwnds.nlods_combo, 1);
    import_data->generic_settings_hwnds.enable_bbox = CreateWindowExA(WS_EX_TRANSPARENT, WC_BUTTONA, "Manual BBox", WS_CHILD | BS_AUTOCHECKBOX | BS_PUSHBUTTON, 156, 38, rc_tab.right * 0.19, cyChar + 4, import_data->generic_settings_hwnds.wrapper, (HMENU)ID_ASSETC_IMPORT_DATA_BBOX_ENABLE, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
    SendMessage(import_data->generic_settings_hwnds.enable_bbox, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    ShowWindow(import_data->generic_settings_hwnds.enable_bbox, 1);
    import_data->generic_settings_hwnds.compute_n = CreateWindowExA(WS_EX_TRANSPARENT, WC_BUTTONA, "D3D Nrml", WS_CHILD | BS_AUTOCHECKBOX | BS_PUSHBUTTON, 266, 38, rc_tab.right * 0.14, cyChar + 4, import_data->generic_settings_hwnds.wrapper, 0, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
    SendMessage(import_data->generic_settings_hwnds.compute_n, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    ShowWindow(import_data->generic_settings_hwnds.compute_n, 1);
    import_data->generic_settings_hwnds.scale_toggle = CreateWindowExA(WS_EX_TRANSPARENT, WC_BUTTONA, "Apply Scale", WS_CHILD | BS_AUTOCHECKBOX | BS_PUSHBUTTON, 356, 38, rc_tab.right * 0.18, cyChar + 4, import_data->generic_settings_hwnds.wrapper, 0, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
    SendMessage(import_data->generic_settings_hwnds.scale_toggle, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    ShowWindow(import_data->generic_settings_hwnds.scale_toggle, 1);
    int x_pos_bbox = 8;
    int ids_bbox[6] = {ID_ASSETC_IMPORT_DATA_XMIN, ID_ASSETC_IMPORT_DATA_YMIN ,ID_ASSETC_IMPORT_DATA_ZMIN,ID_ASSETC_IMPORT_DATA_XMAX,ID_ASSETC_IMPORT_DATA_YMAX,ID_ASSETC_IMPORT_DATA_ZMAX };
    for (int i = 0; i < 6; i++)
    {
        import_data->generic_settings_hwnds.bbox[i] = CreateWindowExA(WS_EX_TRANSPARENT, WC_EDITA, 0, WS_CHILD | WS_BORDER | WS_DISABLED | ES_WANTRETURN | ES_AUTOHSCROLL, x_pos_bbox, 66, rc_tab.right * 0.14, cyChar + 4, import_data->tab_wrapper, (HMENU)ids_bbox[i], (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
        SendMessage(import_data->generic_settings_hwnds.bbox[i], WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
        ShowWindow(import_data->generic_settings_hwnds.bbox[i], 1);
        x_pos_bbox += rc_tab.right * 0.156;
    }
    import_data->generic_settings_hwnds.meshdbx_file_editctrl = CreateWindowExA(WS_EX_TRANSPARENT, WC_EDITA, 0, WS_CHILD | WS_BORDER | WS_VISIBLE | ES_WANTRETURN | ES_AUTOHSCROLL, 108, 94, rc_tab.right * 0.7, cyChar + 6, import_data->tab_wrapper, (HMENU)ID_ASSETC_IMPORT_DATA_MESHDBX_EDIT, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
    SendMessage(import_data->generic_settings_hwnds.meshdbx_file_editctrl, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    ShowWindow(import_data->generic_settings_hwnds.meshdbx_file_editctrl, 1);
    import_data->generic_settings_hwnds.lods_file_editctrl = CreateWindowExA(WS_EX_TRANSPARENT, WC_EDITA, 0, WS_CHILD | WS_BORDER | WS_VISIBLE | ES_WANTRETURN | ES_AUTOHSCROLL, 108, 120, rc_tab.right * 0.7, cyChar + 6, import_data->tab_wrapper, (HMENU)ID_ASSETC_IMPORT_DATA_LODSFILE_EDIT, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
    SendMessage(import_data->generic_settings_hwnds.lods_file_editctrl, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    ShowWindow(import_data->generic_settings_hwnds.lods_file_editctrl, 1);
    for (int i = 0; i < N_LODS_COMBO; i++)
    {
        ComboBox_AddString(import_data->generic_settings_hwnds.nlods_combo, lods_text[i]);
    }
    OLD_WRAPPER_PROC = (WNDPROC)SetWindowLongPtrW(import_data->generic_settings_hwnds.wrapper, GWLP_WNDPROC, (LONG_PTR)assetc_import_generic_data_wndproc);
    return;
}

void init_import_lod_sections(HWND hwndtab, import_settings* import_data,int i,int cxChar,int cyChar,bool move_start_btn)
{
    RECT rc_tab,rc_offset;
    HDC hdc = GetDC(hwndtab);
    GetClientRect(hwndtab, &rc_tab);
    GetClientRect(import_data->generic_settings_hwnds.wrapper, &rc_offset);
    wchar_t guid[33];
    int x = (rc_offset.right - (rc_offset.right * 0.95)) / 2;
    int y = (x + (rc_tab.right * 0.132));
    int offset = (x + (rc_offset.right * 0.14));
    int wrapper_pos_y = rc_offset.bottom + 24 +(200*i);
    const char* text[4] = {"VX Buffer 1","VX Buffer 2 (ZOnly)","Render Order","UV Channel for Tangents"};
    const char* lods_text[4] = {"Lod 0", "Lod 1", "Lod 2", "Lod 3"};
    import_data->lods_settings_hwnds[i].wrapper = CreateWindowExA(WS_EX_WINDOWEDGE, WC_BUTTONA, lods_text[i], WS_CHILD | BS_GROUPBOX, 0, rc_offset.bottom + (200*i), rc_tab.right * 0.95, 200, import_data->tab_wrapper, NULL, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
    int y_pos = 12 + wrapper_pos_y + 16;
    for (int j = 0; j < 4; j++)
    {
        import_data->lods_settings_hwnds[i].text_hwnds[j] = CreateWindowExA(WS_EX_WINDOWEDGE, WC_STATICA, text[j], WS_CHILD | ES_READONLY, 8, y_pos, 140, 18, import_data->tab_wrapper, NULL, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
        SendMessage(import_data->lods_settings_hwnds[i].text_hwnds[j], WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
        ShowWindow(import_data->lods_settings_hwnds[i].text_hwnds[j], 1);
        y_pos += 28;
    }
    import_data->lods_settings_hwnds[i].fbx_file_name = CreateWindowExA(WS_EX_WINDOWEDGE, WC_EDITA, 0, WS_CHILD | ES_READONLY, 8, wrapper_pos_y, cxChar * 50, cyChar + 4, import_data->tab_wrapper, NULL, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
    import_data->lods_settings_hwnds[i].fbx_file_open_btn = CreateWindowExA(WS_EX_WINDOWEDGE, WC_BUTTONA, "Open FBX", WS_CHILD | BS_CENTER, (cxChar * 50) + 20, wrapper_pos_y-1, cxChar * 10, cyChar + 5, import_data->tab_wrapper, (HMENU)ID_ASSETC_OPEN_FBX, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
    import_data->lods_settings_hwnds[i].vbuffer1 = CreateWindowExA(WS_EX_WINDOWEDGE, WC_COMBOBOXA, 0, WS_CHILD | CBS_DROPDOWN | CBS_HASSTRINGS | WS_VSCROLL, x + (cxChar * 21), wrapper_pos_y +26, cxChar * 40, cyChar + 50, import_data->tab_wrapper, (HMENU)ID_ASSETC_IMPORT_DATA_VX1, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
    import_data->lods_settings_hwnds[i].vbuffer2 = CreateWindowExA(WS_EX_WINDOWEDGE, WC_COMBOBOXA, 0, WS_CHILD | CBS_DROPDOWN | CBS_HASSTRINGS | WS_VSCROLL, x + (cxChar * 21), wrapper_pos_y +52, cxChar * 40, cyChar + 50, import_data->tab_wrapper, (HMENU)ID_ASSETC_IMPORT_DATA_VX2, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
    int ids_ro[4] = {ID_ASSETC_IMPORT_DATA_R01,ID_ASSETC_IMPORT_DATA_R02,ID_ASSETC_IMPORT_DATA_R03,ID_ASSETC_IMPORT_DATA_R04};
    int x_pos_bbox = 100;
    for (int j = 0; j < 4; j++)
    {
        import_data->lods_settings_hwnds[i].ro_vals[j] = CreateWindowExA(WS_EX_TRANSPARENT, WC_EDITA, 0, WS_CHILD | WS_BORDER | ES_WANTRETURN | ES_AUTOHSCROLL, x_pos_bbox, wrapper_pos_y+82, 75, cyChar + 4, import_data->tab_wrapper, (HMENU)ids_ro[j], (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
        SendMessage(import_data->lods_settings_hwnds[i].ro_vals[j], WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
        ShowWindow(import_data->lods_settings_hwnds[i].ro_vals[j], 1);
        x_pos_bbox += rc_tab.right * 0.18;
    }
    import_data->lods_settings_hwnds[i].tg_select_combo = CreateWindowExA(WS_EX_WINDOWEDGE, WC_COMBOBOXA, 0, WS_CHILD | CBS_DROPDOWN | CBS_HASSTRINGS | WS_VSCROLL, x + (cxChar * 21), wrapper_pos_y + 110, cxChar * 8, cyChar + 50, import_data->tab_wrapper, (HMENU)ID_ASSETC_IMPORT_DATA_TG_SEL, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
    import_data->lods_settings_hwnds[i].enable_billboard = CreateWindowExA(WS_EX_WINDOWEDGE, WC_BUTTONA, "Use Billboard", WS_CHILD | BS_AUTOCHECKBOX | BS_PUSHBUTTON, x + (cxChar * 21)+120, wrapper_pos_y + 112, (cxChar * 20), (cyChar + 4), import_data->tab_wrapper, (HMENU)ID_ASSETC_IMPORT_DATA_BBOARD_ENABLE, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
    import_data->lods_settings_hwnds[i].billboard_text = CreateWindowExA(WS_EX_WINDOWEDGE, WC_EDITA, 0, WS_CHILD | WS_BORDER | WS_VISIBLE | ES_WANTRETURN | WS_DISABLED | ES_AUTOHSCROLL, 8, wrapper_pos_y+140, rc_tab.right * 0.9, cyChar + 6, import_data->tab_wrapper, (HMENU)ID_ASSETC_IMPORT_DATA_BILLBOARD, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
    SendMessage(import_data->lods_settings_hwnds[i].fbx_file_name, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    ShowWindow(import_data->lods_settings_hwnds[i].fbx_file_name, 1);
    SendMessage(import_data->lods_settings_hwnds[i].fbx_file_open_btn, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    ShowWindow(import_data->lods_settings_hwnds[i].fbx_file_open_btn, 1);
    SendMessage(import_data->lods_settings_hwnds[i].vbuffer1, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    ShowWindow(import_data->lods_settings_hwnds[i].vbuffer1, 1);
    SendMessage(import_data->lods_settings_hwnds[i].vbuffer2, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    ShowWindow(import_data->lods_settings_hwnds[i].vbuffer2, 1);
    SendMessage(import_data->lods_settings_hwnds[i].tg_select_combo, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    ShowWindow(import_data->lods_settings_hwnds[i].tg_select_combo, 1);
    SendMessage(import_data->lods_settings_hwnds[i].enable_billboard, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    ShowWindow(import_data->lods_settings_hwnds[i].enable_billboard, 1);
    SendMessage(import_data->lods_settings_hwnds[i].billboard_text, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    ShowWindow(import_data->lods_settings_hwnds[i].billboard_text, 1);
    for (uint32_t k = 0; k < assetc_n_vx_buffers; k++)
    {
        hex_to_wchar(guid, assetc_vx_buffers[k].vxbuffer_id, 16);
        ComboBox_AddString(import_data->lods_settings_hwnds[i].vbuffer1, guid);
        ComboBox_AddString(import_data->lods_settings_hwnds[i].vbuffer2, guid);
    }
    SendMessage(import_data->lods_settings_hwnds[i].wrapper, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    ShowWindow(import_data->lods_settings_hwnds[i].wrapper, 1);
    (WNDPROC)SetWindowLongPtrW(import_data->tab_wrapper, GWLP_WNDPROC, (LONG_PTR)assetc_import_lod_wndproc);
    if ((i == (import_data->n_lods - 1)) && (!move_start_btn))
    {
        import_data->start_button = CreateWindowExA(WS_EX_WINDOWEDGE, WC_BUTTONA, "GENERATE", WS_CHILD | BS_CENTER, rc_tab.right/2-60, wrapper_pos_y+195, 90, 30, import_data->tab_wrapper, (HMENU)ID_ASSETC_IMPORT_START, (HINSTANCE)GetWindowLongPtr(hwndtab, GWLP_HINSTANCE), NULL);
        ShowWindow(import_data->start_button, 1);
    }
    else if (move_start_btn)
    {
        ShowWindow(import_data->start_button, 0);
        MoveWindow(import_data->start_button, rc_tab.right / 2 - 60, wrapper_pos_y + 195, 90, 30, true);
        ShowWindow(import_data->start_button, 1);
    }
    return;
}

TCITEMA** init_import_tabs(HWND hwndtab,import_settings* import_data)
{
    TCITEMA* tabs[N_TABS];
    static const wchar_t* tab_titles[5] = { L"Rigid",L"Composite",L"Tree",L"Skinned 1",L"Skinned 2" };
    for (int i = 0; i < N_TABS; i++)
    {
        tabs[i] = (TCITEMA*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
        ac_structs_pool->current_pos += sizeof(TCITEMA);
        tabs[i]->mask = TCIF_TEXT | TCIF_PARAM;
        tabs[i]->pszText = (LPSTR)tab_titles[i];
        tabs[i]->lParam = (LPARAM)&import_data[i];  
        TabCtrl_InsertItem(hwndtab, i, tabs[i]);
    }
    return tabs;
}

HWND init_import_tab(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    HWND hwndresult = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        WC_TABCONTROLA,
        "TITLE",
        WS_CHILD | WS_EX_RIGHTSCROLLBAR| WS_CLIPSIBLINGS,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top+400,
        hwnd, (HMENU)ID_ASSETC_IMPORT_MESH_TAB, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
    if (hwndresult == NULL)
    {
        MessageBoxA(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    return hwndresult;
}

void destroy_lod_sec(import_settings* import_data, int j)
{
    DestroyWindow(import_data[active_tab].lods_settings_hwnds[j].wrapper);
    DestroyWindow(import_data[active_tab].lods_settings_hwnds[j].tg_select_combo);
    DestroyWindow(import_data[active_tab].lods_settings_hwnds[j].fbx_file_open_btn);
    DestroyWindow(import_data[active_tab].lods_settings_hwnds[j].fbx_file_name);
    DestroyWindow(import_data[active_tab].lods_settings_hwnds[j].vbuffer1);
    DestroyWindow(import_data[active_tab].lods_settings_hwnds[j].vbuffer2);
    DestroyWindow(import_data[active_tab].lods_settings_hwnds[j].enable_billboard);
    DestroyWindow(import_data[active_tab].lods_settings_hwnds[j].billboard_text);
    for (int k = 0; k < 5; k++)
    {
        DestroyWindow(import_data[active_tab].lods_settings_hwnds[j].text_hwnds[k]);
    }
    for (int k = 0; k < 4; k++)
    {
        DestroyWindow(import_data[active_tab].lods_settings_hwnds[j].ro_vals[k]);
    }
    return;
}
