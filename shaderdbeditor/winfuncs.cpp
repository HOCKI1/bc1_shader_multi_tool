#define _CRT_SECURE_NO_WARNINGS
#include "funcs.h"
#include "structs.h"
#include <string>
#include <vector>
#include <algorithm>
#include "Resource.h"

#ifndef UNICODE  
typedef TCHAR wchar_t;
#else
typedef std::wstring String;
#endif


//HTREEITEM AddItemToTree(HWND hwndTV, LPTSTR lpszItem, int nLevel, int index);
HTREEITEM AddItemToTree(HWND hwndTV, LPTSTR lpszItem, int nLevel, int type, hti_lparam* lparam);
HTREEITEM AddChild(HWND hwndTV, LPTSTR lpszItem, int nLevel, int type, HTREEITEM hti_parent);
HTREEITEM add_child(HWND hwndTV, LPTSTR lpszItem, int nLevel, hti_lparam* lparam, HTREEITEM hti_parent);
bool hide_tree(HWND hwndTV, int& treesize);



OPENFILENAMEA get_output_file_name(HWND hwnd, const char* prefix)
{
    OPENFILENAMEA ofn;
    static char szFileName[400] = "";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn); // SEE NOTE BELOW
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = prefix;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = 400;
    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "txt";
    return ofn;
}


void create_wnd_and_txt(HWND hwnd,HDC hdc, RECT rcTree, int cxChar, int cyChar, int hwnd_index, generic_hwnds*& hwnd_container, const char* title, const char* wnd_text)
{
    bool result;
    TextOutA(hdc, rcTree.right - (rcTree.left - 23), 24 + ((cyChar + 14) * (hwnd_index * 2)), (LPCSTR)title, strlen(title));
    result = windowcreator_new(hwnd, hwnd_container->w_hwnds[hwnd_index], (rcTree.right - (rcTree.left - 20)), (20 + ((cyChar + 14) * (hwnd_index * 2 + 1))), (cxChar) * 100, cyChar + 8);
    if (!SetWindowTextA(hwnd_container->w_hwnds[hwnd_index], wnd_text))
    {
        MessageBoxA(hwnd, "SHOW VX BUFFER ERROR", "MessageBox", MB_OK);
    }

    return;
}


bool clean_and_renew_generic(HWND hwnd,generic_hwnds* &hwnd_container, int new_size)
{
    bool result;
    if (!hwnd_container) {
         hwnd_container = new generic_hwnds;
         hwnd_container->w_hwnds_size = new_size;
         hwnd_container->w_hwnds = new HWND[new_size];
         hwnd_container->first_draw = TRUE;
         hwnd_container->destroy = FALSE;
         return TRUE;
    }
    else {
        if (hwnd_container->destroy)
        {
            for (uint32_t i = 0; i < hwnd_container->w_hwnds_size; i++)
            {
                result = DestroyWindow(hwnd_container->w_hwnds[i]);
                if (!result) {
                    MessageBoxA(hwnd, "ERROR:clean_and_renew_generic", "MessageBox", MB_OK);
                }
            }
        }
        delete[] hwnd_container->w_hwnds;
        hwnd_container->w_hwnds = new HWND[new_size];
        hwnd_container->w_hwnds_size = new_size;
        hwnd_container->first_draw = TRUE;
        hwnd_container->destroy = FALSE;
        return TRUE;
    }
}


