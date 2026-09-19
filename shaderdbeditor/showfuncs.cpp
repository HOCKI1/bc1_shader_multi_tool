#define _CRT_SECURE_NO_WARNINGS
#include "showfuncs.h"
#include "funcs.h"
#include <sstream>
#include "Resource.h"

// OLD CODE. MOST OF THESE FUNCTIONS ARE NOT USED ANYMORE


// VS and PS shader titles
const char* id = "ID:";
const char* txeid = "Texture List ref:";
const char* d3dp1id = "D3D Params ref 1:";
const char* d3dp2id = "D3D Params ref 2:";
const char* vs_strings[4] = { id,txeid,d3dp1id,d3dp2id };

typedef struct  
{ 
    char text[90]; 
} data_text;

typedef struct
{
    char text[5];
} data_text_small;

typedef struct
{
    uint16_t* internal_type;
    char* text_type;
    uint16_t mesh_tool_type;
} vxbuffer_text;


void hex_to_string(char* id,char* src, int size) // USE THIS FOR ID STRINGS (STRNCPY DOESNT WORK BECAUSE OF NULL BYTES)
{
    int pos;
    for (int i = 0; i < size; i++)
    {
        pos = snprintf(id + (i * 2), 3, "%02X", 0xFF & src[i]);
    }
    id[(size*2)] = '\0';
    return;
}

int get_index(vxbuffer_text* vx_text,uint16_t type)
{
    int i = 0;
    while (*vx_text[i].internal_type != type) 
    {
        i++;
    }
    return i;
}


void show_shdr(HWND hwnd, shader_struct* shader, int cxChar, int cyChar, RECT rcTree, generic_hwnds* &hwnd_container)
{
    HDC hdc = GetDC(hwnd);
    char id[33];
    char number[5];
    hex_to_string(id, shader->shader_ref->guid, 16);
    uint32_t intdummy = (uint32_t)shader->tables_ref->n_table_refs;
    snprintf(number, 5, "%d", intdummy);
    create_wnd_and_txt(hwnd, hdc, rcTree, cxChar, cyChar, 0, hwnd_container, "ID: ", id);
    create_wnd_and_txt(hwnd, hdc, rcTree, cxChar, cyChar, 1, hwnd_container, "Number of Table References:",  number);
    ReleaseDC(hwnd, hdc);
    hwnd_container->first_draw = FALSE;
    hwnd_container->destroy = TRUE;
    return;
}

void show_str(HWND hwnd,string_ref &strings, int index_i, int cxChar, int cyChar, RECT rcTree, generic_hwnds* &hwnd_container)
{
    bool result;
    HDC hdc = GetDC(hwnd);
    char id[33];
    hex_to_string(id, strings.guid, 16);
    if (index_i > 1)
    {   
        create_wnd_and_txt(hwnd, hdc, rcTree, cxChar, cyChar, 0, hwnd_container, "Vertex Buffer: ", id);
    }
    else
    {
        create_wnd_and_txt(hwnd, hdc, rcTree, cxChar, cyChar, 0, hwnd_container, "ID:", id);
    }
    ReleaseDC(hwnd, hdc);
    hwnd_container->first_draw = FALSE;
    hwnd_container->destroy = TRUE;
    return;
}



