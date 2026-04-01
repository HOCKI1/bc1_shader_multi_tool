#include "importfuncs.h"
#include "funcs.h"
#include <stdio.h>


char* alloc_db(FILE* input)
{
	char* dbfile;
	int dbsize;
	fseek(input, SEEK_SET, SEEK_END);
	dbsize = ftell(input);
	fseek(input, 0, SEEK_SET);
	dbfile = (char*)malloc(dbsize * sizeof(char));
	fread(dbfile, 1, dbsize, input);
	fclose(input);
	return dbfile;
}

// merges databases, doesnt check for mesh or shader names existence. tables get assigned a custom ID to avoid collisions (even though I dont believe that would ever be the case) 
// it does check for existing vertex buffers however
// vertex, pixel shaders and tables need to be patched 
database_export* import_db(database_export* old_db, char* target)
{
	char base_guid[25] = "000102030405060708090A0B";
	uint64_t half1 = 0x0807060504030201;
	uint64_t half2 = 0x11100F0D0C0B0A09;
	char* dbfile;
	int dbsize;
	database* temp_db = nullptr;
	database_export* new_db = new database_export;
	uint32_t ptr_pos = 0;
	uint32_t* ptr_end = &ptr_pos;
	FILE* input = fopen(target, "r+b");
	dbfile = alloc_db(input);
	allocated_db.mem_blocks[allocated_db.used_slots] = dbfile;
	allocated_db.used_slots++;
	temp_db = dbfirstload(dbfile, ptr_end);
	uint32_t base_txe = old_db->n_txe;
	uint32_t base_d3d1 = old_db->n_d3d_p1;
	uint32_t base_vs = old_db->n_vs;
	uint32_t base_ps = old_db->n_ps;
	uint32_t base_table = old_db->n_table_elements;
	// arrange the new d3d params block. merges the 2 and updates size and count
	uint32_t d3d_size_diff = old_db->d3dparams_size - old_db->d3d_p1_size;
	new_db->d3dparams_block = (char*)memcpy(file_edit_pool.pool + file_edit_pool.current_pos, old_db->d3dparams_block, old_db->d3d_p1_size);
	file_edit_pool.current_pos += old_db->d3d_p1_size;
	memcpy(file_edit_pool.pool + file_edit_pool.current_pos, temp_db->d3dparams_block + 4, temp_db->d3d_p1_size - 4);
	file_edit_pool.current_pos += temp_db->d3d_p1_size - 4;
	memcpy(file_edit_pool.pool + file_edit_pool.current_pos, old_db->d3dparams_block + old_db->d3d_p1_size, d3d_size_diff);
	*(uint32_t*)new_db->d3dparams_block = (old_db->n_d3d_p1 + temp_db->n_d3d_p1); // patch number of elements
	// patch vs shaders
	for (int i = 0; i < temp_db->n_vs_shader; i++)
	{
		*temp_db->vs_shader[i].d3d1p += base_d3d1;
		*temp_db->vs_shader[i].txeindex += base_txe;
	}
	// patch ps shaders
	for (int i = 0; i < temp_db->n_ps_shader; i++)
	{
		*temp_db->ps_shader[i].d3d1p += base_d3d1;
		*temp_db->ps_shader[i].txeindex += base_txe;
	}
	// patch tables 
	for (int i = 0; i < temp_db->n_table_elements; i++)
	{
		*(uint64_t*)temp_db->table[i].element_id = half1;
		*(uint64_t*)(temp_db->table[i].element_id + 8) = half2;
		*temp_db->table[i].vs_ref += base_vs;
		*temp_db->table[i].ps_ref += base_ps;
		*temp_db->table[i].txe_ref1 += base_txe;
		*temp_db->table[i].txe_ref2 += base_txe;
		half2 += 666;
	}
	// check for repeated vx buffers. the temp db array is deleted (its is temp for a reason) and the new db gets the merged array of vx buffer structs
	uint32_t new_vbuffers = 0;
	uint32_t new_vbuffers_idx[150];
	for (int i = 0; i < temp_db->n_vx_buffers; i++)
	{
		if (seek_guid_vx(old_db->n_vx_buffers, old_db->vx_buffers, temp_db->vx_buffers[i].vxbuffer_id) == -1)
		{
			new_vbuffers_idx[new_vbuffers] = i;
			new_vbuffers++;
		}
	}
	new_db->vx_buffers = new vxbuffer[new_vbuffers + old_db->n_vx_buffers];
	memcpy(new_db->vx_buffers, old_db->vx_buffers, sizeof(vxbuffer) * old_db->n_vx_buffers);
	for (int i = 0; i < new_vbuffers; i++)
	{
		memcpy(&new_db->vx_buffers[old_db->n_vx_buffers + i], &temp_db->vx_buffers[new_vbuffers_idx[i]], sizeof(vxbuffer) * 1);
	}
	// check for repeated systems 
	uint32_t new_sys = 0;
	uint32_t new_sys_idx[16];
	for (int i = 0; i < temp_db->n_strings[1]; i++)
	{
		if (seek_guid(old_db->n_systems, old_db->systems_ref, temp_db->strings[1][i].guid) == -1)
		{
			new_sys_idx[new_sys] = i;
			new_sys++;
		}
	}
	new_db->systems_ref = new string_ref[new_sys + old_db->n_systems];
	memcpy(new_db->systems_ref, old_db->systems_ref, sizeof(string_ref) * old_db->n_systems);
	for (int i = 0; i < new_sys; i++)
	{
		memcpy(&new_db->systems_ref[old_db->n_systems + i], &temp_db->strings[1][new_sys_idx[i]], sizeof(string_ref) * 1);
	}
	// create the new shaders 
	shader_struct* new_shaders = build_shaders(dbfile, temp_db, new_db->systems_ref);
	new_db->shaders = new shader_struct[old_db->n_shaders + temp_db->n_strings[0]];
	memcpy(new_db->shaders, old_db->shaders, sizeof(shader_struct) * old_db->n_shaders);
	memcpy(&new_db->shaders[old_db->n_shaders], new_shaders, sizeof(shader_struct) * temp_db->n_strings[0]);
	// patch string references 
	for (int i = 0; i < temp_db->n_strings[0]; i++)
	{
		for (int j = 0; j < temp_db->shader_refs[i].n_table_refs; j++)
		{
			temp_db->shader_refs[i].table_refs[j] += base_table;
		}
	}
	// merge mesh strings. the vxbuffer pointer needs to be updated so it points to the new_db array instead of the temp_db
	int z;
	for (int i = 0; i < temp_db->n_strings[2]; i++)
	{
		z = seek_guid_vx(old_db->n_vx_buffers + new_vbuffers, new_db->vx_buffers, temp_db->strings[2][i].guid);
		temp_db->strings[2][i].vx_buffer = &new_db->vx_buffers[z];
	}
	new_db->mesh_refs = new string_ref[old_db->n_mesh_refs + temp_db->n_strings[2]];
	memcpy(new_db->mesh_refs, old_db->mesh_refs, sizeof(string_ref) * old_db->n_mesh_refs);
	memcpy(&new_db->mesh_refs[old_db->n_mesh_refs], temp_db->strings[2], sizeof(string_ref) * temp_db->n_strings[2]);
	// update info 
	new_db->d3d_p1_size = old_db->d3d_p1_size + (temp_db->d3d_p1_size - 4);
	new_db->d3dparams_size = new_db->d3d_p1_size + d3d_size_diff;
	new_db->n_d3d_p1 = old_db->n_d3d_p1 + temp_db->n_d3d_p1;
	new_db->n_mesh_refs = old_db->n_mesh_refs + temp_db->n_strings[2];
	new_db->n_ps = old_db->n_ps + temp_db->n_ps_shader;
	new_db->n_vs = old_db->n_vs + temp_db->n_vs_shader;
	new_db->n_txe = old_db->n_txe + temp_db->ntxe;
	new_db->n_shaders = old_db->n_shaders + temp_db->n_strings[0];
	new_db->n_table_elements = old_db->n_table_elements + temp_db->n_table_elements;
	new_db->n_systems = old_db->n_systems + new_sys;
	new_db->n_vx_buffers = old_db->n_vx_buffers + new_vbuffers;
	// delete structs of temp_db and old db
	delete[] temp_db->strings[1];
	delete[] temp_db->strings[2];
	delete[] temp_db->vx_buffers;
	delete temp_db;
	delete[] new_shaders;
	delete[] old_db->shaders;
	delete[] old_db->mesh_refs;
	delete[] old_db->systems_ref;
	delete[] old_db->vx_buffers;
	delete old_db;
	return new_db;
}


