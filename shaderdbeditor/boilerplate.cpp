#include "boilerplate.h"
#include "Resource.h"

const int txe_type = ID_TEXTURELIST;
const int vs_type = ID_VS_HLSL_SHADER;
const int ps_type = ID_PS_HLSL_SHADER;
const int table_type = 5;
const int vx_type = 6;
const int str_type = 7;
const int vs_index_pos = 14;
const int ps_index_pos = 13;
const int table_index_pos = 14;
//const int table_index_pos = 14;


void boilerplate_txe(HWND hwnd, HWND hwndTree, database*& dbptr, int &treesize, generic_hwnds*& hwnd_container, RECT rc,RECT rcTree)
{
    wchar_t* txebasen = const_cast<wchar_t*>(L"Texture List ");
    shader_view = FALSE;
    window_reset(hwnd, hwndTree, rc, rcTree);
    lprm_childs->lprms_counter = 0;
    lprm_parents->lprms_counter = 0;
    if ((hwnd_container) && (hwnd_container->destroy))
    {
        clean_and_renew_generic(hwnd, hwnd_container, 20);
    }
    wchar_t txename[20];
    show_generic_tree(hwndTree, dbptr, treesize, txe_type, txename, dbptr->ntxe, 13, txebasen);
    return;
}

void boilerplate_vs(HWND hwnd, HWND hwndTree, database*& dbptr, int &treesize, generic_hwnds*& hwnd_container, RECT rc, RECT rcTree)
{
    wchar_t* vsbasen = const_cast<wchar_t*>(L"Vertex Shader ");
    shader_view = FALSE;
    window_reset(hwnd, hwndTree, rc, rcTree);
    lprm_childs->lprms_counter = 0;
    lprm_parents->lprms_counter = 0;
    if ((hwnd_container) && (hwnd_container->destroy))
    {
        clean_and_renew_generic(hwnd, hwnd_container, 4);
    }
    wchar_t vsname[21];
    show_generic_tree(hwndTree, dbptr, treesize, vs_type, vsname, dbptr->n_vs_shader, vs_index_pos, vsbasen);
    return;
}

void boilerplate_ps(HWND hwnd, HWND hwndTree, database*& dbptr, int &treesize, generic_hwnds*& hwnd_container, RECT rc, RECT rcTree)
{
    wchar_t* psbasen = const_cast<wchar_t*>(L"Pixel Shader ");
    shader_view = FALSE;
    window_reset(hwnd, hwndTree, rc, rcTree);
    lprm_childs->lprms_counter = 0;
    lprm_parents->lprms_counter = 0;
    if ((hwnd_container) && (hwnd_container->destroy))
    {
        clean_and_renew_generic(hwnd, hwnd_container, 4);
    }
    wchar_t psname[20];
    show_generic_tree(hwndTree, dbptr, treesize, ps_type, psname, dbptr->n_ps_shader, ps_index_pos, psbasen);
    return;
}


void boilerplate_table(HWND hwnd, HWND hwndTree, database*& dbptr, int &treesize, generic_hwnds*& hwnd_container, RECT rc, RECT rcTree)
{
    wchar_t* tablebasen = const_cast<wchar_t*>(L"Table Element ");
    shader_view = FALSE;
    window_reset(hwnd, hwndTree, rc, rcTree);
    lprm_childs->lprms_counter = 0;
    lprm_parents->lprms_counter = 0;
    if ((hwnd_container) && (hwnd_container->destroy))
    {
        clean_and_renew_generic(hwnd, hwnd_container, 13);
    }
    wchar_t psname[20];
    show_generic_tree(hwndTree, dbptr, treesize, table_type, psname, dbptr->n_table_elements, table_index_pos, tablebasen);
    return;
}

void boilerplate_vbuffer(HWND hwnd, HWND hwndTree, database*& dbptr, int &treesize, generic_hwnds*& hwnd_container, RECT rc, RECT rcTree)
{
    shader_view = FALSE;
    window_reset(hwnd, hwndTree, rc, rcTree);
    lprm_childs->lprms_counter = 0;
    lprm_parents->lprms_counter = 0;
    if ((hwnd_container) && (hwnd_container->destroy))
    {
        clean_and_renew_generic(hwnd, hwnd_container, 13);
    }
    show_generic_tree(hwndTree, dbptr, treesize, vx_type, NULL, dbptr->n_vx_buffers, 0, NULL);
    return;
}

void boilerplate_strings(HWND hwnd, HWND hwndTree, database*& dbptr, int &treesize, generic_hwnds*& hwnd_container, RECT rc, RECT rcTree)
{
    shader_view = FALSE;
    window_reset(hwnd, hwndTree, rc, rcTree);
    lprm_childs->lprms_counter = 0;
    lprm_parents->lprms_counter = 0;
    if ((hwnd_container) && (hwnd_container->destroy))
    {
        clean_and_renew_generic(hwnd, hwnd_container, 1);
    }
    show_strings_tree(hwndTree, dbptr, treesize, str_type);
    return;
}