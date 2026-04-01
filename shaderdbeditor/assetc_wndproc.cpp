#include "assetc.h"
#include "assetc_structs.h"
#include "assetc_meshset_funcs.h"
#include "assetc_export_funcs.h"
#include "assetc_import_funcs.h"
#include "assetc_meshdata_funcs.h"
#include "exportfuncs.h"
#include "Resource.h"

int base_x, base_y;
WNDPROC ASSETC_WND_PROC=0;
HFONT hfont;
HWND hwndAssetc;
HWND hwndExportTab = NULL;
HWND hwndImportTab = NULL;
//lod_wnd_sec* lod_sections = nullptr;
export_settings export_data = { 0 };
import_settings import_data[5] = { 0 };
TCITEMA** import_tabs;
export_fopen_box open_box = { 0 };
mem_pool* ac_structs_pool;
mem_pool* ac_data_pool;
mem_pool* ac_import_pool;
int active_import_tab;

// window to manage 3d objects. export, import and third option to edit mesh set files (.rigidmeshset, compositemeshset, etc) 

int cxChar, cyChar;

LRESULT CALLBACK assetc_wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)                                                                                                                                                                        
{        
    static HDC hdc;
    static char temp_file_name[400];
    static OPENFILENAMEA ofn;
    static HMENU main_options = { 0 };
    static HMENU meshset_sub_menu = { 0 };
    static HMENU meshdata_sub_menu = { 0 };
    static TEXTMETRICA tm;
    static int active_lod;
    static NONCLIENTMETRICS metrics;
    static uint32_t active_func = 0;
    static bool meshdata_btn_enable = FALSE;
    static bool insert_import_tabs = TRUE;
    hwndAssetc = hwnd;
    hwndAsset = hwnd;
    switch (message)
    {
    case WM_CREATE:
    {
        if (hIcon) {
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

            SendMessage(GetWindow(hwnd, GW_OWNER), WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(GetWindow(hwnd, GW_OWNER), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        }
        ac_structs_pool = (mem_pool*)malloc(sizeof(mem_pool));
        ac_data_pool = (mem_pool*)malloc(sizeof(mem_pool));
        ac_import_pool = (mem_pool*)malloc(sizeof(mem_pool));
        ac_data_pool->pool = (char*)malloc(sizeof(char)*16000000);
        ac_data_pool->current_pos = 0;
        memset(ac_data_pool->pool, 0, 16000000);
        ac_structs_pool->pool = (char*)malloc(sizeof(char) * 1000000);
        ac_structs_pool->current_pos = 0;
        memset(ac_structs_pool->pool,0, 1000000);
        ac_import_pool->pool = (char*)malloc(sizeof(char) * 16000000);
        ac_import_pool->current_pos = 0;
        memset(ac_import_pool->pool, 0, 16000000);
        hdc = GetDC(hwnd);
        ShowWindow(hwnd, 1);
        GetTextMetricsA(hdc, &tm);
        cxChar = tm.tmAveCharWidth;
        cyChar = tm.tmHeight + tm.tmExternalLeading;
        base_x = cxChar;
        base_y = cyChar;
        meshdata_sub_menu = init_meshdata_menu(hwnd);
        meshset_sub_menu = init_meshset_menu(hwnd);
        main_options = init_menu(hwnd, meshset_sub_menu, meshdata_sub_menu);
        SetMenu(hwnd, main_options);
        DrawMenuBar(hwnd);
        NONCLIENTMETRICS metrics;
        metrics.cbSize = sizeof(NONCLIENTMETRICS);
        ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS),
            &metrics, 0);
        hfont = ::CreateFontIndirect(&metrics.lfMessageFont);
        SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
        return 0;
    }
    case WM_SETFOCUS:
        SetFocus(hwnd);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {      
        case ID_ASSETC_EXPORT_MESH:
        {
            if (!ac_data_pool->pool)
            {
                ac_data_pool->pool = new char[16000000];
                ac_data_pool->current_pos = 0;
                ZeroMemory(ac_data_pool->pool, 16000000);
            }
            else
            {
                ZeroMemory(ac_data_pool->pool, 16000000);
            }
            if (active_func == ID_ASSETC_IMPORT_MESH)
            {
                ShowWindow(hwndImportTab, 0);
            }
            active_func = ID_ASSETC_EXPORT_MESH;
            if (!hwndExportTab)
            {
                hwndExportTab = init_tab(hwnd);
                ASSETC_WND_PROC = (WNDPROC)SetWindowLongPtrW(hwndExportTab, GWLP_WNDPROC, (LONG_PTR)assetc_export_wndproc);
            }
            UpdateWindow(hwndExportTab);
            ShowWindow(hwndExportTab, 1);
            if (!open_box.btn_wrapper)
            {
                init_open_meshset(hwnd, hwndExportTab, &open_box, cxChar, cyChar, hfont);
            }
            return 0;
        }
        case ID_ASSETC_IMPORT_MESH:
        {
            if (!ac_import_pool->pool)
            {
                ac_import_pool->pool = new char[16000000];
                ac_import_pool->current_pos = 0;
                ZeroMemory(ac_import_pool->pool, 16000000);
            }
            else
            {
                ZeroMemory(ac_import_pool->pool, 16000000);
            }
            if (active_func == ID_ASSETC_EXPORT_MESH)
            {
                ShowWindow(hwndExportTab, 0);
            }
            active_func = ID_ASSETC_IMPORT_MESH;
            if (!hwndImportTab)
            {
                hwndImportTab = init_import_tab(hwnd);
            }
            UpdateWindow(hwndImportTab);
            ShowWindow(hwndImportTab, 1);
            if (insert_import_tabs)
            {
                import_tabs = init_import_tabs(hwndImportTab, import_data);
                insert_import_tabs = false;
            }
            if (!import_data[0].generic_settings_hwnds.wrapper)
            {
                init_generic_settings_wnd(hwndImportTab, nullptr, &import_data[0], cxChar, cyChar);
            }
            ShowWindow(import_data[0].tab_wrapper, 1);
            ShowWindow(import_data[0].generic_settings_hwnds.wrapper, 1);
            import_data[0].mesh_type = 0;
            TabCtrl_SetCurSel(hwndImportTab,0);
            return 0;
        }
        case ID_ASSETC_CREATE_SET:
        {
            char* txt_path = (char*)malloc(sizeof(char) * 384);
            ofn = open_dialog(hwnd, "Text File (*.txt)\0*.txt\0\0");
            uint32_t type,n_lines;
            if (GetOpenFileNameA(&ofn))
            {
                strncpy(txt_path, ofn.lpstrFile, 384);
                char* buffer = (char*)malloc(sizeof(char) * 262144);
                char* file_buffer = (char*)malloc(sizeof(char) * 262144);
                char* line_ptrs = (char*)malloc(sizeof(char*) * 8192);
                char** lines;
                memset(buffer, 0, 65536);
                memset(file_buffer, 0, 65536);
                lines = parse_txt_generic(&txt_path[0], buffer, line_ptrs,&n_lines);
                meshset_txt_to_binary(lines, file_buffer, &txt_path[0], n_lines);
                free(line_ptrs);
                free(buffer);
                free(file_buffer);
            }
            free(txt_path);
            return 0;
        }
        case ID_ASSETC_EXTRACT_SET:
        {
            char meshset_path[384] = { 0 };
            char* dot_index;
            meshset* temp_meshset = nullptr;
            ofn = open_dialog(hwnd, "All files (*.*)\0*.*\0(*.rigidmeshset)\0*.rigidmeshset\0(*.compositemeshset)\0*.compositemeshset\0(*.treemeshset)\0*.treemeshset\0\0 ");
            uint32_t type;
            if (GetOpenFileNameA(&ofn))
            {
                strncpy(&meshset_path[0], ofn.lpstrFile, 384);
                temp_meshset = meshset_handler(&meshset_path[0], &type);
                dot_index = strstr(&meshset_path[0], ".");
                strcpy(dot_index, "_Meshset.txt");
                meshset_to_txt(temp_meshset, &meshset_path[0], type);
            }
            return 0;
        }
        case ID_ASSETC_EXTRACT_MESHDATA_INFO:
        {
            char* meshdata_path = (char*)malloc(sizeof(char) * 384);
            char* dot_index;
            meshdata* temp_meshdata = nullptr;
            ofn = open_dialog(hwnd, "MeshData (*.meshdata*)\0*.meshdata*\0\0 ");
            uint32_t type;
            if (GetOpenFileNameA(&ofn))
            {
                strncpy(&meshdata_path[0], ofn.lpstrFile, 384);
                FILE* meshdata = fopen(&meshdata_path[0], "r+b");
                int fsize;
                fseek(meshdata, SEEK_SET, SEEK_END);
                fsize = ftell(meshdata);
                fseek(meshdata, 0, SEEK_SET);
                char* file_buffer = (char*)malloc(sizeof(char) * fsize);
                fread(file_buffer, 1, fsize, meshdata);
                fclose(meshdata);
                temp_meshdata = meshdata_reader(file_buffer);
                dot_index = strstr(&meshdata_path[0], ".");
                strcpy(dot_index, "_Meshdata.txt");
                meshdata_to_txt(temp_meshdata, &meshdata_path[0]);
                free(file_buffer);
            }
            free(meshdata_path);
            return 0;
        }
        }  
    case WM_CLOSE:
    {
        SendMessage(hwndMain,WM_COMMAND, ID_ASSETC_ENABLE_MENU_BTN,0);
        if (ac_structs_pool->pool)
        {
           free(ac_structs_pool->pool);
           free(ac_structs_pool);
        }       
        if (ac_data_pool->pool)
        {
            free(ac_data_pool->pool);
            free(ac_data_pool);
        }
        if (ac_import_pool->pool)
        {
            free(ac_import_pool->pool);
            free(ac_import_pool);
        }
        DestroyWindow(hwnd);
        DestroyWindow(hwndImportTab);
        DestroyWindow(hwndExportTab);
        hwndExportTab = nullptr;
        hwndImportTab = nullptr;
        ZeroMemory(&open_box, sizeof(export_fopen_box));
        ZeroMemory(&import_data[0], sizeof(import_settings) * 5);
        insert_import_tabs = true;
        return 0;
    }
    case WM_NOTIFY:
    {
        LPNMHDR lpnmh2 = (LPNMHDR)lParam;
        TVHITTESTINFO ht = { 0 };

        switch (lpnmh2->idFrom)
        {
        case ID_ASSETC_IMPORT_MESH_TAB:
        {
            switch (lpnmh2->code)
            {
            case NM_CLICK:
            {
                return 0;
            }
            case TCN_SELCHANGING:
            {
                int i = TabCtrl_GetCurSel(hwndImportTab);
                ShowWindow(import_data[i].tab_wrapper,SW_HIDE);
                return FALSE;
            }
            case TCN_SELCHANGE:
            {
                int i = TabCtrl_GetCurSel(hwndImportTab);
                import_data[i].mesh_type = i;
                active_import_tab = i;
                if (!import_data[i].generic_settings_hwnds.wrapper)
                {
                    init_generic_settings_wnd(hwndImportTab, nullptr, &import_data[i], cxChar, cyChar);
                }
                switch (i)
                {
                case 0:
                    import_data[i].mesh_type = RIGID_MESH;
                    break;
                case 1:
                    import_data[i].mesh_type = COMPOSITE_MESH;
                    break;
                case 2:
                    import_data[i].mesh_type = TREE_MESH;
                    break;
                case 3:
                    import_data[i].mesh_type = SKINNED_MESH;
                    break;
                case 4:
                    import_data[i].mesh_type = SKINNED_MESH;
                    break;
                }
                ShowWindow(import_data[i].tab_wrapper, SW_SHOW);
                break;
            }
            }
            break;
        }
        }
        return 0;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW+1));
        EndPaint(hwnd, &ps);
        return 0;
    }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}


