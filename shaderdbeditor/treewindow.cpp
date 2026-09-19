#define _CRT_SECURE_NO_WARNINGS
#include "funcs.h"
#include "Resource.h"


bool CreateTree(HWND hwnd, HWND &hwndTree, RECT &rc) {

    hwndTree = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        WC_TREEVIEW,
        0,
        WS_CHILD | WS_VISIBLE | TVS_CHECKBOXES| TVS_HASLINES,
        0, 0, rc.right/3.2, rc.bottom,
        hwnd, (HMENU)ID_TREEVIEW, NULL, NULL);
	if (hwndTree == NULL)
	{
		MessageBox(NULL, L"Window Creation Failed!", L"Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return false;
	}   
	return true ;
}
