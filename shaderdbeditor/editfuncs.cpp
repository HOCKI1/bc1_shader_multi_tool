#define _CRT_SECURE_NO_WARNINGS
#include "editfuncs.h"

// most of the functions that write to the pool are here, except for import_db which is on another file

bool check_guid_correctness(char* new_val);
bool check_u32_correctness(char* new_val);
bool check_hex_correctness(char* new_val, int str_length);
char* text_update(char* new_val);
char* guid_update(char* new_val);
char* hex_update(char* new_val, int str_length);
uint32_t* u32_update(char* new_val);


void clone_txe(table_parent &src_table, int index, database_export* new_db)
{
	if (index == 0)
	{
		TxE* src_element = src_table.txeptr_null;
		char* result = (char*)memcpy(file_edit_pool.pool + file_edit_pool.current_pos, src_element->eptr, src_element->esize);
		file_edit_pool.current_pos += src_element->esize+1;
		src_table.txeptr_null = txe_individual_assign(result);
		*src_table.txe_ref1 = new_db->n_txe;
	}
	else
	{
		TxE* src_element = src_table.txeptr_2;
		char* result = (char*)memcpy(file_edit_pool.pool + file_edit_pool.current_pos, src_element->eptr, src_element->esize);
		file_edit_pool.current_pos += src_element->esize+1;
		src_table.txeptr_2 = txe_individual_assign(result);
		*src_table.txe_ref2 = new_db->n_txe;
	}
	uint32_t new_ceiling = new_db->n_txe+1;
	uint32_t* ptr = &new_db->n_txe;
	*ptr = new_ceiling;
	return;
}


