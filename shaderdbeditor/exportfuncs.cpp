#include "exportfuncs.h"
#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

#define MAX_ITEMS 3500


typedef struct
{
	uint32_t final_index;
	uint32_t original_val;
	uint32_t bytes_to_write;
	char* object;
}map_item;

typedef struct
{
	bool exists;
	uint32_t index;
} item_existence;


// export selected as database 
// first go through every shader and write the indexes of selected ones
// second = count the items of each section (txe, vs shaders, etc) and check for repeated ones 
// third = $$$


void fill_txe_item1(table_parent*& tps, map_item*& txe_storage, uint32_t& txe_count, char* txe_buffer);
void fill_txe_item2(table_parent*& tps, map_item*& txe_storage, uint32_t& txe_count, char* txe_buffer);
void table_child_buf_wr(table_child* src, char* dst, uint32_t t_count);
uint32_t fill_vs_item(table_parent*& tps, map_item*& vs_storage, uint32_t& vs_count, char* vs_buffer, map_item*& txe_storage, uint32_t& txe_count,char* txe_buffer);
uint32_t fill_ps_item(table_parent*& tps, map_item*& vs_storage, uint32_t& vs_count, char* vs_buffer, map_item*& txe_storage, uint32_t& txe_count,char* txe_buffer);


int acc_txe = 0;
int acc_s_txe = 0;
int txe_bfr_pos = 4;


void write_txe(TxE* src, char* txe_buffer)
{
	uint32_t size_dif = txe_bfr_pos;
	char* beginning = txe_buffer + txe_bfr_pos;
	memcpy(txe_buffer + txe_bfr_pos, src->eptr, 32 * sizeof(char));
	txe_bfr_pos += 32;
	for (int i = 0; i < *src->ntxt; i++)
	{
		*(uint32_t*)(txe_buffer + txe_bfr_pos) = *src->txtptrs[i].index;
		txe_bfr_pos += 4;
		memcpy(txe_buffer + txe_bfr_pos, src->txtptrs[i].name, 136 * sizeof(char));
		txe_bfr_pos += 136;
	}
	for (int i = 0; i < *src->nparams; i++)
	{
		memcpy(txe_buffer + txe_bfr_pos, src->paramptrs[i].name, 56 * sizeof(char));
		txe_bfr_pos += 56;
	}
	for (int i = 0; i < *src->ntypes; i++)
	{
		memcpy(txe_buffer + txe_bfr_pos, src->typeptrs[i].name, 40 * sizeof(char));
		txe_bfr_pos += 40;
	}
	size_dif = src->esize - (txe_bfr_pos - size_dif);
	char* dummy = (char*)src->eptr + *src->unknowndata;
	memcpy(txe_buffer + txe_bfr_pos, dummy, size_dif * sizeof(char));
	txe_bfr_pos += size_dif;
	return;
}


item_existence check_existence(map_item* container, uint32_t target, uint32_t cont_size)
{
	item_existence status; 
	status.exists = FALSE;
	status.index = -1;
	for (uint32_t i = 0; i < cont_size; i++)
	{
		if (container[i].original_val == target)
		{
			status.exists = TRUE;
			status.index = i;
			return status;	 
		}
	}
	return status;
}


void update_storage(uint32_t original_val,uint32_t bytes_to_write,void* object, map_item* &storage,uint32_t index)
{
	storage[index].final_index = index;
	storage[index].original_val = original_val;
	storage[index].object = (char*)object;
	storage[index].bytes_to_write = bytes_to_write;
	return;
}


uint32_t write_str_ref(string_ref* src, char* dst)
{
	uint32_t size = strlen(src->name) + 1;
	memcpy(dst, src->name,size);
	memcpy((dst + size), src->guid, 16);
	return (size+16);
}

bool seek_val(uint32_t* arr, uint32_t arr_size, uint32_t target)
{
	for (int i = 0; i<arr_size;i++)
	{
		if (arr[i] == target)
		{	
			return TRUE;
		}
	}
	return FALSE;
}

