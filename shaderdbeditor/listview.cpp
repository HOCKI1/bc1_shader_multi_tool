#define _CRT_SECURE_NO_WARNINGS
#include "listview.h"
#include "Resource.h"
#include "editfuncs.h"

long _stdcall ListViewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_LBUTTONDOWN:
    {
        if (hwndEdit != NULL) 
        { 
            SendMessage(hwndEdit, WM_KILLFOCUS, 0, 0); 
        };
        LVITEM lv_item;
        LVHITTESTINFO itemclicked;
        long x, y;
        x = (long)LOWORD(lParam);
        y = (long)HIWORD(lParam);
        itemclicked.pt.x = x;
        itemclicked.pt.y = y;
        int lResult = ListView_SubItemHitTest(hwnd, &itemclicked);
        lv_item.mask = LVIF_PARAM;
        lv_item.iItem = iItem;
        lv_item.iSubItem = iSubItem;
        ListView_GetItem(hwnd, &lv_item);
        if (itemclicked.iSubItem > 0)
        {
            if (lResult != -1)
            {
                wchar_t* temp = new wchar_t[500];
                RECT subitemrect;
                ListView_GetSubItemRect(hwnd, itemclicked.iItem, itemclicked.iSubItem, LVIR_BOUNDS, &subitemrect);
                int height = subitemrect.bottom - subitemrect.top;
                int width = subitemrect.right - subitemrect.left;
              //  if (itemclicked.iSubItem == 0) { width = width / 4; };
                hwndEdit = CreateWindowA( "EDIT", "",
                    WS_CHILD | WS_VISIBLE | ES_WANTRETURN| ES_AUTOHSCROLL,
                    subitemrect.left+3, subitemrect.top+2, width, height, hwnd, 0, GetModuleHandle(NULL), NULL);
                if (hwndEdit == NULL)
                {
                    MessageBoxA(hwnd, "Could not create edit box.", "Error", MB_OK | MB_ICONERROR);
                }
                iItem = itemclicked.iItem;
                iSubItem = itemclicked.iSubItem;
                HFONT defaultFont = CreateFont(16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
                SendMessage(hwndEdit, WM_SETFONT, WPARAM(defaultFont), TRUE);
                ListView_GetItemText(hwndList,iItem,1,temp,500);
                SetWindowTextW(hwndEdit,temp);
                SetFocus(hwndEdit);
                ORIGINAL_EDIT_PROC = (WNDPROC)SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)EditProc);
                delete[] temp;
            }
        }       
        return 0;
        break;
    }   
   /* case LVM_SETITEMTEXT:
        
        ListView_SetItemText(hwnd,)
        break;*/
    }
    return CallWindowProc(PARENT_WND_PROC, hwnd, message, wParam, lParam);
}




long _stdcall EditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_KILLFOCUS:
    {
        LV_DISPINFO lvDispinfo;
        ZeroMemory(&lvDispinfo, sizeof(LV_DISPINFO));
        lvDispinfo.hdr.hwndFrom = hwnd;
        lvDispinfo.hdr.idFrom = GetDlgCtrlID(hwnd);
        lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
        lvDispinfo.item.mask = LVIF_TEXT;
        lvDispinfo.item.iItem = iItem;
        lvDispinfo.item.iSubItem = iSubItem;
        lvDispinfo.item.pszText = NULL;
        wchar_t szEditText[10];
        GetWindowText(hwnd, szEditText, 10);
        lvDispinfo.item.pszText = szEditText;
        lvDispinfo.item.cchTextMax = lstrlen(szEditText);
        SendMessage(GetParent(GetDlgItem(hwndList, ID_LISTVIEW)), WM_NOTIFY, (WPARAM)ID_LISTVIEW, (LPARAM)&lvDispinfo); //the LV ID and the LVs Parent window's HWND
        DestroyWindow(hwnd);
        break;
    }
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_RETURN:
            //Get Text&Set Text
            LVITEM lv_item;
            lv_item.mask = LVIF_PARAM;
            lv_item.iItem = iItem;
            lv_item.iSubItem = iSubItem;
            ListView_GetItem(hwndList,&lv_item);
            lv_item_data* test = (lv_item_data*)lv_item.lParam;
            if (test->edit_enable)
            {
                LPTSTR buffer = new TCHAR[300];
                GetWindowText(hwnd, buffer, 300);
                int wcs_length = wcslen(buffer);
                char temp[300];
                wcstombs(temp, buffer, wcs_length + 1);
                edit_central(test, temp);
                delete[] buffer;
            }
            SendMessage(hwndEdit, WM_KILLFOCUS, 0, 0);
            break;
        }
        break;

    }

    return CallWindowProc((WNDPROC)ORIGINAL_EDIT_PROC, hwnd, message, wParam, lParam);
}


HWND create_lv(HWND hwnd, RECT rc, RECT rcTree)
{
    HWND hwndList = CreateWindow(WC_LISTVIEW,
        L"",
        WS_CHILD | LVS_REPORT | LVS_EDITLABELS,
        rcTree.right - rcTree.left,  // x position 
        rc.top,  // y position
        rc.right - (rcTree.right - rcTree.left), // Button width
        rc.bottom, // Button height
        hwnd,
        (HMENU)ID_LISTVIEW,
        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
        NULL);
    return hwndList;
}


BOOL init_columns(HWND hWndListView, int list_width)
{
    LVCOLUMN lvc;
    int iCol;
    WCHAR* names[3] = { const_cast<WCHAR*>(L"Property Type"),const_cast<WCHAR*>(L"Value"),const_cast<WCHAR*>(L"Hex Data")};
    // Initialize the LVCOLUMN structure.
    // The mask specifies that the format, width, text,
    // and subitem members of the structure are valid.
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    // Add the columns.
    for (iCol = 0; iCol < 2; iCol++)
    {
        lvc.iSubItem = iCol;
        lvc.pszText = names[iCol];
        if (iCol == 0)
        {
            lvc.cx = 200;
        }
        else
        {
            lvc.cx = list_width -200;
        }
                      // Width of column in pixels.
        lvc.fmt = LVCFMT_LEFT;  
        // Insert the columns into the list view.
        if (ListView_InsertColumn(hWndListView, iCol, &lvc) == -1)
            return FALSE;
    }

    return TRUE;
}
