#define _CRT_SECURE_NO_WARNINGS
#include "show_lv_funcs.h"


// VS and PS shader titles
const wchar_t* id = L"ID:";
const wchar_t* txeid = L"Texture List ref:";
const wchar_t* d3dp1id = L"D3D Params ref 1:";
const wchar_t* d3dp2id = L"D3D Params ref 2:";
const wchar_t* vs_strings[4] = { id,txeid,d3dp1id,d3dp2id };

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
    wchar_t* text_type;
    uint16_t mesh_tool_type;
} vxbuffer_textw;


void hex_to_wchar(wchar_t* dst, char* src, int size) // USE THIS FOR ID STRINGS (STRNCPY DOESNT WORK BECAUSE OF NULL BYTES)
{
    int pos;
    char* step1 = new char[(size * 2) + 1];
    for (int i = 0; i < size; i++)
    {
        pos = snprintf(step1 + (i * 2), 3, "%02X", 0xFF & src[i]);
    }
    step1[(size * 2)] = '\0';
    mbstowcs(dst,step1,(size+1)*sizeof(wchar_t));
    delete[] step1;
    return;
}

void set_item(int mask, int iItem, int iSubItem, const wchar_t* pszText, LVITEM &dst_item, lv_item_data* lparam)
{
    dst_item.mask = mask;
    dst_item.iItem = iItem;
    dst_item.iSubItem = iSubItem;
    dst_item.pszText = (LPWSTR)pszText;
    dst_item.lParam = (LPARAM)lparam;
    return;
}

void load_vx_vals(int array_size, uint32_t* &int_array, vxbuffer& vx_buffer)
{
    int_array[0] = *vx_buffer.vx_stride;
    int_array[1] = *vx_buffer.n_data_types;
    for (int i = 2; i < array_size; i++)
    {
        int_array[i] = *vx_buffer.types_array[i - 2];  
    }
    return;
}

int get_indexw(vxbuffer_textw* vx_text, uint16_t type)
{
    int i = 0;
    while (*vx_text[i].internal_type != type)
    {
        i++;
    }
    return i;
}


