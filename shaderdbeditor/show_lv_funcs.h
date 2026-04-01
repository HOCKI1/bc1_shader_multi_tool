#pragma once
#include "funcs.h"
#include <sstream>
#include "Resource.h"
#include "framework.h"

void show_txlist_lv(HWND hwnd, HWND hwndList, TxE& txeptr, int cxChar, int cyChar, RECT rc, RECT rcTree);
void show_vs_lv(HWND hwnd, HWND hwndList, bc2_vs& vs_shader, int cxChar, int cyChar, RECT rc, RECT rcTree);
void show_ps_lv(HWND hwnd, HWND hwndList, bc2_ps& ps_shader, int cxChar, int cyChar, RECT rc, RECT rcTree);
void show_table_lv(HWND hwnd, HWND hwndList, table_parent& table, int cxChar, int cyChar, RECT rc, RECT rcTree);
void show_vxbfr_lv(HWND hwnd, HWND hwndList, vxbuffer& vx_buffer, int cxChar, int cyChar, RECT rc, RECT rcTree);
void show_str_lv(HWND hwnd, HWND hwndList, string_ref& strings, int index_i, int cxChar, int cyChar, RECT rcTree);
void show_shdr_lv(HWND hwnd, HWND hwndList, shader_struct* shader, int cxChar, int cyChar, RECT rcTree);
void hex_to_wchar(wchar_t* dst, char* src, int size);