void write_buffer(char* buffer, uint32_t count, map_item* storage,FILE* output)
{
	int acc = 0;
	char* dummy = buffer;
	uint32_t total_items = count;
	uint32_t* items_write = (uint32_t*)dummy;
	*items_write = total_items;
	dummy += 4;
	acc = (dummy - buffer);
	for (uint32_t i = 0; i < count; i++)
	{
		memcpy((buffer + acc), storage[i].object, storage[i].bytes_to_write);
		acc += storage[i].bytes_to_write;
	}
	fwrite(buffer, 1, acc, output);
	return;
}

uint32_t seek_guid_vx(uint32_t src_cont_size, vxbuffer* src_container, char* target)
{
	int success = 0;
	for (int i = 0; i < src_cont_size; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			if (src_container[i].vxbuffer_id[j] != target[j])
			{
				break;
			}
			else {
				success++;
			}
		}
		if (success == 16)
		{
			return i;
		}
		success = 0;
	}
	return -1;
}

uint32_t seek_guid(uint32_t src_cont_size, string_ref* src_container, char* target)
{
	int success = 0;
	for (int i = 0; i < src_cont_size; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			if (src_container[i].guid[j] != target[j])
			{
				break;
			}
			else {
				success++;
			}
		}
		if (success == 16)
		{
			return i;
		}
		success = 0;
	}
	return -1;
}

void copy_vals(table_parent* &dst, table_parent *src, char* table_src)
{
	dst->element_id = table_src;
	table_src += 16;
	dst->unknown1 = (uint32_t*)(table_src);
	dst->unknown2 = (uint32_t*)(table_src + 4);
	dst->temp_hash = (uint32_t*)(table_src + 8);
	dst->null_int = (uint32_t*)(table_src + 12);
	dst->vs_ref = (uint32_t*)(table_src + 16);
	dst->ps_ref = (uint32_t*)(table_src + 20);
	dst->txe_ref1 = (uint32_t*)(table_src + 24);
	dst->txe_ref2 = (uint32_t*)(table_src + 28);
	dst->ps_shader = src->ps_shader;
	dst->vs_shader = src->vs_shader;
	dst->txeptr_null = src->txeptr_null;
	dst->txeptr_2 = src->txeptr_2;
	dst->child = src->child;
	return; 
}


void export_hlsl(database_export* new_db,char* dst_folder) // finish this func 
{
	char* vertex_bfr = (char*)malloc(20000 * sizeof(char));
	char* pixel_bfr = (char*)malloc(20000 * sizeof(char));
	char* temp_name[400];
	FILE* dummy;
	std::string folder(dst_folder);
	folder += "\\";
	std::string vs_base = "Vertex_Shader_";
	std::string ps_base = "Pixel_Shader_";
	std::string obf = "_obf";
	std::string extension = ".hlsl";

	for (uint32_t i = 0; i < new_db->n_shaders; i++)
	{
		for (uint32_t j = 0; j < new_db->shaders[i].tables_ref->n_table_refs; j++)
		{
			if (new_db->shaders[i].table_members[j].vs_shader->lparam_data.selected)
			{
				std::string index_s = std::to_string(i);
				std::string index_t = std::to_string(j);
				dummy = fopen((folder+vs_base+ index_s +"_"+ index_t+extension).c_str(), "w+b");
				fwrite(new_db->shaders[i].table_members[j].vs_shader->shader, 1, *new_db->shaders[i].table_members[j].vs_shader->vs_size, dummy);
				fclose(dummy);
				new_db->shaders[i].table_members[j].vs_shader->lparam_data.selected = FALSE;
			}
			if (new_db->shaders[i].table_members[j].ps_shader->lparam_data.selected)
			{
				std::string index_s = std::to_string(i);
				std::string index_t = std::to_string(j);
				dummy = fopen((folder + ps_base + index_s + "_" + index_t + extension).c_str(), "w+b");
				fwrite(new_db->shaders[i].table_members[j].ps_shader->shader, 1, *new_db->shaders[i].table_members[j].ps_shader->ps_size, dummy);
				fclose(dummy);
				new_db->shaders[i].table_members[j].ps_shader->lparam_data.selected = FALSE;
			}
		}
	}

	free(vertex_bfr);
	free(pixel_bfr);
	return;
}