void show_vbuffer(HWND hwnd, vxbuffer &vx_buffers, int cxChar, int cyChar, RECT rcTree, generic_hwnds*& hwnd_container)
{
    bool result;
    static bool initialize = TRUE;
    static const char* vx_titles[22] = {"ID: ", "Type: ", "Stride Size:", "Number of Data types: (Internal || Mesh Tool Type)", "Vertex Coordinates", "Vertex Coordinates (FP32)", "Normals", "Tangents","Component"
    ,"UV Coordinates", "Bone Index // Shape Index", "Bone Weights","Vertex Coordinates + Component", "Null vector", "Unknown Tree Mesh Property", "SootPerVertex_Half","DecalGenerate2d"
    ,"Environment", "Unknown", "VTexShared","Generate2d","Decal"};
    static uint16_t internal_type[18] = {263,259,1032,1288,1797,6,524,780,264,8,4,12,21,3,1544,15,2,1};
    vxbuffer_text* vx_text = new vxbuffer_text[18];
    char id[33];
    HDC hdc = GetDC(hwnd);
    hex_to_string(&id[0], &vx_buffers.vxbuffer_id[0], 16);
    if (initialize) 
    {
        for (uint16_t i = 0; i < 18; i++)
        {
            vx_text[i].internal_type = &internal_type[i];
            vx_text[i].mesh_tool_type = (i+1);
            vx_text[i].text_type = const_cast<char*>(vx_titles[(i + 4)]);
        }
    }
    if (*vx_buffers.type != 4294967040) 
    {
        char data[147];
        hex_to_string(data, vx_buffers.unknown_data, 73);
        create_wnd_and_txt(hwnd, hdc, rcTree, cxChar, cyChar, 0, hwnd_container, vx_titles[0], id);
        if (*vx_buffers.type == 4294902016) 
        {
            create_wnd_and_txt(hwnd, hdc, rcTree, cxChar, cyChar, 1, hwnd_container, vx_titles[1], "Stream Instancing");
        }          
        else
        {
            create_wnd_and_txt(hwnd, hdc, rcTree, cxChar, cyChar, 1, hwnd_container, vx_titles[1], "Other");
        }
        create_wnd_and_txt(hwnd, hdc, rcTree, cxChar, cyChar, 2, hwnd_container, "Data:", data);
    }
    else
    {
        int type_index;
        char final_text[60];
        char* final_txt_ptr = &final_text[0];
        char type_text[9] = "Standard";
        char wnd_2[5];
        char wnd_3[5];
        char* three_windows[4] = {id,type_text, wnd_2, wnd_3};
        char splitter[5] = " || ";
        uint8_t* dummyptr = vx_buffers.n_data_types;
        uint32_t intdummy = (uint32_t)*dummyptr;
        snprintf(three_windows[3], 5, "%d", intdummy);
        dummyptr = vx_buffers.vx_stride;
        intdummy = (uint32_t)*dummyptr;
        snprintf(three_windows[2], 5, "%d", intdummy);
        for (int i = 0; i < 4; i++)
        { 
            create_wnd_and_txt(hwnd, hdc, rcTree, cxChar, cyChar, i, hwnd_container, vx_titles[i], three_windows[i]);
        }
        uint32_t* intcast;
        int str_length;
      //  data_text_small* vx_types_txt_array = new data_text_small[(uint32_t)*dbptr->vx_buffers[index].n_data_types];
        for (int i = 4; i < hwnd_container->w_hwnds_size; i++)
        {
          //  intcast = (uint32_t*)dbptr->vx_buffers[index].types_array[(i - 3)];
            hex_to_string(final_txt_ptr,(char*)vx_buffers.types_array[(i - 4)],2);
           // snprintf(&final_txt_ptr[0], 5, "%d", *intcast);
            type_index = get_index(vx_text, *vx_buffers.types_array[(i - 4)]);
            str_length = strlen(final_txt_ptr);
            strncpy(&final_txt_ptr[str_length], &splitter[0], 5);
            str_length = strlen(final_txt_ptr);
            snprintf(&final_txt_ptr[str_length], 5, "%d", vx_text[type_index].mesh_tool_type);
            create_wnd_and_txt(hwnd, hdc, rcTree, cxChar, cyChar, i, hwnd_container, vx_text[type_index].text_type, final_txt_ptr);
            for (int j = 0; j < 60; j++)
            {
                final_txt_ptr[j] = 0;
            }
        }
    }
    hwnd_container->first_draw = FALSE;
    hwnd_container->destroy = TRUE;
    ReleaseDC(hwnd, hdc);
    return;
}


void show_table(HWND hwnd, table_parent &table, int cxChar, int cyChar, RECT rcTree, generic_hwnds*& hwnd_container)
{
    bool result;
    HDC hdc = GetDC(hwnd);
    static const char* table_titles[13] = { "ID: ", "Unknown: ", "Unknown: ", "Hash: ", "NULL","Vertex Shader Number: ", "Pixel Shader Number: ",
                                    "Empty Texture List: ", "Texture List Number: ", "Shader Container ID: ", "Parent Node ID: ", "Vertex Buffer ID: " , "Unknown Data: "};
    data_text* data = new data_text[13];
    uint32_t* dummyptr = table.unknown1;
    char* hex_strings[4] = { table.child->shader_id, table.child->system_id, table.child->vbuffer_id, table.child->unknown_data };
   
    hex_to_string(data[0].text, table.element_id, 16);
    for (int i = 0; i < 8; i++)
    {
        snprintf(data[(i+1)].text, 7, "%d", *dummyptr);
        dummyptr += 1;
    }
    for (int i = 0; i < 3; i++)
    {
        hex_to_string(data[(i + 9)].text, hex_strings[i], 16);
    }
    hex_to_string(data[12].text, hex_strings[3], 40);
    for (uint32_t i = 0; i < hwnd_container->w_hwnds_size; i++)
    {
        create_wnd_and_txt(hwnd, hdc, rcTree, cxChar, cyChar, i, hwnd_container, table_titles[i], data[i].text);
    }
    hwnd_container->destroy = TRUE;
    delete[] data;
    ReleaseDC(hwnd, hdc);
    return;
}