bool show_shaders(HWND hwndTV, database_export* &new_db, int& treesize)
{
    HTREEITEM hti_parents[3]; // 1 = Shaders, 2 = Systems, 3 = Mesh 
    HTREEITEM* hti_systems = nullptr;
    int n_parents = 0;
    int systems_idx = 0;
    int shader_level = 0;
    uint32_t table_types[4] = {vs_type,ps_type,txe_type,txe_type};
    LPTSTR item_shader = NULL;
    LPTSTR sys = NULL;
    LPTSTR item_mesh;
    TCHAR basechr[] = TEXT("DB");
    TCHAR n1[] = TEXT("Shaders");
    TCHAR n2[] = TEXT("Systems");
    TCHAR n3[] = TEXT("Meshes");
    LPTSTR base = basechr;
    bool result;
    wchar_t shader_name[320];
    wchar_t sys_name[320];
    wchar_t mesh_name[320];
    wchar_t* table_name = const_cast<wchar_t*>(L"Table Reference ");
    static const wchar_t* table_childs[5] = {L"Vertex Shader", L"Pixel Shader", L"Texture List Empty", L"Texture List", L"Vertex Buffer"};

    result = hide_tree(hwndTV, treesize);

    lprm_parents->lprms[0].hti_index = 1;
    lprm_parents->lprms[0].hti_type = 103;    
    lprm_parents->lprms[0].selected = FALSE;
    AddItemToTree(hwndTV, base, 1, 0, &lprm_parents->lprms[0]);
    lprm_parents->lprms_counter++;
    lprm_parents->lprms[lprm_parents->lprms_counter].hti_index = 1;
    lprm_parents->lprms[lprm_parents->lprms_counter].hti_type = ID_SHADERS_P_NODE;
    lprm_parents->lprms[lprm_parents->lprms_counter].selected = FALSE;
    new_db->shader_p_node = AddItemToTree(hwndTV, (LPTSTR)&n1,2,0, &lprm_parents->lprms[lprm_parents->lprms_counter]); // Shaders node 
    lprm_parents->lprms_counter++;
    if (new_db->n_systems)
    {
        lprm_parents->lprms[lprm_parents->lprms_counter].hti_index = 1;
        lprm_parents->lprms[lprm_parents->lprms_counter].hti_type = ID_SYSTEMS_P_NODE;
        lprm_parents->lprms[lprm_parents->lprms_counter].selected = FALSE;
        //hti_systems = new HTREEITEM[new_db->n_systems];
        hti_parents[n_parents] = AddItemToTree(hwndTV, (LPTSTR)&n2, 2, 0, &lprm_parents->lprms[lprm_parents->lprms_counter]); // Systems node
        lprm_parents->lprms_counter++;
        systems_idx = n_parents;
        n_parents++;
        for (uint32_t i = 0; i < new_db->n_systems; i++)
        {
            lprm_parents->lprms[lprm_parents->lprms_counter].hti_index = i;
            lprm_parents->lprms[lprm_parents->lprms_counter].hti_type = ID_SYSTEMS_N_NODE;
            lprm_parents->lprms[lprm_parents->lprms_counter].selected = FALSE;
            mbstowcs(sys_name, new_db->systems_ref[i].name, 320);
            sys = (LPTSTR)sys_name;
            new_db->systems_ref[i].str_item = add_child(hwndTV, sys, 3, &lprm_parents->lprms[lprm_parents->lprms_counter], hti_parents[systems_idx]);
            lprm_parents->lprms_counter++;
        }
    }
    lprm_parents->lprms[lprm_parents->lprms_counter].hti_index = 3;
    lprm_parents->lprms[lprm_parents->lprms_counter].hti_type = ID_MESH_P_NODE;
    lprm_parents->lprms[lprm_parents->lprms_counter].selected = FALSE;
    new_db->mesh_p_node = AddItemToTree(hwndTV, (LPTSTR)&n3, 2, 0, &lprm_parents->lprms[lprm_parents->lprms_counter]); // Mesh node
    lprm_parents->lprms_counter++;
    for (uint32_t i = 0; i < new_db->n_shaders; i++)   // THIS IS LEVEL 2 
    {
        mbstowcs(shader_name, new_db->shaders[i].shader_ref->name, 320);
        item_shader = (LPTSTR)shader_name;
        new_db->shaders[i].lparam_data.hti_type = ID_SHADER_NODE;
        new_db->shaders[i].lparam_data.hti_index = i;
        new_db->shaders[i].lparam_data.selected = FALSE;
        if (!new_db->shaders[i].is_child)
        {
            new_db->shaders[i].hti_shader = add_child(hwndTV, item_shader, 3, &new_db->shaders[i].lparam_data, new_db->shader_p_node);
            shader_level = 3;
        }
        else
        {                   
            new_db->shaders[i].hti_shader = add_child(hwndTV, item_shader, 4, &new_db->shaders[i].lparam_data, new_db->systems_ref[new_db->shaders[i].root_node_idx[0]].str_item);
            shader_level = 4;
        }
        for (uint32_t j = 0; j < new_db->shaders[i].tables_ref->n_table_refs; j++) // THIS IS LEVEL 3 (TABLES LEVEL)
        {
            new_db->shaders[i].table_members[j].lparam_data.hti_index = j;
            new_db->shaders[i].table_members[j].lparam_data.hti_type = 5;
            new_db->shaders[i].table_members[j].lparam_data.shader_index = i;
            //new_db->shaders[i].table_members[j].table_item = add_child(hwndTV, table_name, (shader_level + 1), &lprm_childs->lprms[lprm_childs->lprms_counter], new_db->shaders[i].hti_shader);          
            new_db->shaders[i].table_members[j].table_item = add_child(hwndTV, table_name, (shader_level + 1), &new_db->shaders[i].table_members[j].lparam_data, new_db->shaders[i].hti_shader);
            for (uint32_t k = 0; k < 4; k++)
            {
                lprm_childs->lprms[lprm_childs->lprms_counter].hti_index = j; // use table index here
                lprm_childs->lprms[lprm_childs->lprms_counter].hti_type = table_types[k];
                lprm_childs->lprms[lprm_childs->lprms_counter].shader_index = i;
                lprm_childs->lprms[lprm_childs->lprms_counter].selected = FALSE;
                new_db->shaders[i].table_members[j].hti_table_childs[k] = add_child(hwndTV, (LPTSTR)table_childs[k], (shader_level + 2), &lprm_childs->lprms[lprm_childs->lprms_counter], new_db->shaders[i].table_members[j].table_item);
                lprm_childs->lprms_counter++;
            }
        }
    }
    for (uint32_t i = 0; i < new_db->n_mesh_refs; i++)
    {
        mbstowcs(mesh_name, new_db->mesh_refs[i].name, 320);
        item_mesh = (LPTSTR)mesh_name;
        lprm_childs->lprms[lprm_childs->lprms_counter].hti_index = i;
        lprm_childs->lprms[lprm_childs->lprms_counter].hti_type = ID_MESH_NODE;
        lprm_childs->lprms[lprm_childs->lprms_counter].selected = FALSE;
        new_db->mesh_refs[i].str_item = add_child(hwndTV,item_mesh,3,&lprm_childs->lprms[lprm_childs->lprms_counter],new_db->mesh_p_node);
        lprm_childs->lprms_counter++;
        new_db->mesh_refs[i].vx_buffer->lparam_data.hti_index = i;
        new_db->mesh_refs[i].vx_buffer->lparam_data.hti_type = 6;
        new_db->mesh_refs[i].vx_buffer->vx_item = add_child(hwndTV, (LPTSTR)table_childs[4], 4, &new_db->mesh_refs[i].vx_buffer->lparam_data, new_db->mesh_refs[i].str_item);

    }
    if (lprm_childs->lprms_counter > lprm_childs->max_val)
    {
        lprm_childs->max_val = lprm_childs->lprms_counter;
    }
    if (lprm_parents->lprms_counter > lprm_parents->max_val)
    {
        lprm_parents->max_val = lprm_parents->lprms_counter;
    }
    treesize = 1;
    return TRUE;
}