void import_str(database_export*& old_db, char* target_file) // test this function later. it should work
{
	database* temp = new database;
	FILE* input = fopen(target_file, "r+b");
	void* strfile = alloc_db(input);
	allocated_db.mem_blocks[allocated_db.used_slots] = (char*)strfile;
	allocated_db.used_slots++;
	vbufferslook(strfile, temp);
	uint32_t* nelements = (uint32_t*)strfile;
	temp->n_strings[2] = *nelements;
	temp->strings[2] = new string_ref[*nelements];
	string_pass(strfile, temp, *nelements, temp->strings[2], 2);

	uint32_t new_vbuffers = 0;
	uint32_t new_vbuffers_idx[150];
	for (int i = 0; i < temp->n_vx_buffers; i++)
	{
		if (seek_guid_vx(old_db->n_vx_buffers, old_db->vx_buffers, temp->vx_buffers[i].vxbuffer_id) == -1)
		{
			new_vbuffers_idx[new_vbuffers] = i;
			new_vbuffers++;
		}
	}
	old_db->vx_buffers = new vxbuffer[new_vbuffers + old_db->n_vx_buffers];
	memcpy(old_db->vx_buffers, old_db->vx_buffers, sizeof(vxbuffer) * old_db->n_vx_buffers);
	for (int i = 0; i < new_vbuffers; i++)
	{
		memcpy(&old_db->vx_buffers[old_db->n_vx_buffers + i], &temp->vx_buffers[new_vbuffers_idx[i]], sizeof(vxbuffer) * 1);
	}
	int z;
	for (int i = 0; i < temp->n_strings[2]; i++)
	{
		z = seek_guid_vx(old_db->n_vx_buffers + new_vbuffers, old_db->vx_buffers, temp->strings[2][i].guid);
		temp->strings[2][i].vx_buffer = &old_db->vx_buffers[z];
	}
	old_db->mesh_refs = new string_ref[old_db->n_mesh_refs + temp->n_strings[2]];
	memcpy(old_db->mesh_refs, old_db->mesh_refs, sizeof(string_ref) * old_db->n_mesh_refs);
	memcpy(&old_db->mesh_refs[old_db->n_mesh_refs], temp->strings[2], sizeof(string_ref) * temp->n_strings[2]);

	delete[] temp->vx_buffers;
	delete[] temp->strings[2];
	delete temp;
	return;
}