void export_str(HWND hwnd, database_export* new_db, char* output_name)
{
	char* str_bfr = (char*)malloc(1000000 * sizeof(char));
	char* vx_bfr = (char*)malloc(12000 * sizeof(char));
	char* buffers[2] = { vx_bfr,str_bfr };
	FILE* output = fopen(output_name, "wb");
	char* str_blck_ptr = str_bfr + 4;
	char* vx_bfr_ptr = vx_bfr + 4;
	uint32_t* dummy_int = nullptr;
	uint32_t n_vx_buffers = 0;
	uint32_t n_mesh_str = 0;
	uint32_t vx_bfrs_idx[150];
	uint32_t buffers_size[2];
	for (uint32_t i = 0; i < new_db->n_mesh_refs; i++)
	{
		if (new_db->mesh_refs[i].selected)
		{
			str_blck_ptr += write_str_ref(&new_db->mesh_refs[i], str_blck_ptr);
			int z = seek_guid_vx(new_db->n_vx_buffers, new_db->vx_buffers, new_db->mesh_refs[i].vx_buffer->vxbuffer_id);
			if (!seek_val(vx_bfrs_idx, 150, z))
			{
				memcpy(vx_bfr_ptr + (n_vx_buffers * 89), new_db->vx_buffers[z].vxbuffer_id, 89);
				vx_bfrs_idx[n_vx_buffers] = z;
				n_vx_buffers++;
			}
			n_mesh_str++;
		}
	}
	dummy_int = (uint32_t*)str_bfr;
	*dummy_int = n_mesh_str;
	dummy_int = (uint32_t*)vx_bfr;
	*dummy_int = n_vx_buffers;
	buffers_size[0] = (n_vx_buffers*89) + 4;
	buffers_size[1] = str_blck_ptr - str_bfr;
	for (uint32_t i = 0; i < 2; i++)
	{
		fwrite(buffers[i], 1, buffers_size[i], output);
	}
	fclose(output);
	free(str_bfr);
	free(vx_bfr);
	return;
}