HMENU init_menu(HWND hwnd, HMENU &submenu, HMENU &meshdatamenu)
{
    HMENU main_options = CreateMenu();
    if (!main_options)
    {
        return 0;
    }
    if (!AppendMenuA(main_options, MF_STRING | MF_POPUP, (INT)meshdatamenu, "&MeshData"))
    {
        return 0;
    }
    if (!AppendMenuA(main_options, MF_STRING | MF_POPUP, (INT)submenu, "&MeshSet Tools"))
    {
        return 0;
    }
    if (!AppendMenuA(main_options, MF_STRING | MF_ENABLED, ID_ASSETC_IMPORT_MESH, "&Import Mesh"))
    {
        return 0;
    }
    if (!AppendMenuA(main_options, MF_STRING | MF_ENABLED, ID_ASSETC_EXPORT_MESH, "&Export Mesh"))
    {
        return 0;
    }
    return main_options;
}

HMENU init_meshset_menu(HWND hwnd)
{
    HMENU hSubMenu = CreatePopupMenu();
    if (!AppendMenuA(hSubMenu, MF_STRING | MF_ENABLED, ID_ASSETC_CREATE_SET, "&Compile"))
    {
        return 0;
    }
    if (!AppendMenuA(hSubMenu, MF_STRING | MF_ENABLED, ID_ASSETC_EXTRACT_SET, "&Decompile"))
    {
        return 0;
    }
    return hSubMenu;
}