bool show_strings_tree(HWND hwndTV, database* dbptr, int &treesize, int type)
{
    bool result;
    HTREEITEM hti_parents[3];
    HTREEITEM hti_child;
    TCHAR name1[] = TEXT("Shaders");
    TCHAR name2[] = TEXT("Systems");
    TCHAR name3[] = TEXT("Meshes");
    TCHAR basechr[] = TEXT("DB");
    LPTSTR base = basechr;
    LPTSTR item = NULL;
    static LPTSTR parent_names[3] = {name1,name2,name3};
    wchar_t item_name[320];

    result = hide_tree(hwndTV, treesize);
    lprm_parents->lprms[0].hti_index = 1;
    lprm_parents->lprms[0].hti_type = -1;
    lprm_parents->lprms_counter++;
    AddItemToTree(hwndTV, base, 1, 0, lprm_parents[0].lprms);
    for (uint32_t i = 0; i < 3; i++)
    {
        lprm_parents->lprms[lprm_parents->lprms_counter].hti_index = 2;
        lprm_parents->lprms[lprm_parents->lprms_counter].hti_type = 723;
        hti_parents[i] = AddItemToTree(hwndTV, parent_names[i], 2, 0,&lprm_parents->lprms[lprm_parents->lprms_counter]);
        if (hti_parents[i] == NULL) {
            return FALSE;
        }
        dbptr->str_parent[i] = hti_parents[i];
        for (uint32_t j = 0; j < dbptr->n_strings[i]; j++)
        {
            lprm_childs->lprms[j].hti_index = j; 
            lprm_childs->lprms[j].hti_type = type;
            mbstowcs(item_name, dbptr->strings[i][j].name, 320);
            item = (LPTSTR)item_name;
            hti_child = add_child(hwndTV,item,3, &lprm_childs->lprms[j],hti_parents[i]);
            lprm_childs->lprms_counter++;
            if (hti_child == NULL) {
                return FALSE;
            }
            dbptr->strings[i][j].str_item = hti_child;       
        }  
        lprm_parents->lprms_counter++;
    }
    if (lprm_childs->lprms_counter > lprm_childs->max_val)
    {
        lprm_childs->max_val = lprm_childs->lprms_counter;
    }
    treesize = 1;
    return TRUE;
}