void export_db(HWND hwnd, database_export* new_db, char* output_name, int force_export) 
{
	char* txe_buffer = (char*)malloc(3200000 * sizeof(char)); 
	char* vs_buffer = (char*)malloc(10000000 * sizeof(char));
	char* ps_buffer = (char*)malloc(20000000 * sizeof(char));
	char* table1_buffer = (char*)malloc(960000 * sizeof(char));
	char* table2_buffer = (char*)malloc(1760000 * sizeof(char));
	char* refs_buffer = (char*)malloc(1760000 * sizeof(char));
	char* strings_block = (char*)malloc(1500000 * sizeof(char));
	char* vx_buffers = (char*)malloc(16020 * sizeof(char));
	char* buffers[9] = {txe_buffer,new_db->d3dparams_block,vs_buffer,ps_buffer,table1_buffer,table2_buffer,vx_buffers,strings_block,refs_buffer};
	FILE* output = fopen(output_name, "wb");
	char magic[5] = { 0x78,0x00,0x00,0x00,0x00};
	char test[5];
	//bool result;
	if (output == NULL)
	{
		snprintf(test, 5, "%d", errno);
		MessageBoxA(hwnd, test, "ERROR", MB_OK);
	}   
	map_item* txe_storage = new map_item[MAX_ITEMS];
	map_item* vs_storage = new map_item[MAX_ITEMS];
	map_item* ps_storage = new map_item[MAX_ITEMS];
	uint16_t* table_write_idx;
	uint32_t count = 0;
	uint32_t txe_count = 0;
	uint32_t ps_count = 0;
	uint32_t vs_count = 0;
	uint32_t tables_count = 0;
	uint32_t n_systems = 0;
	uint32_t system_indexes[16]; 
	uint32_t* dummy_int;
	char* table_buf_ptr = (table1_buffer + 4);
	char* table2_buf_ptr = (table2_buffer + 4);
	char* refs_buf_ptr = (refs_buffer+ 4);
	char* vs_buffer_ptr = (vs_buffer + 4);
	char* ps_buffer_ptr = (ps_buffer + 4);
	char* str_blck_ptr = (strings_block + 4);
	char* temp_table = nullptr;
	uint32_t buffers_size[9] = {0,0,0,0,0,0,0,0,0};
	item_existence status;
	table_parent* tps;
	table_parent* tps_temp = new table_parent;
	for (uint32_t i = 0; i < new_db->n_shaders; i++) // NUMBER OF SHADERS (TOTAL)
	{
		if (new_db->shaders[i].lparam_data.selected || force_export) // IF SELECTED GO AHEAD
		{
			str_blck_ptr += write_str_ref(new_db->shaders[i].shader_ref, str_blck_ptr);
			memcpy(refs_buf_ptr, new_db->shaders[i].shader_ref->guid, 16);
			dummy_int = (uint32_t*)(refs_buf_ptr+16);
			*dummy_int = new_db->shaders[i].tables_ref->n_table_refs;
			refs_buf_ptr += 20;
			for (uint32_t k = 0; k < new_db->shaders[i].tables_ref->n_table_refs; k++) // PROCESS SELECTED SHADER TABLES 
			{
				table_write_idx = (uint16_t*)refs_buf_ptr;
				*table_write_idx = tables_count; 
				refs_buf_ptr += 2;
				tps = &new_db->shaders[i].table_members[k];
				temp_table = (char*)(memcpy((table_buf_ptr + (tables_count*48)), tps->element_id, 48));
				copy_vals(tps_temp,tps,temp_table);
				fill_txe_item1(tps_temp, txe_storage, txe_count, txe_buffer);
				fill_txe_item2(tps_temp, txe_storage, txe_count, txe_buffer);
				buffers_size[2] += fill_vs_item(tps_temp,vs_storage,vs_count, vs_buffer_ptr,txe_storage, txe_count,txe_buffer);
				buffers_size[3] += fill_ps_item(tps_temp, ps_storage, ps_count, ps_buffer_ptr, txe_storage, txe_count,txe_buffer);
				table_child_buf_wr(tps_temp->child, table2_buf_ptr + (tables_count*88),tables_count);
				if (new_db->shaders[i].table_members[k].child->has_parent)  // IF SHADER IS UNDER "SYSTEMS" SAVE THE INDEX. WE CAN WRITE THEM LATER AS ORDER DOESNT MATTER
				{
					int z = seek_guid(new_db->n_systems, new_db->systems_ref, new_db->shaders[i].table_members[k].child->system_id);
					if (!seek_val(system_indexes,16,z))
					{
						system_indexes[n_systems] = z;
						n_systems++;
					}
				}
				tables_count++;
			}
			count++;
		}
	}
	dummy_int = (uint32_t*)refs_buffer;
	*dummy_int = count;
	dummy_int = (uint32_t*)strings_block;
	*dummy_int = count;
	dummy_int = (uint32_t*)str_blck_ptr;
	*dummy_int = n_systems;
	str_blck_ptr += 4;
	for (uint32_t i = 0; i<n_systems;i++)
	{	
		str_blck_ptr += write_str_ref(&new_db->systems_ref[system_indexes[i]], str_blck_ptr);
	}
	uint32_t n_vx_buffers = 0;
	uint32_t n_mesh_str = 0;
	uint32_t vx_bfrs_idx[150];
	char* vx_bfr_ptr = vx_buffers + 4;
	char* mesh_str_ptr = str_blck_ptr;
	str_blck_ptr += 4;
	for (uint32_t i = 0; i < new_db->n_mesh_refs; i++)
	{
		if (new_db->mesh_refs[i].selected)
		{
			str_blck_ptr += write_str_ref(&new_db->mesh_refs[i], str_blck_ptr);
			int z = seek_guid_vx(new_db->n_vx_buffers, new_db->vx_buffers, new_db->mesh_refs[i].vx_buffer->vxbuffer_id);
			if (!seek_val(vx_bfrs_idx, 150, z))
			{
				memcpy(vx_bfr_ptr+(n_vx_buffers*89),new_db->vx_buffers[z].vxbuffer_id,89);
				vx_bfrs_idx[n_vx_buffers] = z;
				n_vx_buffers++;
			}
			n_mesh_str++;
		}	
	}
	dummy_int = (uint32_t*)txe_buffer;
	*dummy_int = txe_count;
	dummy_int = (uint32_t*)mesh_str_ptr;
	*dummy_int = n_mesh_str;
	dummy_int = (uint32_t*)vx_buffers;
	*dummy_int = n_vx_buffers;
	buffers_size[0] = txe_bfr_pos;
	buffers_size[1] = new_db->d3dparams_size;
	buffers_size[2] += 4;
	buffers_size[3] += 4;
	buffers_size[4] = (tables_count * 48) + 4;
	buffers_size[5] = (tables_count * 88) + 4;
	buffers_size[6] = (n_vx_buffers * 89) + 4;
	buffers_size[7] = str_blck_ptr-strings_block;
	buffers_size[8] = refs_buf_ptr - refs_buffer;
	dummy_int = (uint32_t*)vs_buffer;
	*dummy_int = vs_count;
	dummy_int = (uint32_t*)ps_buffer;
	*dummy_int = ps_count;
	dummy_int = (uint32_t*)table1_buffer;
	*dummy_int = tables_count;
	dummy_int = (uint32_t*)table2_buffer;
	*dummy_int = tables_count;
	uint32_t acc = 0;
	fwrite(&magic[0], 1, 5, output);
	for (uint32_t i = 0; i<9;i++)
	{
		fwrite(buffers[i],1,buffers_size[i],output);
	}
	snprintf(test, 5, "%d", count);
	fclose(output);
	free(txe_buffer);
	free(table1_buffer);
	free(table2_buffer);
	free(ps_buffer);
	free(vs_buffer);
	free(refs_buffer);
	free(strings_block);
	free(vx_buffers);
	delete tps_temp;
	delete[] txe_storage;
	delete[] vs_storage;
	delete[] ps_storage;
	return;
}


