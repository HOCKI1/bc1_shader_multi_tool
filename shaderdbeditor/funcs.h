#pragma once 
#include "structs.h"
#include "framework.h"
#include <stdint.h>

// winfuncs.cpp 
bool show_generic_tree(HWND hwndTV, database* dbptr, int& treesize, int type, wchar_t* name, uint32_t loop, int charpos, wchar_t* txebasen);
bool clean_and_renew_generic(HWND hwnd, generic_hwnds*& hwnd_container, int new_size);
bool window_no_border(HWND hwnd, HWND& hwndtxe, RECT rcTree, int cyChar, int cxChar, int i, char* wndtext);
bool windowcreator_new(HWND hwnd, HWND& hwndout, int xpos, int ypos, int xsize, int ysize);
bool hide_tree(HWND hwndTV, int& treesize);
bool show_strings_tree(HWND hwndTV, database* dbptr, int &treesize, int type);
bool show_shaders(HWND hwndTV, database_export*& new_db, int& treesize);
void window_reset(HWND hwnd, HWND hwndTree, RECT rc, RECT rcTree);
void create_wnd_and_txt(HWND hwnd, HDC hdc, RECT rcTree, int cxChar, int cyChar, int hwnd_index, generic_hwnds*& hwnd_container, const char* title, const char* wnd_text);
void hex_to_string(char* id, char* src, int size);
BOOL insert_item_lv(HWND hWndListView, char* value, int item_count);
BOOL set_col_val(HWND hWndListView, LVITEMW &lv_item, uint32_t index, uint32_t col_index);
OPENFILENAMEA get_output_file_name(HWND hwnd, const char* prefix);
OPENFILENAMEA open_db(HWND hwnd);

// treewindow.cpp
bool CreateTree(HWND hwnd, HWND& hwndTree, RECT& rc);


// menu.cpp
void MenuTool(HWND hwnd, HMENU& hMenu, HMENU& hSubMenu, HMENU  &hDebugMenu);
void enable_menu(HWND hwnd, HMENU& hMenu, HMENU& hSubMenu, HMENU &hDebugMenu);

// dbfuncs.cpp
database* dbfirstload(void* dbfile, uint32_t* ptr_end);
void vbufferslook(void*& dbfile, database*& dbptr);
void string_pass(void*& dbfile, database*& dbptr, uint32_t elements, string_ref*& str_ref, int pass);
shader_struct* build_shaders(void* dbfile, database*& dbptr, string_ref* &systems_ref);
database_export* export_db_init(database*& dbptr, void* dbfile);
TxE* txe_individual_assign(char* src);

// datagrid_procs.cpp
WNDCLASSEX set_grid_wndclass(HINSTANCE hInstance);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
long _stdcall ListViewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
long _stdcall EditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