HMENU init_meshdata_menu(HWND hwnd)
{
    HMENU hSubMenu = CreatePopupMenu();
    if (!AppendMenuA(hSubMenu, MF_STRING | MF_ENABLED, ID_ASSETC_EXTRACT_MESHDATA_INFO, "&Extract info"))
    {
        return 0;
    }
    return hSubMenu;
}

OPENFILENAMEA open_dialog(HWND hwnd, const char* filters)
{
    OPENFILENAMEA ofn;
    char szFileName[384] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = filters;
    ofn.lpstrFile = &szFileName[0];
    ofn.nMaxFile = 384;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = 0;
    return ofn;
}

char* show_file_name_only(char* src)
{

    char* start = strstr(src, "\\");
    int i = 0;
    int index = 0;
    while (start != 0)
    {
        index = i;
        i = (start - src);
        start = strstr(&src[i+1], "\\");
    }
    return &src[i+1];
}



char* assetc_guid_convert( wchar_t*src, mem_pool* pool)
{
    char guid_text[34];
    char value[3] = { 0,0,0 };
    char guid[16];
    uint8_t byte = 0;
    wcstombs(guid_text, src, 34);
    for (int i = 0; i < 16; i++)
    {

        value[0] = guid_text[i * 2];
        value[1] = guid_text[i * 2 + 1];
        byte = strtol(value, nullptr, 16);
        guid[i] = (char)byte;
    }
    char* result = (char*)memcpy(&pool->pool[pool->current_pos], guid, 16);
    pool->current_pos += 17;
    return result;
}
