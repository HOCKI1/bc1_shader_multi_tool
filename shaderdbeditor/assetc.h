#pragma once
#include "framework.h"
#include "structs.h"

LRESULT CALLBACK assetc_wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

extern vxbuffer* assetc_vx_buffers;
extern uint32_t assetc_n_vx_buffers;