bool show_generic_tree(HWND hwndTV, database* dbptr, int& treesize, int type, wchar_t* name, uint32_t loop, int charpos, wchar_t* basename)
{
    bool result;
    TCHAR basechr[] = TEXT("DB");
    LPTSTR base = basechr;
    HTREEITEM hti;
    LPTSTR item = NULL;
    char id[33];
    wchar_t id_name[33];
    hti_lparam* lparam_bitch = new hti_lparam;
    hti_lparam* lparam_parent = new hti_lparam;
    lprm_parents->lprms[0].hti_index = 555;
    lprm_parents->lprms[0].hti_type = -1;
    result = hide_tree(hwndTV, treesize);
    AddItemToTree(hwndTV, base, 1, 0, lprm_parents[0].lprms);
    for (int i = 0; i < loop; i++)
    {
        lprm_childs->lprms[i].hti_index = i;
        lprm_childs->lprms[i].hti_type = type;
        lprm_childs->lprms_counter++;
        if (type != 6) 
        {
            wcscpy(name, basename);
            wsprintf(&name[charpos], L"%d", i); // 13 = char pos 
            item = (LPTSTR)name;
        }
        else 
        {
            hex_to_string(id, dbptr->vx_buffers[i].vxbuffer_id, 16);
            mbstowcs(id_name, id, 33);
            item = (LPTSTR)id_name;
        }
        hti = AddItemToTree(hwndTV, item, 2, type, &lprm_childs->lprms[i]);  // type indicates if the item is a txe, a shader, a table entry, etc. used later to call the appropriate function

        if (hti == NULL) {
            return FALSE;
        }
        switch (type)
        {
        case 1:
        {          
            dbptr->txeptr[i].lparam_data.hti_index = i;
            dbptr->txeptr[i].lparam_data.hti_type = type;
            dbptr->txeptr[i].txeitem = hti;
            break;
        }
        case 3:
        {
            dbptr->vs_shader[i].lparam_data.hti_index = i;
            dbptr->vs_shader[i].lparam_data.hti_type = type;
            dbptr->vs_shader[i].vs_item = hti;
            break;
        }
        case 4:
        {
            dbptr->ps_shader[i].lparam_data.hti_index = i;
            dbptr->ps_shader[i].lparam_data.hti_type = type;
            dbptr->ps_shader[i].ps_item = hti;
            break;
        }
        case 5:
        {
            dbptr->table[i].lparam_data.hti_index = i;
            dbptr->table[i].lparam_data.hti_type = type;
            dbptr->table[i].table_item = hti;
            break;
        }
        case 6:
        {
            dbptr->vx_buffers[i].lparam_data.hti_index = i;
            dbptr->vx_buffers[i].lparam_data.hti_type = type;
            dbptr->vx_buffers[i].vx_item = hti;
            break;
        }
        }      
    }
    if (lprm_childs->lprms_counter > lprm_childs->max_val)
    {
        lprm_childs->max_val = lprm_childs->lprms_counter;
    }
    treesize = 1;
    delete lparam_bitch;
    return TRUE;

}