void show_ps(HWND hwnd, bc2_ps &ps_shader, int cxChar, int cyChar, RECT rcTree, generic_hwnds*& hwnd_container)
{
    bool result;
    HDC hdc = GetDC(hwnd);
    char* shdr_id = ps_shader.psid;
    char id[33];
    hex_to_string(id, shdr_id,16);
    char local1[5];
    char local2[5];
    char local3[5];
    char* data[4] = { id,local1,local2,local3 };
    uint32_t dummy1 = *ps_shader.txeindex;
    uint32_t dummy2 = *ps_shader.d3d1p;
    uint32_t dummy3 = *ps_shader.d3d2p;
    uint32_t arr[3] = { dummy1,dummy2,dummy3 };

	for (uint32_t i = 1; i < 4; i++) {
		snprintf(data[i], 5, "%d", arr[i - 1]);
	}
	for (uint32_t i = 0; i < hwnd_container->w_hwnds_size; i++)
	{
        create_wnd_and_txt(hwnd, hdc, rcTree, cxChar, cyChar, i, hwnd_container, vs_strings[i], data[i]);   
    }
	hwnd_container->first_draw = FALSE;
	hwnd_container->destroy = TRUE;
    ReleaseDC(hwnd, hdc);
    return;
}


void show_vs(HWND hwnd,bc2_vs &vs_shader, int cxChar,int cyChar, RECT rcTree, generic_hwnds*& hwnd_container) 
{
    bool result;
    HDC hdc = GetDC(hwnd);
    char* shdr_id = vs_shader.vsid;
    char id[33];
    hex_to_string(id, shdr_id,16);
    char local1[5];
    char local2[5];
    char local3[5];
    char* data[4] = { id,local1,local2,local3 };
    uint32_t dummy1 = *vs_shader.txeindex;
    uint32_t dummy2 = *vs_shader.d3d1p;
    uint32_t dummy3 = *vs_shader.d3d2p;
    uint32_t arr[3] = {dummy1,dummy2,dummy3};

	for (int i = 1; i < 4; i++) {
		snprintf(data[i], 5, "%d", arr[i - 1]);
	}
	for (uint32_t i = 0; i < hwnd_container->w_hwnds_size; i++)
	{
		/*TextOutA(hdc, rcTree.right - (rcTree.left - 23), 24 + ((cyChar + 14) * (i * 2)), (LPCSTR)&vs_strings[i][0], strlen(vs_strings[i]));
		result = windowcreator_new(hwnd, hwnd_container->w_hwnds[i], (rcTree.right - (rcTree.left - 20)), (20 + ((cyChar + 14) * (i * 2 + 1))), (cxChar) * 90, cyChar + 8);
		if (!SetWindowTextA(hwnd_container->w_hwnds[i], data[i]))
		{
			MessageBoxA(hwnd, "SHOW TXE ERROR", "MessageBox", MB_OK);
		}*/
        create_wnd_and_txt(hwnd, hdc, rcTree, cxChar, cyChar, i, hwnd_container, vs_strings[i], data[i]);
    }
	hwnd_container->first_draw = FALSE;
	hwnd_container->destroy = TRUE;
	ReleaseDC(hwnd, hdc);
    return; 
}


void show_txlist(HWND hwnd, TxE &txeptr, int cxChar, int cyChar, RECT rc, RECT rcTree, generic_hwnds*& hwnd_container)
{
    bool result;
    const char* out1 = "Textures: ";
    const char* out2 = "Parameters: ";
    const char* out3 = "Types: ";
    HDC hdc = GetDC(hwnd);
    int acc = 0;
    char* text[32]; // a big enough array
    const char* titles[3] = { out1,out2,out3 };
    bool draw_title[3] = { FALSE,FALSE,FALSE };
    uint32_t loop_control[3] = { *txeptr.ntxt, *txeptr.nparams , *txeptr.ntypes };

    for (int i = 0; i < loop_control[0]; i++) {
        text[i] = txeptr.txtptrs[i].name;
        acc++;
        draw_title[0] = TRUE;
    }
    for (int i = 0; i < loop_control[1]; i++) {
        text[(acc)] = txeptr.paramptrs[i].name;
        acc++;
        draw_title[1] = TRUE;
    }
    for (int i = 0; i < loop_control[2]; i++) {
        text[(acc)] = txeptr.typeptrs[i].name;
        acc++;
        draw_title[2] = TRUE;
    }
    int hwnd_index = 0;
    int w_pos = 0;
    for (uint32_t i = 0; i < 3; i++)
    {
        if (draw_title[i]) {
            TextOutA(hdc, rcTree.right - (rcTree.left - 24), 24 + ((cyChar + 14) * w_pos), (LPCSTR)titles[i], strlen(titles[i]));
            w_pos++;
        }
        for (int j = 0; j < loop_control[i]; j++)
        {
            result = windowcreator_new(hwnd, hwnd_container->w_hwnds[hwnd_index], (rcTree.right - (rcTree.left - 20)), (20 + ((cyChar + 14) * w_pos)), (cxChar) * 90, cyChar + 8);
            if (!SetWindowTextA(hwnd_container->w_hwnds[hwnd_index], text[hwnd_index]))
            {
                MessageBoxA(hwnd, "SHOW PS ERROR", "MessageBox", MB_OK);
            }
            hwnd_index++;
            w_pos++;
        }
    }
    hwnd_container->first_draw = FALSE;
    hwnd_container->destroy = TRUE;
    ReleaseDC(hwnd, hdc);
    return;
}




