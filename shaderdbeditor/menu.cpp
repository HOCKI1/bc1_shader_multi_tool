#include "Resource.h"
#include "funcs.h"

void MenuTool(HWND hwnd,HMENU &hMenu,HMENU &hSubMenu,HMENU& hDebugMenu) {

    hMenu = CreateMenu();
    hSubMenu = CreatePopupMenu();
    AppendMenuA(hSubMenu, MF_STRING, ID_FILE_OPEN, "Open");
    AppendMenuA(hSubMenu, MF_STRING | MF_DISABLED, ID_SAVE_DB, "Save");
    AppendMenuA(hSubMenu, MF_STRING, ID_FILE_EXIT, "E&xit");
    AppendMenuA(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "&File");
    hSubMenu = CreatePopupMenu();
    AppendMenuA(hSubMenu, MF_STRING, ID_EXPORT_DB, "&Export Selected as Database");
    AppendMenuA(hSubMenu, MF_STRING, ID_EXPORT_STR, "&Export Selected Mesh String");
    AppendMenuA(hSubMenu, MF_STRING, ID_EXPORT_HLSL, "&(debug)Export Selected HLSL Shaders");
    AppendMenuA(hMenu, MF_STRING | MF_DISABLED, ID_IMPORT_DB, "&Import DB");
    AppendMenuA(hMenu, MF_STRING | MF_POPUP | MF_DISABLED, (UINT)hSubMenu, "&Export");
    AppendMenuA(hMenu, MF_STRING | MF_DISABLED, ID_SHADER_VIEW, "&Shaders");
    hDebugMenu = CreatePopupMenu();
    AppendMenuA(hMenu, MF_STRING | MF_DISABLED, ID_UPDATE_LV, "&Update View");
    AppendMenuA(hMenu, MF_STRING | MF_POPUP | MF_DISABLED, (UINT)hDebugMenu, "&Debug Views");
    AppendMenuA(hMenu, MF_STRING | MF_DISABLED, ID_ASSETC_START, "&Asset Creator");
    AppendMenuA(hDebugMenu, MF_STRING | MF_DISABLED, ID_TXE_VIEW, "&Texture List");
    AppendMenuA(hDebugMenu, MF_STRING | MF_DISABLED, ID_D3DPARAMS1_VIEW, "&D3D Params");
    AppendMenuA(hDebugMenu, MF_STRING | MF_DISABLED, ID_VERTEXSHADER_VIEW, "&Vertex Shaders");
    AppendMenuA(hDebugMenu, MF_STRING | MF_DISABLED, ID_PIXELSHADER_VIEW, "&Pixel Shaders");
    AppendMenuA(hDebugMenu, MF_STRING | MF_DISABLED, ID_TABLES_VIEW, "&Tables");
    AppendMenuA(hDebugMenu, MF_STRING | MF_DISABLED, ID_VBUFFERS_VIEW, "&Vertex Buffers");
    AppendMenuA(hDebugMenu, MF_STRING | MF_DISABLED, ID_STRINGS_VIEW, "&String references");

    SetMenu(hwnd, hMenu);

	return;
}

void enable_menu(HWND hwnd, HMENU &hMenu, HMENU &hSubMenu, HMENU &hDebugMenu)
{
    EnableMenuItem(hMenu, (UINT)hSubMenu, MF_ENABLED);
    EnableMenuItem(hMenu, (UINT)hDebugMenu, MF_ENABLED);
    EnableMenuItem(hMenu, ID_SHADER_VIEW, MF_ENABLED);
    EnableMenuItem(hMenu, ID_TXE_VIEW, MF_ENABLED);
    EnableMenuItem(hMenu, ID_IMPORT_DB, MF_ENABLED);
    EnableMenuItem(hMenu, ID_VERTEXSHADER_VIEW, MF_ENABLED);
    EnableMenuItem(hMenu, ID_PIXELSHADER_VIEW, MF_ENABLED);
    EnableMenuItem(hMenu, ID_TABLES_VIEW, MF_ENABLED);
    EnableMenuItem(hMenu, ID_VBUFFERS_VIEW, MF_ENABLED);
    EnableMenuItem(hMenu, ID_STRINGS_VIEW, MF_ENABLED);
    EnableMenuItem(hMenu, ID_ASSETC_START, MF_ENABLED);
    return;
}