// copies the shader to the buffer and patches it there. otherwise it messes up the lookup when it reads a patched shader again 
uint32_t fill_vs_item(table_parent*& tps, map_item*& vs_storage, uint32_t& vs_count, char* vs_buffer, map_item*& txe_storage, uint32_t& txe_count, char* txe_buffer)
{
	item_existence status;
	item_existence status2;
	static char* write_pos = vs_buffer;
	char* dst_shader;
	char* dst_shader_start;
	uint32_t* txe_index;
	uint32_t txe_idx_dist;
	if (vs_count > 0)
	{
		status = check_existence(vs_storage, *tps->vs_ref, vs_count);  // first check if the shader has been indexed (reading from the table) and update the index storage (or not)
		if (!status.exists)
		{
			update_storage(*tps->vs_ref, tps->vs_shader->total_size, tps->vs_shader->vsid, vs_storage, vs_count);
			*tps->vs_ref = vs_storage[vs_count].final_index;  // patch table with new index
			dst_shader = (char*)memcpy(write_pos, vs_storage[vs_count].object, vs_storage[vs_count].bytes_to_write);
			write_pos += vs_storage[vs_count].bytes_to_write;
			dst_shader_start = dst_shader;
			txe_idx_dist = ((char*)tps->vs_shader->txeindex - tps->vs_shader->vsid);
			dst_shader += txe_idx_dist;
			txe_index = (uint32_t*)dst_shader; // get the texture list index, referenced by the shader 
			status2 = check_existence(txe_storage, *txe_index, txe_count);
			if (!status2.exists)
			{
				update_storage(*txe_index, tps->vs_shader->txeptr->esize, tps->vs_shader->txeptr->eptr, txe_storage, txe_count);
				write_txe(tps->vs_shader->txeptr, txe_buffer);
				*txe_index = txe_storage[txe_count].final_index;
				txe_count++;
			}
			else
			{
				*txe_index = txe_storage[status2.index].final_index;
			}
			vs_count++;
			return vs_storage[vs_count - 1].bytes_to_write;
		}
		else
		{
			*tps->vs_ref = vs_storage[status.index].final_index; // patch table with new index
			return 0;
		}
	}
	else
	{
		update_storage(*tps->vs_ref, tps->vs_shader->total_size, tps->vs_shader->vsid, vs_storage, 0);
		*tps->vs_ref = vs_storage[0].final_index;
		vs_count++;
		dst_shader = (char*)memcpy(write_pos, vs_storage[0].object, vs_storage[0].bytes_to_write);
		write_pos += vs_storage[0].bytes_to_write;
		dst_shader_start = dst_shader;
		txe_idx_dist = ((char*)tps->vs_shader->txeindex - tps->vs_shader->vsid);
		dst_shader += txe_idx_dist;
		txe_index = (uint32_t*)dst_shader; // get the texture list index, referenced by the shader 
		status2 = check_existence(txe_storage, *txe_index, txe_count);
		if (!status2.exists)
		{
			update_storage(*txe_index, tps->vs_shader->txeptr->esize, tps->vs_shader->txeptr->eptr, txe_storage, txe_count);
			write_txe(tps->vs_shader->txeptr, txe_buffer);
			*txe_index = txe_storage[txe_count].final_index;
			txe_count++;
		}
		else
		{
			*txe_index = txe_storage[status2.index].final_index;
		}
		return vs_storage[0].bytes_to_write;
	}
}