void edit_central(lv_item_data* item_data, char* new_value)
{
	wchar_t buffer[350];
	char* dummy;
	int int_dummy;
	ZeroMemory(buffer, 350 * sizeof(wchar_t));
	void* item_data_ptr = nullptr;
	void* aux_ptr = nullptr;
	switch (item_data->type)
	{
	case ID_TEXTURELIST:
		switch (item_data->subtype)
		{
		case TEXTURE_NAME:
			item_data_ptr = (txe_e*)(item_data->block_data);
			((txe_e*)(item_data_ptr))->name = text_update(new_value);
			mbstowcs(buffer, ((txe_e*)(item_data_ptr))->name, strlen(((txe_e*)(item_data_ptr))->name) + 1);
			ListView_SetItemText(hwndList, iItem, iSubItem, (LPWSTR)buffer);
			return;
		case PARAM_NAME:
			item_data_ptr = (txe_p*)(item_data->block_data);
			((txe_p*)(item_data_ptr))->name = text_update(new_value);
			mbstowcs(buffer, ((txe_p*)(item_data_ptr))->name, strlen(((txe_p*)(item_data_ptr))->name) + 1);
			ListView_SetItemText(hwndList, iItem, iSubItem, (LPWSTR)buffer);
			return;
		}
	case MEMBER_TYPE:
		item_data_ptr = (txe_t*)(item_data->block_data);
		((txe_t*)(item_data_ptr))->name = text_update(new_value);
		mbstowcs(buffer, ((txe_t*)(item_data_ptr))->name, strlen(((txe_t*)(item_data_ptr))->name) + 1);
		ListView_SetItemText(hwndList, iItem, iSubItem, (LPWSTR)buffer);
		return;
		break;
	case ID_VS_HLSL_SHADER:
		switch (item_data->subtype)
		{
		case TYPE_GUID:
			item_data_ptr = (bc2_vs*)item_data->struct_ptr;
			if (!check_guid_correctness(new_value))
			{
				return;
			}
			else
			{
				((bc2_vs*)(item_data_ptr))->vsid = guid_update(new_value);
				mbstowcs(buffer, new_value, 34);
				ListView_SetItemText(hwndList, iItem, iSubItem, (LPWSTR)buffer);
				return;
			}
		case TYPE_UINT32:
			if (check_u32_correctness(new_value))
			{
				item_data_ptr = item_data->block_data;
				*((uint32_t*)(item_data_ptr)) = (uint32_t)u32_update(new_value);
				mbstowcs(buffer, new_value, 11);
				ListView_SetItemText(hwndList, iItem, iSubItem, (LPWSTR)buffer);
			}
			else
			{
				MessageBox(NULL, L"Value exceeds MAX UINT32", L"Error",
					MB_ICONEXCLAMATION | MB_OK);
				return;
			}
			break;
		}
	case ID_PS_HLSL_SHADER:
		switch (item_data->subtype)
		{
		case TYPE_GUID:
			item_data_ptr = (bc2_ps*)item_data->struct_ptr;
			if (!check_guid_correctness(new_value))
			{
				return;
			}
			else
			{
				((bc2_ps*)(item_data_ptr))->psid = guid_update(new_value);
				mbstowcs(buffer, new_value, 34);
				ListView_SetItemText(hwndList, iItem, iSubItem, (LPWSTR)buffer);
				return;
			}
		case TYPE_UINT32:
			if (check_u32_correctness(new_value))
			{
				item_data_ptr = item_data->block_data;
				*((uint32_t*)(item_data_ptr)) = (uint32_t)u32_update(new_value);
				mbstowcs(buffer, new_value, 11);
				ListView_SetItemText(hwndList, iItem, iSubItem, (LPWSTR)buffer);
			}
			else
			{
				MessageBox(NULL, L"Value exceeds MAX UINT32", L"Error",
					MB_ICONEXCLAMATION | MB_OK);
				return;
			}
			break;			
		}
		break;
	case ID_TABLE:
	{
		switch (item_data->subtype)
		{
		case TYPE_GUID:
			item_data_ptr = item_data->block_data;
			if (!check_guid_correctness(new_value))
			{
				return;
			}
			else
			{
				*(uint32_t*)(item_data_ptr) = (uint32_t)guid_update(new_value);
				mbstowcs(buffer, new_value, 34);
				ListView_SetItemText(hwndList, iItem, iSubItem, (LPWSTR)buffer);
				if (shader_view)
				{
					HWND main_wnd = GetParent(hwndTree);
					SendMessage(main_wnd, WM_COMMAND, ENABLE_UPD_BTN, 0);
				}
				return;
			}
			break;
		case TYPE_UINT32:
			if (check_u32_correctness(new_value))
			{
				item_data_ptr = item_data->block_data;
				*((uint32_t*)(item_data_ptr)) = (uint32_t)u32_update(new_value);
				mbstowcs(buffer, new_value, 11);
				ListView_SetItemText(hwndList, iItem, iSubItem, (LPWSTR)buffer);
			}
			else
			{
				MessageBox(NULL, L"Value exceeds MAX UINT32", L"Error",
					MB_ICONEXCLAMATION | MB_OK);
				return;
			}
			break;
		case TYPE_HEX:
			item_data_ptr = item_data->block_data;
			if (!check_hex_correctness(new_value,80))
			{
				return;
			}
			else
			{
				*(uint32_t*)(item_data_ptr) = (uint32_t)hex_update(new_value,40);
				mbstowcs(buffer, new_value, 81);
				ListView_SetItemText(hwndList, iItem, iSubItem, (LPWSTR)buffer);
				return;
			}
			break;
		}
		break; 
	}
	case ID_STR_MESH_REF:
		if (item_data->subtype == TYPE_GUID)
		{
			item_data_ptr = item_data->block_data;
			if (!check_guid_correctness(new_value))
			{
				return;
			}
			else
			{
				dummy = guid_update(new_value);
				int_dummy = seek_guid(new_db->n_mesh_refs, new_db->mesh_refs, dummy);
				if (int_dummy == -1)
				{
					MessageBox(NULL, L"Vertex Buffer not found. Incorrect GUID", L"Error",
						MB_ICONEXCLAMATION | MB_OK);
					return;
				}
				else {
					*(uint32_t*)(item_data_ptr) = (uint32_t)dummy;
					item_data_ptr = item_data->struct_ptr;
					((string_ref*)(item_data_ptr))->vx_buffer = new_db->mesh_refs[int_dummy].vx_buffer;
					mbstowcs(buffer, new_value, 34);
					ListView_SetItemText(hwndList, iItem, iSubItem, (LPWSTR)buffer);
					if (shader_view) 
					{
						HWND main_wnd = GetParent(hwndTree);
						SendMessage(main_wnd, WM_COMMAND, ENABLE_UPD_BTN, 0);
					}
				}
				return;
			}		
		}
		else if (item_data->subtype == TYPE_TEXT)
		{
			 item_data_ptr = item_data->block_data;
	       	 *(uint32_t*)item_data_ptr = (uint32_t)text_update(new_value);
		 	 mbstowcs(buffer, new_value, 350);
			 ListView_SetItemText(hwndList, iItem, iSubItem, (LPWSTR)buffer);
			 if (shader_view)
			 {
				 HWND main_wnd = GetParent(hwndTree);
				 SendMessage(main_wnd, WM_COMMAND, ENABLE_UPD_BTN, 0);
			 }
			 return;
		}
		break;
	case ID_SHADER_NODE:
		switch (item_data->subtype)
		{
		case TYPE_GUID:
			item_data_ptr = item_data->block_data;
			if (!check_guid_correctness(new_value))
			{
				return;
			}
			else
			{
				*(uint32_t*)(item_data_ptr) = (uint32_t)guid_update(new_value);
				aux_ptr = item_data->struct_ptr;
				mbstowcs(buffer, new_value, 34);
				ListView_SetItemText(hwndList, iItem, iSubItem, (LPWSTR)buffer);
				int_dummy = ((shader_struct*)(aux_ptr))->tables_ref->n_table_refs;
				((shader_struct*)(aux_ptr))->tables_ref->id = (char*)item_data_ptr;
				for (int i = 0; i < ((shader_struct*)(aux_ptr))->tables_ref->n_table_refs; i++)
				{
				  ((shader_struct*)(aux_ptr))->table_members[i].child->shader_id = *(char**)item_data_ptr;
				}
				/*if (shader_view)
				{
					HWND main_wnd = GetParent(hwndTree);
					SendMessage(main_wnd, WM_COMMAND, ENABLE_UPD_BTN, 0);
				}*/
			}
			break;
		case TYPE_TEXT:
			item_data_ptr = item_data->block_data;
			*(uint32_t*)item_data_ptr = (uint32_t)text_update(new_value);
			mbstowcs(buffer, new_value,350);
			ListView_SetItemText(hwndList, iItem, iSubItem, (LPWSTR)buffer);
			if (shader_view)  
			{
				HWND main_wnd = GetParent(hwndTree);
				SendMessage(main_wnd, WM_COMMAND, ENABLE_UPD_BTN, 0);
			}
			break; 
		}
		break;
	}
	return; 
}