bool hide_tree(HWND hwndTV, int& treesize)
{

    if (treesize == 0)
    {
         return TRUE;
    }
    else {
        if (!TreeView_DeleteAllItems(hwndTV)) {
            MessageBox(NULL, L"ERROR ON CALL TO TreeView_DeleteAllItems", L"Error!",
                MB_ICONEXCLAMATION | MB_OK);
            return FALSE;
        }
    }
    treesize = 0;
    return TRUE;
}


HTREEITEM add_child(HWND hwndTV, LPTSTR lpszItem, int nLevel, hti_lparam* lparam, HTREEITEM hti_parent)
{
    TVITEM tvi;
    TVINSERTSTRUCT tvins;
    static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
    static HTREEITEM hPrevRootItem = NULL;
    static HTREEITEM hPrevLev2Item = NULL;
    HTREEITEM hti;

    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;

    // Set the text of the item. 
    tvi.pszText = lpszItem;
    tvi.cchTextMax = sizeof(tvi.pszText) / sizeof(tvi.pszText[0]);

    // Assume the item is not a parent item, so give it a 
    // document image. 
    tvi.iImage = 0;
    tvi.iSelectedImage = 0;

    // Save the heading level in the item's application-defined 
    // data area. 
    tvi.lParam = (LPARAM)lparam;
    tvins.item = tvi;
    tvins.hInsertAfter = hPrev;

    // Set the parent item based on the specified level. 
    if (nLevel == 1)
        tvins.hParent = TVI_ROOT;
    else if (nLevel == 2)
        tvins.hParent = hPrevRootItem;
    else
    {
        tvins.hParent = hti_parent;
    }
    // Add the item to the tree-view control. 
    hPrev = (HTREEITEM)SendMessage(hwndTV, TVM_INSERTITEM,
        0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);

    if (hPrev == NULL)
        return NULL;

    // Save the handle to the item. 
    if (nLevel == 1)
        hPrevRootItem = hPrev;
    else if (nLevel == 2)
        hPrevLev2Item = hPrev;

    // The new item is a child item. Give the parent item a 
    // closed folder bitmap to indicate it now has child items. 
    if (nLevel > 1)
    {
        hti = TreeView_GetParent(hwndTV, hPrev);
        tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvi.hItem = hti;
        tvi.iImage = 0;
        tvi.iSelectedImage = 0;
        TreeView_SetItem(hwndTV, &tvi);
    }

    return hPrev;
}