uint32_t fill_ps_item(table_parent*& tps, map_item*& vs_storage, uint32_t& vs_count, char* vs_buffer, map_item*& txe_storage, uint32_t& txe_count, char* txe_buffer)
{
	item_existence status;
	item_existence status2;
	static char* write_pos = vs_buffer;
	static uint32_t acc_size = 0;
	char* dst_shader;
	char* dst_shader_start;
	uint32_t* txe_index;
	uint32_t txe_idx_dist;
	if (vs_count > 0)
	{
		status = check_existence(vs_storage, *tps->ps_ref, vs_count);  // first check if the shader has been indexed (reading from the table) and update the index storage (or not)
		if (!status.exists)
		{
			update_storage(*tps->ps_ref, tps->ps_shader->total_size, tps->ps_shader->psid, vs_storage, vs_count);
			*tps->ps_ref = vs_storage[vs_count].final_index;  // patch table with new index
			dst_shader = (char*)memcpy(write_pos, vs_storage[vs_count].object, vs_storage[vs_count].bytes_to_write);
			write_pos += vs_storage[vs_count].bytes_to_write;
			acc_size += vs_storage[vs_count].bytes_to_write;
			dst_shader_start = dst_shader;
			txe_idx_dist = ((char*)tps->ps_shader->txeindex - tps->ps_shader->psid);
			dst_shader += txe_idx_dist;
			txe_index = (uint32_t*)dst_shader; // get the texture list index, referenced by the shader 
			status2 = check_existence(txe_storage, *txe_index, txe_count);
			if (!status2.exists)
			{
				update_storage(*txe_index, tps->ps_shader->txeptr->esize, tps->ps_shader->txeptr->eptr, txe_storage, txe_count);
				write_txe(tps->ps_shader->txeptr, txe_buffer);
				*txe_index = txe_storage[txe_count].final_index;
				txe_count++;
			}
			else
			{
				*txe_index = txe_storage[status2.index].final_index;
			}
			vs_count++;
			return vs_storage[vs_count - 1].bytes_to_write;
		}
		else
		{
			*tps->ps_ref = vs_storage[status.index].final_index; // patch table with new index
			return 0;
		}
	}
	else
	{
		update_storage(*tps->ps_ref, tps->ps_shader->total_size, tps->ps_shader->psid, vs_storage, 0);
		*tps->ps_ref = vs_storage[0].final_index;
		vs_count++;
		dst_shader = (char*)memcpy(write_pos, vs_storage[0].object, vs_storage[0].bytes_to_write);
		write_pos += vs_storage[0].bytes_to_write;
		acc_size += vs_storage[0].bytes_to_write;
		dst_shader_start = dst_shader;
		txe_idx_dist = ((char*)tps->ps_shader->txeindex - tps->ps_shader->psid);
		dst_shader += txe_idx_dist;
		txe_index = (uint32_t*)dst_shader; // get the texture list index, referenced by the shader 
		status2 = check_existence(txe_storage, *txe_index, txe_count);
		if (!status2.exists)
		{
			update_storage(*txe_index, tps->ps_shader->txeptr->esize, tps->ps_shader->txeptr->eptr, txe_storage, txe_count);
			write_txe(tps->ps_shader->txeptr, txe_buffer);
			*txe_index = txe_storage[txe_count].final_index;
			txe_count++;
		}
		else
		{
			*txe_index = txe_storage[status2.index].final_index;
		}
		return vs_storage[0].bytes_to_write;
	}
}