// all of the edit functions patch the pointer to the new value in the edit pool 

char* text_update(char* new_val) 
{
	int str_length = strlen(new_val);
	char* result = (char*)memcpy(file_edit_pool.pool + file_edit_pool.current_pos, new_val, str_length + 1);
	file_edit_pool.current_pos += str_length + 1;
	return result;
}


uint32_t* u32_update(char* new_val)
{
	uint32_t int_val = strtoul(new_val,nullptr,10);
	uint32_t* result = (uint32_t*)memcpy(file_edit_pool.pool + file_edit_pool.current_pos, &int_val, 4);
	return result;
}


char* hex_update(char* new_val, int str_length)
{
	char value[3] = { 0,0,0 };
	char hex[100];
	uint8_t byte = 0;
	for (int i = 0; i < str_length; i++)
	{

		value[0] = new_val[i * 2];
		value[1] = new_val[i * 2 + 1];
		byte = strtol(value, nullptr, 16);
		hex[i] = (char)byte;
	}
	char* result = (char*)memcpy(file_edit_pool.pool + file_edit_pool.current_pos, hex, str_length);
	file_edit_pool.current_pos += str_length+1;
	return result;
}


char* guid_update(char* new_val)
{
	char value[3] = { 0,0,0 };
	char guid[16];
	uint8_t byte = 0;
	for (int i = 0; i < 16; i++)
	{

		value[0] = new_val[i * 2];
		value[1] = new_val[i * 2 + 1];
		byte = strtol(value, nullptr, 16);
		guid[i] = (char)byte;
	}
	char* result = (char*)memcpy(file_edit_pool.pool + file_edit_pool.current_pos, guid, 16);
	file_edit_pool.current_pos += 17;
	return result;
}

bool check_hex_correctness(char* new_val,int str_length)
{
	char temp[350];
	memcpy(temp, new_val, str_length+1);
	int length = strlen(temp);
	bool number, capital, lower;
	if (length % 2 == 0)
	{
		for (int i = 0; i < str_length; i++)
		{
			number = (new_val[i] > 47 && new_val[i] < 58);
			capital = (new_val[i] > 64 && new_val[i] < 71);
			lower = (new_val[i] > 96 && new_val[i] < 103);
			if (!(number || (capital || lower)))
			{
				MessageBox(NULL, L"Invalid hex character", L"Error",
					MB_ICONEXCLAMATION | MB_OK);
				return FALSE;
			}
		}
		return TRUE;
	}
	else
	{
		MessageBox(NULL, L"Character count incorrect", L"Error",
			MB_ICONEXCLAMATION | MB_OK);
		return FALSE;
	}
}

bool check_guid_correctness(char* new_val)
{
	char temp[350];
	memcpy(temp, new_val, 33);
	bool number, capital, lower;
	if (strlen(temp) == 32)
	{
		for (int i = 0; i < 32; i++)
		{
			number = (new_val[i] > 47 && new_val[i] < 58);
			capital = (new_val[i] > 64 && new_val[i] < 71);
			lower = (new_val[i] > 96 && new_val[i] < 103);
			if (!(number || (capital || lower)))
			{
				MessageBox(NULL, L"Invalid hex character", L"Error",
					MB_ICONEXCLAMATION | MB_OK);
				return FALSE;
			}
		}
		return TRUE;
	}
	else
	{
		MessageBox(NULL, L"Character count incorrect", L"Error",
			MB_ICONEXCLAMATION | MB_OK);
		return FALSE;
	}
}

bool check_u32_correctness(char* new_val)
{
	char max_u32[] = "4294967295";
	int length = strlen(new_val);
	if (length > 10)
	{
		return FALSE;
	}
	else if (length == 10)
	{
		for (int i = 0; i < 10; i++)
		{
			if (new_val[i] > 47 && new_val[i] < 58)
			{
				if (new_val[i] > max_u32[i])
				{
					return FALSE;
				}

			}
			else
			{
				return FALSE;
			}

		}
		return TRUE;
	}
	else
	{
		return TRUE;
	}
}