HTREEITEM AddChild(HWND hwndTV, LPTSTR lpszItem, int nLevel, int type, HTREEITEM hti_parent)
{
    TVITEM tvi;
    TVINSERTSTRUCT tvins;
    static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
    static HTREEITEM hPrevRootItem = NULL;
    static HTREEITEM hPrevLev2Item = NULL;
    HTREEITEM hti;

    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;

    // Set the text of the item. 
    tvi.pszText = lpszItem;
    tvi.cchTextMax = sizeof(tvi.pszText) / sizeof(tvi.pszText[0]);

    // Assume the item is not a parent item, so give it a 
    // document image. 
    tvi.iImage = 0;
    tvi.iSelectedImage = 0;

    // Save the heading level in the item's application-defined 
    // data area. 
    tvi.lParam = (LPARAM)type;
    tvins.item = tvi;
    tvins.hInsertAfter = hPrev;

    // Set the parent item based on the specified level. 
    if (nLevel == 1)
        tvins.hParent = TVI_ROOT;
    else if (nLevel == 2)
        tvins.hParent = hPrevRootItem;
    else
    {
        tvins.hParent = hti_parent;
    }
    // Add the item to the tree-view control. 
    hPrev = (HTREEITEM)SendMessage(hwndTV, TVM_INSERTITEM,
        0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);

    if (hPrev == NULL)
        return NULL;

    // Save the handle to the item. 
    if (nLevel == 1)
        hPrevRootItem = hPrev;
    else if (nLevel == 2)
        hPrevLev2Item = hPrev;

    // The new item is a child item. Give the parent item a 
    // closed folder bitmap to indicate it now has child items. 
    if (nLevel > 1)
    {
        hti = TreeView_GetParent(hwndTV, hPrev);
        tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvi.hItem = hti;
        tvi.iImage = 0;
        tvi.iSelectedImage = 0;
        TreeView_SetItem(hwndTV, &tvi);
    }

    return hPrev;
}




HTREEITEM AddItemToTree(HWND hwndTV, LPTSTR lpszItem, int nLevel, int type, hti_lparam* lparam)
{
    TVITEM tvi;
    TVINSERTSTRUCT tvins;
    static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
    static HTREEITEM hPrevRootItem = NULL;
    static HTREEITEM hPrevLev2Item = NULL;
    HTREEITEM hti;

    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;

    // Set the text of the item. 
    tvi.pszText = lpszItem;
    tvi.cchTextMax = sizeof(tvi.pszText) / sizeof(tvi.pszText[0]);

    // Assume the item is not a parent item, so give it a 
    // document image. 
    tvi.iImage = 0;
    tvi.iSelectedImage = 0;

    // Save the heading level in the item's application-defined 
    // data area. 
    //tvi.lParam = (LPARAM)type;
    tvi.lParam = (LPARAM)lparam;
    tvins.item = tvi;
    tvins.hInsertAfter = hPrev;

    // Set the parent item based on the specified level. 
    if (nLevel == 1)
        tvins.hParent = TVI_ROOT;
    else if (nLevel == 2)
        tvins.hParent = hPrevRootItem;
    else
        tvins.hParent = hPrevLev2Item;

    // Add the item to the tree-view control. 
    hPrev = (HTREEITEM)SendMessage(hwndTV, TVM_INSERTITEM,
        0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);

    if (hPrev == NULL)
        return NULL;

    // Save the handle to the item. 
    if (nLevel == 1)
        hPrevRootItem = hPrev;
    else if (nLevel == 2)
        hPrevLev2Item = hPrev;

    // The new item is a child item. Give the parent item a 
    // closed folder bitmap to indicate it now has child items. 
    if (nLevel > 1)
    {
        hti = TreeView_GetParent(hwndTV, hPrev);
        tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvi.hItem = hti;
        tvi.iImage = 0;
        tvi.iSelectedImage = 0;
        TreeView_SetItem(hwndTV, &tvi);
    }

    return hPrev;
}