void show_shdr_lv(HWND hwnd, HWND hwndList, shader_struct* shader, int cxChar, int cyChar, RECT rcTree)
{
    wchar_t id[33];
    wchar_t number[12];
    wchar_t name[350];
    mbstowcs(name, shader->shader_ref->name, 350);
    hex_to_wchar(id, shader->shader_ref->guid, 16);
    uint32_t intdummy = (uint32_t)shader->tables_ref->n_table_refs;
    swprintf_s(&number[0], 11, L"%u", intdummy);

    if (shader->initialize_items)
    {
        shader->lv_items = new LVITEM[3];
        shader->lv_data = new lv_item_data[3];
    }
    if (!ListView_DeleteAllItems(hwndList)) {
        MessageBox(NULL, L"List View DELETE ALL ITEMS ERROR", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
    }
    for (int i = 0; i < 3; i++)
    {
        shader->lv_data[i].type = ID_SHADER_NODE;
        shader->lv_data[i].struct_ptr = shader;
        if (i == 1)
        {
            shader->lv_data[i].subtype = TYPE_GUID;
            shader->lv_data[i].edit_enable = TRUE;
            shader->lv_data[i].block_data = &shader->shader_ref->guid;
        }
        else if (i==2)
        {
            shader->lv_data[i].subtype = TYPE_UINT32;
            shader->lv_data[i].edit_enable = FALSE;
            shader->lv_data[i].block_data = &shader->tables_ref->n_table_refs;
        }
        else
        {
            shader->lv_data[i].subtype = TYPE_TEXT;
            shader->lv_data[i].edit_enable = TRUE;
            shader->lv_data[i].block_data = &shader->shader_ref->name;
        }
    }
    set_item(LVIF_TEXT | LVIF_PARAM, 0, 0, L"Name:", shader->lv_items[0], &shader->lv_data[0]);
    set_item(LVIF_TEXT | LVIF_PARAM, 1, 0, L"ID:", shader->lv_items[1], &shader->lv_data[1]);
    set_item(LVIF_TEXT | LVIF_PARAM, 2, 0, L"Table References:", shader->lv_items[2], &shader->lv_data[2]);
    ListView_InsertItem(hwndList,&shader->lv_items[0]);
    ListView_InsertItem(hwndList, &shader->lv_items[1]);
    ListView_InsertItem(hwndList, &shader->lv_items[2]);
    ListView_SetItemText(hwndList, 0, 1, &name[0]);
    ListView_SetItemText(hwndList, 1, 1, &id[0]);
    ListView_SetItemText(hwndList, 2, 1, &number[0]);
    shader->initialize_items = FALSE;
    return;
}


void show_str_lv(HWND hwnd, HWND hwndList, string_ref& strings, int index_i, int cxChar, int cyChar, RECT rcTree)
{
    bool result;
    wchar_t id[33];
    wchar_t name[350];
    hex_to_wchar(id, strings.guid, 16);
    mbstowcs(name, strings.name, 350);
    if (strings.initialize_items)
    {
        strings.lv_items = new LVITEM[2];
        strings.lv_data = new lv_item_data[2];
    }
    if (!ListView_DeleteAllItems(hwndList)) {
        MessageBox(NULL, L"List View DELETE ALL ITEMS ERROR", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
    }

    if (index_i > 1)
    {
        set_item(LVIF_TEXT | LVIF_PARAM, 0, 0, L"Name: ", strings.lv_items[0], &strings.lv_data[0]);
        set_item(LVIF_TEXT | LVIF_PARAM,1,0,L"Vertex Buffer: ", strings.lv_items[1], &strings.lv_data[1]);
        strings.lv_data[0].edit_enable = TRUE;
        strings.lv_data[0].block_data = &strings.name;
        strings.lv_data[0].struct_ptr = &strings;
        strings.lv_data[0].type = ID_STR_MESH_REF;
        strings.lv_data[0].subtype = TYPE_TEXT;
        strings.lv_data[1].edit_enable = TRUE;
        strings.lv_data[1].block_data = &strings.guid;
        strings.lv_data[1].struct_ptr = &strings;
        strings.lv_data[1].type = ID_STR_MESH_REF;
        strings.lv_data[1].subtype = TYPE_GUID;
        ListView_InsertItem(hwndList, &strings.lv_items[0]);
        ListView_InsertItem(hwndList, &strings.lv_items[1]);
        ListView_SetItemText(hwndList, 0, 1, &name[0]);
        ListView_SetItemText(hwndList, 1, 1, &id[0]);
    }
    else if (index_i == 1)
    {
        set_item(LVIF_TEXT | LVIF_PARAM, 0, 0, L"ID: ", strings.lv_items[0], &strings.lv_data[0]);
        strings.lv_data[0].edit_enable = FALSE;
        strings.lv_data[0].block_data = &strings.guid;
        strings.lv_data[0].struct_ptr = &strings;
        strings.lv_data[0].type = ID_SYSTEMS_N_NODE;
        strings.lv_data[0].subtype = TYPE_GUID;
        ListView_InsertItem(hwndList, &strings.lv_items[0]);
        ListView_SetItemText(hwndList, 0, 1, &id[0]);
    }
    else if (index_i == 0)
    {
        set_item(LVIF_TEXT | LVIF_PARAM, 0, 0, L"ID: ", strings.lv_items[0], &strings.lv_data[0]);
        strings.lv_data[0].edit_enable = FALSE;
        strings.lv_data[0].block_data = &strings.guid;
        strings.lv_data[0].struct_ptr = &strings;
        strings.lv_data[0].type = ID_STR_SHDR_REF;
        strings.lv_data[0].subtype = TYPE_GUID;
        ListView_InsertItem(hwndList, &strings.lv_items[0]);
        ListView_SetItemText(hwndList, 0, 1, &id[0]);
    }
    strings.initialize_items = FALSE;
    return;
}




void show_vxbfr_lv(HWND hwnd, HWND hwndList, vxbuffer& vx_buffer, int cxChar, int cyChar, RECT rc, RECT rcTree)
{
    static bool initialize = TRUE;
    static const wchar_t* vx_titles[23] = { L"ID: ", L"Type: ", L"Stride Size:", L"Number of Data types: ", L"Vertex Coordinates", L"Vertex Coordinates (FP32)", L"Normals", L"Tangents",L"Component"
,L"UV Coordinates", L"Bone Index // Shape Index", L"Bone Weights",L"Vertex Coordinates + Component", L"Null vector", L"Unknown Tree Mesh Property", L"Unknown Property",L"SootPerVertex_Half",
L"DecalGenerate2d",L"Environment", L"Unknown", L"VTexShared",L"Generate2d",L"Decal" };
    static uint16_t internal_type[18] = { 263,259,1032,1288,1797,6,524,780,264,8,4,12,21,3,1544,15,2,1 };
    static vxbuffer_textw* vx_text = new vxbuffer_textw[18];
    uint32_t* ints = new uint32_t[*vx_buffer.n_data_types+2];


    if (vx_buffer.initialize_items)
    {
        vx_buffer.lv_items = new LVITEM[vx_buffer.n_wnds];
        vx_buffer.lv_data = new lv_item_data[vx_buffer.n_wnds];
    }
    if (initialize)
    {
        for (uint16_t i = 0; i < 18; i++)
        {
            vx_text[i].internal_type = &internal_type[i];
            vx_text[i].mesh_tool_type = (i + 1);
            vx_text[i].text_type = const_cast<wchar_t*>(vx_titles[(i + 4)]);
        }
    }
    if (!ListView_DeleteAllItems(hwndList)) {
        MessageBox(NULL, L"List View DELETE ALL ITEMS ERROR", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
    }
    if (*vx_buffer.type != 4294967040)
    {
        vx_buffer.lv_data[0].edit_enable = FALSE;
        vx_buffer.lv_data[1].edit_enable = FALSE;
        vx_buffer.lv_data[2].edit_enable = FALSE;
        wchar_t data[200];
        hex_to_wchar(data,vx_buffer.unknown_data,73);
        hex_to_wchar(&data[147], vx_buffer.vxbuffer_id, 16);
        set_item(LVIF_TEXT | LVIF_PARAM,0,0,L"ID",vx_buffer.lv_items[0],&vx_buffer.lv_data[0]);
        ListView_InsertItem(hwndList, &vx_buffer.lv_items[0]);
        ListView_SetItemText(hwndList, 0, 1, &data[147]);
        if (*vx_buffer.type == 4294902016)
        {
            vx_buffer.lv_data[1].type = ID_VX_STREAM;
            set_item(LVIF_TEXT | LVIF_PARAM, 1, 0, L"Type", vx_buffer.lv_items[1],&vx_buffer.lv_data[1]);
            ListView_InsertItem(hwndList, &vx_buffer.lv_items[1]);
            ListView_SetItemText(hwndList, 1, 1, (LPWSTR)L"Stream Instancing");
        }
        else
        {
            vx_buffer.lv_data[1].type = ID_VX_OTHER;
            set_item(LVIF_TEXT | LVIF_PARAM, 1, 0, L"Type", vx_buffer.lv_items[1],&vx_buffer.lv_data[1]);
            ListView_InsertItem(hwndList, &vx_buffer.lv_items[1]);
            ListView_SetItemText(hwndList, 1, 1, (LPWSTR)L"Other");
        }
        set_item(LVIF_TEXT | LVIF_PARAM, 2, 0, L"Data", vx_buffer.lv_items[2],&vx_buffer.lv_data[1]);
        ListView_InsertItem(hwndList, &vx_buffer.lv_items[2]);
        ListView_SetItemText(hwndList, 2, 1, &data[0]);
   
    }
    else
    {
        load_vx_vals(*vx_buffer.n_data_types + 2, ints,vx_buffer);
        wchar_t data[300];
        int nextpos = 0;
        int delay = 0;
        int idx = 0;
        for (int i = 0; i < vx_buffer.n_wnds; i++)
        {
            vx_buffer.lv_data[i].edit_enable = FALSE;
            vx_buffer.lv_data[i].type = ID_VX_STANDARD;
            if (i == 0)
            {
                set_item(LVIF_TEXT | LVIF_PARAM, i, 0, vx_titles[i], vx_buffer.lv_items[i],&vx_buffer.lv_data[i]);
                ListView_InsertItem(hwndList, &vx_buffer.lv_items[i]);
                hex_to_wchar(data, vx_buffer.vxbuffer_id, 16);
                ListView_SetItemText(hwndList,i, 1, &data[0]);       
                delay += 33;
            }
            else if (i == 1)
            {
                set_item(LVIF_TEXT | LVIF_PARAM, i, 0, L"Type", vx_buffer.lv_items[i], &vx_buffer.lv_data[i]);
                ListView_InsertItem(hwndList, &vx_buffer.lv_items[i]);
                ListView_SetItemText(hwndList, i, 1, (LPWSTR)L"Standard");
            }
            else if ((i > 1) && (i < 4))
            {
                set_item(LVIF_TEXT | LVIF_PARAM, i, 0, vx_titles[i], vx_buffer.lv_items[i], &vx_buffer.lv_data[i]);
                swprintf_s(&data[nextpos], 11, L"%u", ints[i-2]);
                ListView_InsertItem(hwndList, &vx_buffer.lv_items[i]);
                ListView_SetItemText(hwndList, i, 1, (LPWSTR)&data[nextpos]);
                delay += 11;
            }
            else 
            {
                idx = get_indexw(vx_text,ints[i-2]);
                set_item(LVIF_TEXT | LVIF_PARAM, i, 0, vx_text[idx].text_type, vx_buffer.lv_items[i], &vx_buffer.lv_data[i]);
                swprintf_s(&data[nextpos], 11, L"%u", vx_text[idx].mesh_tool_type);
                ListView_InsertItem(hwndList, &vx_buffer.lv_items[i]);
                ListView_SetItemText(hwndList, i, 1, &data[nextpos]);
                delay += 11;       
            }      
            nextpos = delay;
        }
    }
    delete[] ints;
    initialize = FALSE;
    vx_buffer.initialize_items = FALSE;
    return;
}


void show_table_lv(HWND hwnd, HWND hwndList, table_parent& table, int cxChar, int cyChar, RECT rc, RECT rcTree)
{
    static const wchar_t* table_titles[13] = { L"ID: ", L"Unknown: ", L"Unknown: ", L"Hash: ", L"NULL", L"Vertex Shader Number: ", L"Pixel Shader Number: ",
                                L"Empty Texture List: ", L"Texture List Number: ", L"Shader Container ID: ", L"Parent Node ID: ", L"Vertex Buffer ID: " , L"Unknown Data: " };
    uint32_t** ints[8] = { &table.unknown1,&table.unknown2,&table.temp_hash,&table.null_int,&table.vs_ref,&table.ps_ref,&table.txe_ref1,&table.txe_ref2 };
    char** text[5] = {&table.element_id,&table.child->shader_id,&table.child->system_id, &table.child->vbuffer_id, &table.child->unknown_data};
    
    if (table.initialize_items)
    {
        table.lv_items = new LVITEM[13];
        table.lv_data = new lv_item_data[13];
    }
    if (!ListView_DeleteAllItems(hwndList)) {
        MessageBox(NULL, L"List View DELETE ALL ITEMS ERROR", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
    }

    int str_index = 0;
    int int_index = 0;
    int nextpos = 0;
    int delay = 0;
    wchar_t temp[400];
    for (int i = 0; i < 13; i++)
    {
        table.lv_data[i].type = ID_TABLE;
        if ((i == 0) || (i == 11) || (i == 10) || (i == 9))
        {
            table.lv_data[i].subtype = TYPE_GUID;
            table.lv_data[i].block_data = text[str_index];
            hex_to_wchar(&temp[nextpos], *text[str_index], 16);
            delay += 34;
            str_index++;
        }
        else if (i == 12)
        {
            table.lv_data[i].subtype = TYPE_HEX;
            table.lv_data[i].block_data = text[str_index];
            hex_to_wchar(&temp[nextpos], *text[str_index], 40);
            delay += 82;
            str_index++;   
        }
        else 
        {
            table.lv_data[i].subtype = TYPE_UINT32;
            table.lv_data[i].block_data = ints[int_index];
            swprintf_s(&temp[nextpos], 11, L"%u", **ints[int_index]);
            delay += 11;
            int_index++;
        }

        table.lv_data[i].edit_enable = TRUE;
        table.lv_data[i].struct_ptr = &table;
        set_item(LVIF_TEXT|LVIF_PARAM,i,0,table_titles[i],table.lv_items[i],&table.lv_data[i]);
        ListView_InsertItem(hwndList, &table.lv_items[i]);
        ListView_SetItemText(hwndList, i, 1, &temp[nextpos]);
        nextpos = delay;
    }
    table.initialize_items = FALSE;
    return;
}



void show_ps_lv(HWND hwnd, HWND hwndList, bc2_ps& ps_shader, int cxChar, int cyChar, RECT rc, RECT rcTree)
{
    bool result;
    uint32_t** arr[3] = { &ps_shader.txeindex, &ps_shader.d3d1p, &ps_shader.d3d2p };

    if (ps_shader.initialize_items)
    {
        ps_shader.lv_items = new LVITEM[4];         // the items must be kept alive until saving
        ps_shader.lv_data = new lv_item_data[4];    // the items must be kept alive until saving
    }
    if (!ListView_DeleteAllItems(hwndList)) {
        MessageBox(NULL, L"List View DELETE ALL ITEMS ERROR", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
    }

    wchar_t temp[90];
    int nextpos = 0;
    int delay = 0;
    for (uint32_t i = 0; i < 4; i++)
    {
        ps_shader.lv_data[i].type = ID_PS_HLSL_SHADER;
        if (i == 0)
        {
            ps_shader.lv_data[i].subtype = TYPE_GUID;
            ps_shader.lv_data[i].block_data = ps_shader.psid;
            hex_to_wchar(temp, ps_shader.psid, 16);
            delay += 34;
        }
        else
        {
            ps_shader.lv_data[i].subtype = TYPE_UINT32;
            ps_shader.lv_data[i].block_data = arr[i - 1];
            swprintf_s(&temp[nextpos], 11, L"%u", **arr[i - 1]);
            delay += 5;
        }
        ps_shader.lv_data[i].edit_enable = TRUE;
        ps_shader.lv_data[i].struct_ptr = (void*)&ps_shader;
        set_item(LVIF_TEXT | LVIF_PARAM, i,0,vs_strings[i], ps_shader.lv_items[i],&ps_shader.lv_data[i]);
        ListView_InsertItem(hwndList, &ps_shader.lv_items[i]);
        ListView_SetItemText(hwndList, i, 1, &temp[nextpos]);
        nextpos = delay;
    }
    ps_shader.initialize_items = FALSE;
    return;
}



void show_vs_lv(HWND hwnd, HWND hwndList, bc2_vs& vs_shader, int cxChar, int cyChar, RECT rc, RECT rcTree)
{
    bool result;
    uint32_t **arr[3] = { &vs_shader.txeindex, &vs_shader.d3d1p, &vs_shader.d3d2p };

    if (vs_shader.initialize_items)
    {
        vs_shader.lv_items = new LVITEM[4];         // the items must be kept alive until saving
        vs_shader.lv_data = new lv_item_data[4];    // the items must be kept alive until saving
    }
    if (!ListView_DeleteAllItems(hwndList)) {
        MessageBox(NULL, L"List View DELETE ALL ITEMS ERROR", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
    }

    wchar_t temp[90];
    int nextpos = 0;
    int delay = 0;
    for (uint32_t i = 0; i < 4; i++)
    {
        vs_shader.lv_data[i].type = ID_VS_HLSL_SHADER;
        if (i == 0)
        {
            vs_shader.lv_data[i].subtype = TYPE_GUID;
            vs_shader.lv_data[i].block_data = vs_shader.vsid;
            hex_to_wchar(temp,vs_shader.vsid,16);
            delay += 34;
        }
        else
        {
            vs_shader.lv_data[i].subtype = TYPE_UINT32;       
            vs_shader.lv_data[i].block_data = arr[i - 1];
            swprintf_s(&temp[nextpos], 11, L"%u", **arr[i - 1]);
            delay += 5;
        }
        vs_shader.lv_data[i].edit_enable = TRUE;
        vs_shader.lv_data[i].struct_ptr = (void*)&vs_shader;
        set_item(LVIF_TEXT | LVIF_PARAM, i, 0, vs_strings[i], vs_shader.lv_items[i], &vs_shader.lv_data[i]);
        ListView_InsertItem(hwndList, &vs_shader.lv_items[i]);
        ListView_SetItemText(hwndList, i, 1, &temp[nextpos]);
        nextpos = delay;
    }
    vs_shader.initialize_items = FALSE;
    return;
}




void show_txlist_lv(HWND hwnd, HWND hwndList, TxE &txeptr, int cxChar, int cyChar, RECT rc, RECT rcTree)
{
    const wchar_t* out1 = L"Texture: ";
    const wchar_t* out2 = L"Parameter: ";
    const wchar_t* out3 = L"Type: ";
    bool result;
    int acc = 0;
    int strlength = 0;
    int items = *txeptr.ntxt + *txeptr.nparams + *txeptr.ntypes;
    char* text[32]; // a big enough array
    bool draw_title[3] = { FALSE,FALSE,FALSE };
    uint32_t loop_control[3] = { *txeptr.ntxt, *txeptr.nparams , *txeptr.ntypes };

    if (txeptr.initialize_items)
    {
        txeptr.lv_items = new LVITEM[items];         // the items must be kept alive until saving
        txeptr.lv_data = new lv_item_data[items];    // the items must be kept alive until saving
    }
    if (!ListView_DeleteAllItems(hwndList)) {
        MessageBox(NULL, L"List View DELETE ALL ITEMS ERROR", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
    }

    for (int i = 0; i < loop_control[0]; i++) {
        txeptr.lv_data[acc].type = ID_TEXTURELIST;
        txeptr.lv_data[acc].subtype = TEXTURE_NAME;
        txeptr.lv_data[acc].block_data = &txeptr.txtptrs[i];
        txeptr.lv_data[acc].edit_enable = TRUE;
        set_item(LVIF_TEXT | LVIF_PARAM, acc, 0, out1, txeptr.lv_items[acc], &txeptr.lv_data[acc]);

      //  txeptr.lv_items[acc].lParam = (LPARAM)&txeptr.lv_data[acc];
        acc++;
    }
    for (int i = 0; i < loop_control[1]; i++) {
        txeptr.lv_data[acc].type = ID_TEXTURELIST;
        txeptr.lv_data[acc].subtype = PARAM_NAME;
        txeptr.lv_data[acc].block_data = &txeptr.paramptrs[i];
        txeptr.lv_data[acc].edit_enable = TRUE;
        set_item(LVIF_TEXT | LVIF_PARAM, acc, 0, out2, txeptr.lv_items[acc], &txeptr.lv_data[acc]);
      //  txeptr.lv_items[acc].lParam = (LPARAM)&txeptr.lv_data[acc];
        acc++;
    }
    for (int i = 0; i < loop_control[2]; i++) {
        txeptr.lv_data[acc].type = ID_TEXTURELIST;
        txeptr.lv_data[acc].subtype = MEMBER_TYPE;
        txeptr.lv_data[acc].block_data = &txeptr.typeptrs[i];
        txeptr.lv_data[acc].edit_enable = TRUE;
        set_item(LVIF_TEXT | LVIF_PARAM, acc, 0, out3, txeptr.lv_items[acc], &txeptr.lv_data[acc]);
       // txeptr.lv_items[acc].lParam = (LPARAM)&txeptr.lv_data[acc];
        acc++;
    }
    int hwnd_index = 0;
    int w_pos = 0;
    wchar_t temp[140];
    char* name_str;
    txe_e* txtptrs = nullptr;
    txe_p* paramptr = nullptr;
    txe_t* typeptr = nullptr;
    for (uint32_t i = 0; i < items; i++)
    {
        lv_item_data* item_data = (lv_item_data*)txeptr.lv_items[i].lParam;
        switch (txeptr.lv_data[i].subtype)
        {
        case TEXTURE_NAME:
            txtptrs = (txe_e*)item_data->block_data;
            name_str = txtptrs->name;
            strlength = strlen(const_cast<char*>(name_str));
            mbstowcs(temp, name_str, strlength + 1);
            ListView_InsertItem(hwndList, &txeptr.lv_items[i]);
            ListView_SetItemText(hwndList, i, 1, temp);
            break;
        case PARAM_NAME:
            paramptr = (txe_p*)item_data->block_data;
            name_str = paramptr->name;
            strlength = strlen(const_cast<char*>(name_str));
            mbstowcs(temp, name_str, strlength + 1);
            ListView_InsertItem(hwndList, &txeptr.lv_items[i]);
            ListView_SetItemText(hwndList, i, 1, temp);
            break;
        case MEMBER_TYPE:
            typeptr = (txe_t*)item_data->block_data;
            name_str = typeptr->name;
            strlength = strlen(const_cast<char*>(name_str));
            mbstowcs(temp, name_str, strlength + 1);
            ListView_InsertItem(hwndList, &txeptr.lv_items[i]);
            ListView_SetItemText(hwndList, i, 1, temp);
            break;
        }
    }
    txeptr.initialize_items = FALSE;
    return;
}