void table_child_buf_wr(table_child* src, char* dst, uint32_t t_count)
{
	memcpy(dst,src->shader_id,16);
	memcpy((dst+16), src->system_id, 16);
	memcpy((dst+32), src->vbuffer_id, 16);
	memcpy((dst+48), src->unknown_data, 40);
	return;
}


void fill_txe_item2(table_parent*& tps, map_item*& txe_storage, uint32_t& txe_count,char* txe_buffer)
{
	item_existence status;
	status = check_existence(txe_storage, *tps->txe_ref2, txe_count);
	if (!status.exists)
	{
		update_storage(*tps->txe_ref2, tps->txeptr_2->esize, tps->txeptr_2->eptr,txe_storage,txe_count);
		*tps->txe_ref2 = txe_storage[txe_count].final_index;
		acc_txe += txe_storage[txe_count].bytes_to_write;
		write_txe(tps->txeptr_2, txe_buffer);
		txe_count++;
		return;
	}
	else
	{
		*tps->txe_ref2 = txe_storage[status.index].final_index;
		return;
	}
}


void fill_txe_item1(table_parent*& tps, map_item*& txe_storage, uint32_t& txe_count, char* txe_buffer)
{
	item_existence status;
	if (txe_count > 0)
	{
		status = check_existence(txe_storage, *tps->txe_ref1, txe_count);
		if (!status.exists)
		{
			update_storage(*tps->txe_ref1, tps->txeptr_null->esize, tps->txeptr_null->eptr,txe_storage,txe_count);
			*tps->txe_ref1 = txe_storage[txe_count].final_index;
			acc_txe += txe_storage[txe_count].bytes_to_write;
			write_txe(tps->txeptr_null, txe_buffer);
			txe_count++;
			return;
		}
		else
		{
			*tps->txe_ref1 = txe_storage[status.index].final_index;
			return;
		}
	}
	else
	{
		update_storage(*tps->txe_ref1, tps->txeptr_null->esize, tps->txeptr_null->eptr,txe_storage,txe_count);
		acc_txe += txe_storage[0].bytes_to_write;
		write_txe(tps->txeptr_null, txe_buffer);
		txe_count++;
		*tps->txe_ref1 = txe_storage[0].final_index;
		return;
	}
}