bool windowcreator_new(HWND hwnd, HWND& hwndout, int xpos, int ypos, int xsize, int ysize) 
{


    hwndout = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"STATIC",  // Predefined class; Unicode assumed 
        0,      // Button text 
        WS_VISIBLE | WS_CHILD | SS_SIMPLE,   // Styles  SS_WHITERECT | SS_BLACKFRAME
        xpos,         // x position 
        ypos,         // y position 
        xsize,        // Button width
        ysize,        // Button height
        hwnd,     // Parent window
        NULL,       // No menu.
        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
        NULL);
    if (hwndout == NULL)
    {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    ShowWindow(hwndout, 1);
    UpdateWindow(hwndout);
    return TRUE;
}


bool window_no_border(HWND hwnd, HWND& hwndout, RECT rcTree, int cyChar, int cxChar, int i, char* wndtext)
{

    wchar_t name[36];
    int length = strlen(wndtext);
    swprintf(name, length, L"%hs", wndtext);

    hwndout = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"STATIC",  // Predefined class 
        name,      // text 
        WS_VISIBLE | WS_CHILD | WS_SYSMENU,   // Styles SS_WHITERECT | SS_BLACKFRAME
        rcTree.right - (rcTree.left - 20),         // x position 
        20 + ((cyChar + 14) * i),         // y position 
        (cxChar) * 90,        // width
        cyChar + 8,        // height
        hwnd,     // Parent window
        NULL,       // No menu
        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
        NULL);
    if (hwndout == NULL)
    {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    ShowWindow(hwndout, 1);
    UpdateWindow(hwndout);
    return TRUE;
}

void window_reset(HWND hwnd, HWND hwndTree, RECT rc, RECT rcTree) 
{
    GetClientRect(hwnd, &rc);
    GetWindowRect(hwndTree, &rcTree);
    RECT rcupd;
    HRGN wrgn;
    rcupd.left = rcTree.right - rcTree.left;
    rcupd.right = rc.right;
    rcupd.top = rc.top;
    rcupd.bottom = rc.bottom - (rc.bottom / 4.35);
    wrgn = CreateRectRgn(rcupd.left, rcupd.top, rcupd.right, rcupd.bottom);
    RedrawWindow(hwnd, &rcupd, wrgn, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    return;
}

BOOL insert_item_lv(HWND hWndListView,char* value, int item_count)
{
    LVITEM lvI;
    wchar_t temp[350];
    mbstowcs(temp, value, strlen(value)+1);
    // Initialize LVITEM members that are common to all items.
	lvI.pszText = (LPWSTR)temp; // Sends an LVN_GETDISPINFO message.
	lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
	lvI.stateMask = 0;
	lvI.iSubItem = 0;
	lvI.state = 0;
	lvI.iItem = item_count;
	lvI.iImage = 0;
	// Insert items into the list.
	if (ListView_InsertItem(hWndListView, &lvI) == -1)
		return FALSE;


    return TRUE;
}

BOOL set_col_val(HWND hWndListView, LVITEMW &lv_item, uint32_t index, uint32_t col_index)
{
    LVITEMA lvI;
    wchar_t temp[350];
   // mbstowcs(temp, lv_item->pszText, strlen(lv_item->pszText) + 1);
    // Initialize LVITEM members that are common to all items.
   /* lvI.pszText = (LPWSTR)L"FUCKER";
    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvI.stateMask = -1;
    lvI.iSubItem = col_index;
    lvI.state = 0;
    lvI.iItem = index;
    lvI.iImage = 0;*/
    lv_item.iItem = index;
    lv_item.iSubItem = col_index;
    lv_item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lv_item.stateMask = -1;
    lv_item.state = 0;
    // Insert items into the list.
    if (ListView_SetItem(hWndListView, &lv_item) == -1)
        return FALSE;


    return TRUE;
}

OPENFILENAMEA open_db(HWND hwnd)
{
    OPENFILENAMEA ofn;
    static char szFileName[MAX_PATH] = "";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn); // SEE NOTE BELOW
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "All Supported Shaders\0*.dx11shaderdatabase;*.ps3shaderdatabase\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    ofn.lpstrDefExt = "txt";
    return ofn;
}