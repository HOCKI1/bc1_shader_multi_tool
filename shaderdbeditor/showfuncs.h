#pragma once
#include "framework.h"
#include "structs.h"

//void showtxedata(HWND hwnd, database* dbptr, int index, int cxChar, int cyChar, RECT rc, RECT rcTree, txe_hwnds*& txehwndlocal);
void show_vs(HWND hwnd, bc2_vs &vs_shader, int cxChar, int cyChar, RECT rcTree, generic_hwnds*& hwnd_container);
void show_ps(HWND hwnd, bc2_ps &ps_shader, int cxChar, int cyChar, RECT rcTree, generic_hwnds*& hwnd_container);
//void show_txe(HWND hwnd, database* dbptr, int index, int cxChar, int cyChar, RECT rc, RECT rcTree, generic_hwnds*& hwnd_container);
void show_table(HWND hwnd, table_parent& table, int cxChar, int cyChar, RECT rcTree, generic_hwnds*& hwnd_container);
void show_vbuffer(HWND hwnd, vxbuffer& vx_buffers, int cxChar, int cyChar, RECT rcTree, generic_hwnds*& hwnd_container);
void show_str(HWND hwnd, string_ref& strings, int index_i, int cxChar, int cyChar, RECT rcTree, generic_hwnds*& hwnd_container);
void show_txlist(HWND hwnd, TxE &txeptr, int cxChar, int cyChar, RECT rc, RECT rcTree, generic_hwnds*& hwnd_container);
void show_shdr(HWND hwnd, shader_struct* shader, int cxChar, int cyChar, RECT rcTree, generic_hwnds*& hwnd_container);

void hex_to_string(char* id, char* src, int size);
