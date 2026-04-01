#pragma once
#include "Resource.h"
#include "funcs.h"
#include "framework.h"
#include "shaderdbeditor.h"
#include "structs.h"
#include <string>
#include <sstream>
#include <locale> 
#include <codecvt>
#include "showfuncs.h"
#include "exportfuncs.h"
#include "listview.h"
#include "boilerplate.h"
#include "show_lv_funcs.h"
#include "g_variables.h"
#include "editfuncs.h"
#include "importfuncs.h"
#include "assetc.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

char* database_pool = nullptr;
uint32_t db_pool_pos;

vxbuffer* assetc_vx_buffers;
uint32_t assetc_n_vx_buffers;
edit_pool file_edit_pool;
allocated_dbs allocated_db;
WNDCLASSEX gridclass = {0};
int iItem, iSubItem;
bool shader_view = FALSE;
HWND hwndTree, hwndList, hwndAsset,hwndMain;
HWND hwndEdit;
HANDLE hIcon;
HMENU hMenu, hSubMenu, hDebugMenu;
WNDPROC ORIGINAL_EDIT_PROC = 0;
WNDPROC PARENT_WND_PROC = 0;
txe_hwnds* txehwndlocal = NULL;
generic_hwnds* hwnd_container = NULL;
database_export* new_db = nullptr;
int selected_shaders = 0;
int selected_mesh_refs = 0;
FILE* shdrdb;
lprm_storage* lprm_childs = new lprm_storage;
lprm_storage* lprm_parents = new lprm_storage;
const int shader_type = 8;
char blank[90];
const wchar_t* assetc_classname = L"AssetC";
//setup converter
using convert_type = std::codecvt_utf8<wchar_t>;
std::wstring_convert<convert_type, wchar_t> converter;


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)                                                                                                                                                                                                                                                                                                                                                    
{
    bool rslt;
    HDC hdc;
    static RECT rc;
    static RECT rcTree;
    TEXTMETRICA tm;
    static int cxChar, cyChar, treesize, cindex;
    static NMTREEVIEW tv_struct_selected;
    static shader_struct* shaders = NULL;
    static HMENU hPopupMenu;
    static TVITEM selected_tv_item;
    static char szFileName[400];
    int i = 22;
    int xPos;
    int yPos;
    std::stringstream box_message;
    static database* dbptr = nullptr;

    switch (msg)
    {
    case WM_CREATE:
    {
       if (hIcon) {
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

            SendMessage(GetWindow(hwnd, GW_OWNER), WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(GetWindow(hwnd, GW_OWNER), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        }
        hwndMain = hwnd;
        MenuTool(hwnd,hMenu,hSubMenu, hDebugMenu);
        GetClientRect(hwnd, &rc);
        CreateTree(hwnd,hwndTree,rc);
        hdc = GetDC(hwnd);
        GetTextMetricsA(hdc, &tm);
        cxChar = tm.tmAveCharWidth;
        cyChar = tm.tmHeight + tm.tmExternalLeading;
        ReleaseDC(hwnd, hdc);
        GetWindowRect(hwndTree,&rcTree);
        hwndList = create_lv(hwnd, rc, rcTree);
        PARENT_WND_PROC = (WNDPROC)SetWindowLongPtrW(hwndList, GWLP_WNDPROC, (LONG_PTR)ListViewProc);
        HFONT defaultFont = CreateFont(16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
        SendMessage(hwndList, WM_SETFONT, WPARAM(defaultFont), TRUE);
        int aval_width = rc.right - (rcTree.right - rcTree.left);
        treesize = 0;
        init_columns(hwndList, aval_width);
		ShowWindow(hwndList, 1);
		UpdateWindow(hwndList);
        hPopupMenu = CreatePopupMenu();
        InsertMenu(hPopupMenu, 0, MF_BYPOSITION | MF_STRING, ID_CLONE_TXE, L"Clone");
    }
    break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_FILE_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        case ID_FILE_OPEN:
        {
            OPENFILENAMEA ofn = open_db(hwnd);
            if (GetOpenFileNameA(&ofn))
            {
                static char* dbfile;
                static int dbsize;           
                strncpy(szFileName, ofn.lpstrFile, 400);
                shdrdb = fopen(szFileName, "r+b");
                fseek(shdrdb, SEEK_SET, SEEK_END);
                dbsize = ftell(shdrdb);
                fseek(shdrdb, 0, SEEK_SET);
                dbfile = (char*)malloc(dbsize * sizeof(char));
                fread(dbfile, 1, dbsize, shdrdb);
                fclose(shdrdb);
                uint32_t ptr_pos = 0;
                uint32_t* ptr_end = &ptr_pos;
                dbptr = dbfirstload(dbfile,ptr_end);
                dbfile += ptr_pos;   
                new_db = export_db_init(dbptr,dbfile);
                assetc_vx_buffers = new_db->vx_buffers;
                assetc_n_vx_buffers = new_db->n_vx_buffers;
                lprm_childs->lprms = new hti_lparam[100000];
                lprm_parents->lprms = new hti_lparam[60];
                lprm_childs->lprms_counter = 0;
                lprm_parents->lprms_counter = 0;
                lprm_childs->max_val = 0;
                lprm_parents->max_val = 0;
                file_edit_pool.pool = (char*)malloc(1000000 *sizeof(char));
                ZeroMemory(file_edit_pool.pool, 1000000 * sizeof(char));
                file_edit_pool.current_pos = 0;               
                allocated_db.used_slots = 0;
                allocated_db.mem_blocks[allocated_db.used_slots] = dbfile;
                allocated_db.used_slots++;
                enable_menu(hwnd, hMenu, hSubMenu, hDebugMenu);
                EnableMenuItem(hMenu, ID_SAVE_DB, MF_ENABLED);
                DrawMenuBar(hwnd);   
            }
            break;
        }
        case ID_TXE_VIEW:
            boilerplate_txe(hwnd,hwndTree,dbptr,treesize,hwnd_container,rc,rcTree);
            break;
        case ID_D3DPARAMS1_VIEW:
            break;
        case ID_VERTEXSHADER_VIEW:
        {          
            boilerplate_vs(hwnd, hwndTree, dbptr, treesize, hwnd_container, rc, rcTree);
            break;
        }
        case ID_PIXELSHADER_VIEW:
        {
            boilerplate_ps(hwnd, hwndTree, dbptr, treesize, hwnd_container, rc, rcTree);
            break;
        }
        case ID_TABLES_VIEW:
        {
            boilerplate_table(hwnd, hwndTree, dbptr, treesize, hwnd_container, rc, rcTree);
            break;
        }
        case ID_VBUFFERS_VIEW:
        {
            boilerplate_vbuffer(hwnd, hwndTree, dbptr, treesize, hwnd_container, rc, rcTree);
            break;
        }
        case ID_STRINGS_VIEW:
        {
            boilerplate_strings(hwnd, hwndTree, dbptr, treesize, hwnd_container, rc, rcTree);
            break;
        }
        case ID_SHADER_VIEW:
        {
            shader_view = TRUE;
            window_reset(hwnd, hwndTree, rc, rcTree);
            lprm_childs->lprms_counter = 0;
            lprm_parents->lprms_counter = 0;
            if ((hwnd_container) && (hwnd_container->destroy))
            {
                clean_and_renew_generic(hwnd, hwnd_container, 1);
            }
            show_shaders(hwndTree, new_db, treesize);
            break;
        }
        case ID_EXPORT_DB:
        {
            OPENFILENAMEA ofn_output = get_output_file_name(hwnd, "Dx11 Shader Database (*.dx11shaderdatabase)\0*.dx11shaderdatabase");
            if (GetSaveFileNameA(&ofn_output))
            {
                char output_name[400];
                strncpy(output_name, ofn_output.lpstrFile,400);
                export_db(hwnd,new_db, output_name,0);          
            }
            break;
        }
        case ID_UPDATE_LV:
            lprm_childs->lprms_counter = 0;
            lprm_parents->lprms_counter = 0;
            EnableMenuItem(hMenu, ID_UPDATE_LV, MF_DISABLED);
            DrawMenuBar(hwnd);
            show_shaders(hwndTree, new_db, treesize);
            break;
        case ENABLE_UPD_BTN:
            EnableMenuItem(hMenu, ID_UPDATE_LV, MF_ENABLED);
            DrawMenuBar(hwnd);
            break;
        case ID_CLONE_TXE:
        {
            hti_lparam* lparam = (hti_lparam*)selected_tv_item.lParam;
            cindex = lparam->shader_index;
            int table_index = lparam->hti_index;
            if (selected_tv_item.hItem == new_db->shaders[cindex].table_members[table_index].hti_table_childs[2]) // texture list empty 
            {
                int test = new_db->n_txe;
                clone_txe(new_db->shaders[cindex].table_members[table_index], 0, new_db);
                test = new_db->n_txe;
                SendMessage(hwnd, WM_COMMAND, ENABLE_UPD_BTN, 0);
            }
            else if (selected_tv_item.hItem == new_db->shaders[cindex].table_members[table_index].hti_table_childs[3]) // texture list 2 
            {
                int test2 = new_db->n_txe;
                clone_txe(new_db->shaders[cindex].table_members[table_index], 1, new_db);
                test2 = new_db->n_txe;
                SendMessage(hwnd, WM_COMMAND, ENABLE_UPD_BTN, 0);
            }
        }
            break;
        case ID_EXPORT_STR:
            OPENFILENAMEA ofn_output = get_output_file_name(hwnd, "Strings (*.dx11strings)\0*.dx11strings");
            if (GetSaveFileNameA(&ofn_output))
            {
                char output_name[400];
                strncpy(output_name, ofn_output.lpstrFile, 400);
                export_str(hwnd, new_db, output_name);
            }
            break;
        case ID_IMPORT_DB:
            OPENFILENAMEA ofn_db = get_output_file_name(hwnd, "Dx11 Shader Database (*.dx11shaderdatabase)\0*.dx11shaderdatabase");
            if (GetOpenFileNameA(&ofn_db))
            {
                char output_name[400];
                strncpy(output_name, ofn_db.lpstrFile, 400);
                new_db = import_db(new_db, output_name);
                assetc_vx_buffers = new_db->vx_buffers;
                assetc_n_vx_buffers = new_db->n_vx_buffers;
            }          
            break;
        case ID_SAVE_DB:
            export_db(hwnd, new_db, szFileName,1);
            ZeroMemory(file_edit_pool.pool, 1000000 * sizeof(char));
            file_edit_pool.current_pos = 0;
            break;
        case ID_EXPORT_HLSL:
        {
            char dst_name[400];
            BROWSEINFOA dst_folder = { 0 };
            dst_folder.hwndOwner = hwnd;
            dst_folder.lpszTitle = "Select Folder";
            dst_folder.ulFlags = BIF_USENEWUI;
            LPITEMIDLIST lpIDList = ::SHBrowseForFolderA(&dst_folder);
            if (lpIDList)
            {
                SHGetPathFromIDListA(lpIDList, dst_name);
                export_hlsl(new_db, dst_name);
            }
            break;
        }
        case ID_ASSETC_START:     
            EnableMenuItem(hMenu, ID_ASSETC_START, MF_DISABLED);
            hwndAsset = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                assetc_classname,
                L"Asset Creator",
                WS_OVERLAPPEDWINDOW| WS_EX_RIGHTSCROLLBAR,
                CW_USEDEFAULT, CW_USEDEFAULT,500, 800,
                hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            if (hwndAsset == NULL)
            {
                MessageBoxA(NULL, "Window Creation Failed!", "Error!",
                    MB_ICONEXCLAMATION | MB_OK);
                return 0;
            }
            break;
        case ID_ASSETC_ENABLE_MENU_BTN:
            EnableMenuItem(hMenu, ID_ASSETC_START, MF_ENABLED);
            break;
        }

    break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
    {
        int cxClient = LOWORD(lParam);
        int cyClient = HIWORD(lParam);
        RECT rc_wnd;
        SetWindowPos(hwndTree, NULL, 0, 0, cxClient/3.2, cyClient, SWP_SHOWWINDOW);
        GetWindowRect(hwndTree, &rcTree);
        GetWindowRect(hwndList, &rc_wnd);
        SendMessage(hwndList,WM_SIZE,0,lParam);
        SetWindowPos(hwndList, NULL, rcTree.right - rcTree.left, 0, cxClient - (rcTree.right - rcTree.left), cyClient, SWP_SHOWWINDOW);
        if (hwndEdit)
        {
            SetWindowPos(hwndEdit, NULL, rcTree.right - rcTree.left, cyClient - (cyClient / 4.35), cxClient - (rcTree.right - rcTree.left), cyClient / 4.35, SWP_SHOWWINDOW);
        }
        int aval_width = cxClient - (rcTree.right - rcTree.left);
        ListView_SetColumnWidth(hwndList, 0, 200);
        ListView_SetColumnWidth(hwndList, 1, aval_width-200);
        //ListView_SetColumnWidth(hwndList, 2, aval_width/3);
        GetClientRect(hwnd, &rc);   
        break;
    }
    case WM_LBUTTONUP:
		xPos = GET_X_LPARAM(lParam);
		yPos = GET_Y_LPARAM(lParam);
		box_message << "X: " << xPos << " Y: " << yPos;
		MessageBoxA(hwndTree, box_message.str().c_str(), "MessageBox", MB_OK);
		break;
    case UM_CHECKSTATECHANGE:
    {
        HTREEITEM hItemChanged = (HTREEITEM)lParam;  
        TVITEM tvi; // Declare a TVITEM structure to get the item information
        tvi.mask = TVIF_TEXT | TVIF_PARAM; // Specify the mask to get the text and the parameter of the item
        tvi.hItem = hItemChanged;
        tvi.stateMask = TVIS_STATEIMAGEMASK;
        LPWSTR szBuffer = new wchar_t[256];
        tvi.pszText = szBuffer; // Set the buffer to receive the item text
        tvi.cchTextMax = 256;
        TreeView_GetItem(hwndTree, &tvi);
        hti_lparam* lparam = (hti_lparam*)tvi.lParam;
        switch (lparam->hti_type)
        {
             case ID_MESH_NODE:              
                 new_db->mesh_refs[lparam->hti_index].selected = (!new_db->mesh_refs[lparam->hti_index].selected);                
                 break;
             case ID_SHADER_NODE:
             {
                 new_db->shaders[lparam->hti_index].lparam_data.selected = (!new_db->shaders[lparam->hti_index].lparam_data.selected);
                 break;
             }
             case ID_SHADERS_P_NODE: // Shaders Parent node 
             {
                 for (uint32_t i = 0; i < new_db->n_shaders; i++)
                 {
                     TreeView_SetCheckState(hwndTree, new_db->shaders[i].hti_shader, (!lparam->selected));
                     new_db->shaders[i].lparam_data.selected = (!lparam->selected);
                 }
                 lparam->selected = (!lparam->selected);
                 break;
             }
             case ID_SYSTEMS_P_NODE: // Systems Parent node 
             {
                 for (uint32_t i = 0; i < new_db->n_systems; i++)
                 {
                     TreeView_SetCheckState(hwndTree, new_db->systems_ref[i].str_item, (!lparam->selected));
                     new_db->systems_ref[i].selected = (!lparam->selected);
                 }
                 lparam->selected = (!lparam->selected);
                 break;
             }
             case ID_SYSTEMS_N_NODE: // Systems name node 
             {
                 TreeView_SetCheckState(hwndTree, new_db->systems_ref[lparam->hti_index].str_item, (!new_db->systems_ref[lparam->hti_index].selected));
                 new_db->systems_ref[lparam->hti_index].selected = (!new_db->systems_ref[lparam->hti_index].selected);
                 break;
             }
             case ID_MESH_P_NODE: // Mesh Parent node 
             {
                 for (uint32_t i = 0; i < new_db->n_mesh_refs; i++)
                 {
                     TreeView_SetCheckState(hwndTree, new_db->mesh_refs[i].str_item, (!lparam->selected));
                     new_db->mesh_refs[i].selected = (!lparam->selected);
                 }
                 lparam->selected = (!lparam->selected);
                 break;
             }
             case ID_VS_HLSL_SHADER:
             {
                 new_db->shaders[lparam->shader_index].table_members[lparam->hti_index].vs_shader->lparam_data.selected = (!lparam->selected);
                 lparam->selected = (!lparam->selected);
                 break;
             }
             case ID_PS_HLSL_SHADER:
             {
                 new_db->shaders[lparam->shader_index].table_members[lparam->hti_index].ps_shader->lparam_data.selected = (!lparam->selected);
                 lparam->selected = (!lparam->selected);
                 break;
             }
        }       
    }  
    case WM_NOTIFY:
    {
		LPNMHDR lpnmh2 = (LPNMHDR)lParam;
		TVHITTESTINFO ht = { 0 };
 
        switch (lpnmh2->idFrom)
        {
        case ID_TREEVIEW:
            LPNMHDR lpnmh = (LPNMHDR)lParam;
            switch (lpnmh2->code)
            {
                case NM_RCLICK:
                { 
                    if (treesize > 0)
                    {
                        TVITEM tvi;
                        POINT clickpos; 
                        GetCursorPos(&clickpos);
                        tvi.mask = TVIF_TEXT | TVIF_PARAM;
                        LPWSTR szBuffer = new wchar_t[256];
                        tvi.pszText = szBuffer; // Set the buffer to receive the item text
                        tvi.cchTextMax = 256;
                        DWORD dwpos = GetMessagePos();
                        ht.pt.x = GET_X_LPARAM(dwpos);
                        ht.pt.y = GET_Y_LPARAM(dwpos);
                        MapWindowPoints(HWND_DESKTOP, lpnmh2->hwndFrom, &ht.pt, 1);
                        TreeView_HitTest(lpnmh2->hwndFrom, &ht);
                        tvi.hItem = ht.hItem;
                        TreeView_GetItem(hwndTree, &tvi);
                        hti_lparam* lparam = (hti_lparam*)tvi.lParam;
                        selected_tv_item = tvi;
                        if (lparam->hti_type == 1 && (TVHT_ONITEMLABEL & ht.flags))
                        {
                           // InsertMenu(hPopupMenu, 0, MF_BYPOSITION | MF_STRING, ID_CLONE_TXE, L"Clone");
                            SetForegroundWindow(hwnd);
                            TrackPopupMenu(hPopupMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, clickpos.x, clickpos.y, 0, hwnd, NULL);
                            cindex = lparam->shader_index;
                            int table_index = lparam->hti_index;
                            if (tvi.hItem == new_db->shaders[cindex].table_members[table_index].hti_table_childs[2]) // texture list empty 
                            {
                                show_txlist_lv(hwnd, hwndList, *new_db->shaders[cindex].table_members[table_index].txeptr_null, cxChar, cyChar, rc, rcTree);
                            }
                            else if (tvi.hItem == new_db->shaders[cindex].table_members[table_index].hti_table_childs[3]) // texture list 2 
                            {
                                show_txlist_lv(hwnd, hwndList, *new_db->shaders[cindex].table_members[table_index].txeptr_2, cxChar, cyChar, rc, rcTree);
                            }
                            delete[] szBuffer;
                        }
                        else
                        {
                            delete[] szBuffer;
                            break;
                        }                      
                    }               
                }
                break;
                case NM_CLICK:
                {
                    DWORD dwpos = GetMessagePos();
                    ht.pt.x = GET_X_LPARAM(dwpos);
                    ht.pt.y = GET_Y_LPARAM(dwpos);
                    MapWindowPoints(HWND_DESKTOP, lpnmh2->hwndFrom, &ht.pt, 1);
                    TreeView_HitTest(lpnmh2->hwndFrom, &ht);
                    if (TVHT_ONITEMSTATEICON & ht.flags)
                    {
                        PostMessage(hwnd, UM_CHECKSTATECHANGE, 0, (LPARAM)ht.hItem);
                    }
                } 
                break;
                case TVN_SELCHANGED:
                {
                LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam; // Get the tree view notification structure
                tv_struct_selected.action = 1;
                tv_struct_selected.itemNew.hItem = pnmtv->itemNew.hItem;
                HTREEITEM hItem = pnmtv->itemNew.hItem;
                TVITEM tvi; // Declare a TVITEM structure to get the item information
                tvi.mask = TVIF_TEXT | TVIF_PARAM; // Specify the mask to get the text and the parameter of the item
                tvi.hItem = hItem;
                LPWSTR szBuffer = new wchar_t[256];
                tvi.pszText = szBuffer; // Set the buffer to receive the item text
                tvi.cchTextMax = 256;
                TreeView_GetItem(hwndTree, &tvi);
                std::wstring tviname = (std::wstring)tvi.pszText;
                std::string converted = converter.to_bytes(tviname);
                hti_lparam* lparam = (hti_lparam*)tvi.lParam;
                delete[] szBuffer;
                switch (lparam->hti_type)
                {
                case ID_TEXTURELIST: // TEXTURE LIST
                {
                    GetClientRect(hwnd, &rc);
                    GetWindowRect(hwndTree, &rcTree);
                    window_reset(hwnd, hwndTree, rc, rcTree);
                    if (!shader_view)
                    {
                        cindex = lparam->hti_index;
                        show_txlist_lv(hwnd,hwndList, dbptr->txeptr[lparam->hti_index], cxChar, cyChar, rc, rcTree);
                    }
                    else
                    {
                        cindex = lparam->shader_index;
                        int table_index = lparam->hti_index;
                        if (tvi.hItem == new_db->shaders[cindex].table_members[table_index].hti_table_childs[2]) // texture list empty 
                        {
                            show_txlist_lv(hwnd, hwndList, *new_db->shaders[cindex].table_members[table_index].txeptr_null, cxChar, cyChar, rc, rcTree);

                        }
                        else if (tvi.hItem == new_db->shaders[cindex].table_members[table_index].hti_table_childs[3]) // texture list 2 
                        {
                            show_txlist_lv(hwnd, hwndList, *new_db->shaders[cindex].table_members[table_index].txeptr_2, cxChar, cyChar, rc, rcTree);
                        }
                    }
                    break;
                }
                case ID_VS_HLSL_SHADER:   // VERTEX SHADER
                {
                    GetClientRect(hwnd, &rc);
                    GetWindowRect(hwndTree, &rcTree);
                    window_reset(hwnd, hwndTree, rc, rcTree);
                    if (!shader_view)
                    {
                        cindex = lparam->hti_index;
                        int test = lprm_childs->lprms_counter;
                        show_vs_lv(hwnd, hwndList, dbptr->vs_shader[cindex], cxChar, cyChar, rc, rcTree);
                    }
                    else
                    {
                        cindex = lparam->shader_index;
                        int table_index = lparam->hti_index;
                        show_vs_lv(hwnd, hwndList, *new_db->shaders[cindex].table_members[table_index].vs_shader, cxChar, cyChar, rc, rcTree);

                    }
                    break;
                }
                case ID_PS_HLSL_SHADER: // PIXEL SHADER
                {
                    GetClientRect(hwnd, &rc);
                    GetWindowRect(hwndTree, &rcTree);
                    window_reset(hwnd, hwndTree, rc, rcTree);
                    if (!shader_view)
                    {
                        cindex = lparam->hti_index;
                        show_ps_lv(hwnd, hwndList, dbptr->ps_shader[cindex], cxChar, cyChar, rc, rcTree);
                    }
                    else
                    {
                        cindex = lparam->shader_index;
                        int table_index = lparam->hti_index;
                        show_ps_lv(hwnd, hwndList, *new_db->shaders[cindex].table_members[table_index].ps_shader, cxChar, cyChar, rc, rcTree);

                    }
                    break;
                }
                case 5: // TABLE
                {
                    GetClientRect(hwnd, &rc);
                    GetWindowRect(hwndTree, &rcTree);
                    window_reset(hwnd, hwndTree, rc, rcTree);
                    if (!shader_view)
                    {
                        cindex = lparam->hti_index;
                        show_table_lv(hwnd, hwndList, dbptr->table[cindex], cxChar, cyChar, rc, rcTree);
                    }
                    else
                    {
                        show_table_lv(hwnd,hwndList, new_db->shaders[lparam->shader_index].table_members[lparam->hti_index], cxChar, cyChar, rc,rcTree);

                    }
                    break;
                }
                case 6: // VERTEX BUFFER
                {

                    GetClientRect(hwnd, &rc);
                    GetWindowRect(hwndTree, &rcTree);
                    window_reset(hwnd, hwndTree, rc, rcTree);
                    if (!shader_view)
                    {
                        cindex = lparam->hti_index;
                        show_vxbfr_lv(hwnd, hwndList, dbptr->vx_buffers[cindex], cxChar, cyChar, rc, rcTree);
                    }
                    else
                    {
                        cindex = lparam->hti_index;
                        show_vxbfr_lv(hwnd, hwndList, *new_db->mesh_refs[cindex].vx_buffer, cxChar, cyChar,rc, rcTree);

                    }
                    break;
                }
                case 7: // STRING
                {
                    GetClientRect(hwnd, &rc);
                    GetWindowRect(hwndTree, &rcTree);
                    window_reset(hwnd, hwndTree, rc, rcTree);
                    if (!shader_view)
                    {
                        HTREEITEM parent = TreeView_GetParent(hwndTree, tvi.hItem);
                        int i = 0;
                        while (parent != dbptr->str_parent[i]) {
                            i++;
                        }
                        cindex = lparam->hti_index;
                        show_str_lv(hwnd, hwndList, dbptr->strings[i][cindex],i, cxChar, cyChar, rcTree);

                    }
                    break;
                }
                case ID_SHADER_NODE: // SHADER NODE 
                {
                    window_reset(hwnd, hwndTree, rc, rcTree);
                    show_shdr_lv(hwnd,hwndList, &new_db->shaders[lparam->hti_index], cxChar, cyChar, rcTree);
                    break;
                }
                case ID_MESH_NODE:
                {
                    cindex = lparam->hti_index;
                    show_str_lv(hwnd, hwndList, new_db->mesh_refs[cindex], i, cxChar, cyChar, rcTree);
                    break;
                }
                case ID_SYSTEMS_N_NODE:
                    cindex = lparam->hti_index;
                    show_str_lv(hwnd, hwndList, new_db->systems_ref[cindex], 1, cxChar, cyChar, rcTree);
                    break;
                case 106:
                {
                    window_reset(hwnd, hwndTree, rc, rcTree);
                    clean_and_renew_generic(hwnd, hwnd_container, 1);
                    break;
                }
                case 107:
                {
                    window_reset(hwnd, hwndTree, rc, rcTree);
                    clean_and_renew_generic(hwnd, hwnd_container, 1);
                    break;
                }

                }
                break;
            break;
            }
        }
        break;
        }
	return FALSE;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        // All painting occurs here, between BeginPaint and EndPaint.
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(hwnd, &ps);
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}



int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow) {
    WNDCLASSEX wc;
    WNDCLASSEX wc_assetc;
    HWND hwnd;
    MSG Msg;
    hIcon = LoadImage(0, _T("logof.ico"), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 1;
    wc.hInstance = hInst;
    wc.hIcon = 0;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"Window Class";
    wc.hIconSm = 0;

    wc_assetc.cbSize = sizeof(WNDCLASSEX);
    wc_assetc.style = 0;
    wc_assetc.lpfnWndProc = assetc_wndproc;
    wc_assetc.cbClsExtra = 0;
    wc_assetc.cbWndExtra = 1;
    wc_assetc.hInstance = hInst;
    wc_assetc.hIcon = LoadIcon(wc_assetc.hInstance, L"logof.ico");
    wc_assetc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc_assetc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    wc_assetc.lpszMenuName = NULL;
    wc_assetc.lpszClassName = assetc_classname;
    wc_assetc.hIconSm = LoadIcon(wc_assetc.hInstance, L"logof.ico");


    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, L"Window Registration Failed!", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    if (!RegisterClassEx(&wc_assetc))
    {
        MessageBox(NULL, L"Window Registration Failed!", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"Window Class",
        L"Database Explorer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
        NULL, NULL, hInst, NULL);
    if (hwnd == NULL)
    {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    ShowWindow(hwnd, ncmdshow);
    UpdateWindow(hwnd);
    InitCommonControls();
    for (int i = 0; i < 90; ++i) {
        blank[i] = (' ');
    }

    while (GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
