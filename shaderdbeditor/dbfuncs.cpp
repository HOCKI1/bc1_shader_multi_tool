#define _CRT_SECURE_NO_WARNINGS
#include "structs.h"
#include <stdio.h>
#include "funcs.h"
#include <vector>
#include <algorithm>
#include <stdlib.h>

FILE* dbg_log = NULL;
#define LOG(...) { if(dbg_log) { fprintf(dbg_log, __VA_ARGS__); fflush(dbg_log); } }

bool is_ps3 = false;
inline uint32_t SWAP32_P(uint32_t* ptr) { if (is_ps3) *ptr = _byteswap_ulong(*ptr); return *ptr; }
inline uint16_t SWAP16_P(uint16_t* ptr) { if (is_ps3) *ptr = _byteswap_ushort(*ptr); return *ptr; }

const uint32_t txe_e_size = 140;
const uint32_t txe_p_size = 56;
const uint32_t txe_t_size = 40;

void txequicklook(void* &dbfile, database*& dbptr);
void d3dparamslook(void* &dbfile, database*& dbptr);
void vslook(void*& dbfile, database*& dbptr);
void pslook(void*& dbfile, database*& dbptr);
void tableslook(void*& dbfile, database*& dbptr);
void vbufferslook(void*& dbfile, database*& dbptr);
void stringslook(void*& dbfile, database*& dbptr);
void shader_ref_look(void*& dbfile, database*& dbptr);


database_export* export_db_init(database* &dbptr, void* dbfile)
{
	LOG("Entering export_db_init...\n");
	database_export* new_db = new database_export;
	new_db->systems_ref = dbptr->strings[1];
	new_db->shaders = build_shaders(dbfile, dbptr, new_db->systems_ref);
	new_db->mesh_refs = dbptr->strings[2];
	new_db->n_shaders = dbptr->n_strings[0];
	new_db->n_systems = dbptr->n_strings[1];
	new_db->n_mesh_refs = dbptr->n_strings[2];
	new_db->n_vx_buffers = dbptr->n_vx_buffers;
	new_db->vx_buffers = dbptr->vx_buffers;
	new_db->d3dparams_size = dbptr->d3dparams_size;
	new_db->d3dparams_block = dbptr->d3dparams_block;
	new_db->n_txe = dbptr->ntxe;
	new_db->n_d3d_p1 = dbptr->n_d3d_p1;
	new_db->d3d_p1_size = dbptr->d3d_p1_size;
	new_db->n_ps = dbptr->n_ps_shader;
	new_db->n_vs = dbptr->n_vs_shader;
	new_db->n_table_elements = dbptr->n_table_elements;
	LOG("export_db_init DONE\n");
	return new_db;
}


database* dbfirstload(void* dbfile, uint32_t *ptr_end) 
{
    uint32_t* ptr_temp = (uint32_t*)dbfile;
	
    uint32_t test_endian = *(uint32_t*)dbfile;
    is_ps3 = ((test_endian & 0xFF000000) != 0);
    dbg_log = fopen("parse_debug.log", "a");
    LOG("is_ps3 = %d\n", is_ps3);
    uint32_t position = (uint32_t)ptr_temp;

	database* dbptr = new database;
	LOG("txequicklook...\n"); txequicklook(dbfile, dbptr); LOG("txequicklook DONE\n");
	LOG("d3dparamslook...\n"); d3dparamslook(dbfile,dbptr); LOG("d3dparamslook DONE\n");
	LOG("vslook...\n"); vslook(dbfile, dbptr); LOG("vslook DONE\n");
	LOG("pslook...\n"); pslook(dbfile,dbptr); LOG("pslook DONE\n");
	LOG("tableslook...\n"); tableslook(dbfile,dbptr); LOG("tableslook DONE\n");
	LOG("vbufferslook...\n"); vbufferslook(dbfile, dbptr); LOG("vbufferslook DONE\n");
	LOG("stringslook...\n"); stringslook(dbfile,dbptr); LOG("stringslook DONE\n");
	LOG("shader_ref_look...\n"); shader_ref_look(dbfile, dbptr); LOG("shader_ref_look DONE\n");
	position = (uint32_t)dbfile - position;
	*ptr_end = position;
	LOG("dbfirstload completely finished!\n");
	return dbptr;
}

shader_ref_struct* seek_table_ref(void* dbfile, database*& dbptr, char* target)
{
	int i = 0;
	while ((strncmp(target,dbptr->shader_refs[i].id,16)) && (i<dbptr->n_strings[0]))
	{
		i++;
	}
	return &dbptr->shader_refs[i];
}

shader_ref_struct* seek_table_ref_v2(void* dbfile, database*& dbptr, char* target)
{
	int index = 0;
	int success = 0;
	for (int i = 0; i < dbptr->n_strings[0]; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			if (dbptr->shader_refs[i].id[j] != target[j])
			{
				break;
			}		
			else {
				success++;
			}
		}
		if (success == 16)
		{	
			return &dbptr->shader_refs[i];
		}
		success = 0;
	}
	return &dbptr->shader_refs[index];
}

vxbuffer* seek_vxbuffer(database*& dbptr, char* target)
{
	int i = 0;
	while (i < dbptr->n_vx_buffers && (strncmp(target, dbptr->vx_buffers[i].vxbuffer_id, 16)))
	{
		i++;
	}
	if (i >= dbptr->n_vx_buffers) return nullptr;
	return &dbptr->vx_buffers[i];
}

string_ref* seek_parent(database*& dbptr, char* target, uint32_t* root_idx)
{
	int i = 0;
	while (i < dbptr->n_strings[1] && (strncmp(target, dbptr->strings[1][i].guid, 16)))
	{
		i++;
	}
	if (i >= dbptr->n_strings[1]) i = 0;
	*root_idx = i;
	return &dbptr->strings[1][i];
}

shader_struct* build_shaders(void* dbfile, database*& dbptr, string_ref* &systems_ref)
{
	uint32_t dummy_int;
	int acc = 0;
	shader_struct* shaders = new shader_struct[dbptr->n_strings[0]];
	for (uint32_t i = 0; i < dbptr->n_strings[0]; i++)
	{
		dummy_int = 0;
		shaders[i].associated_db = dbptr;
		shaders[i].shader_ref = &shaders[i].associated_db->strings[0][i];
		shaders[i].tables_ref = seek_table_ref_v2(dbfile, dbptr, dbptr->strings[0][i].guid);
		shaders[i].tables_ref->id = shaders[i].shader_ref->guid;
		shaders[i].table_members = new table_parent[shaders[i].tables_ref->n_table_refs];
		shaders[i].root_nodes = new string_ref[shaders[i].tables_ref->n_table_refs];
		shaders[i].root_node_idx = new uint32_t[shaders[i].tables_ref->n_table_refs];
		for (uint32_t j = 0; j < shaders[i].tables_ref->n_table_refs; j++)
		{
			shaders[i].table_members[j] = dbptr->table[shaders[i].tables_ref->table_refs[j]];
			shaders[i].table_members[j].child->shader_id = shaders[i].shader_ref->guid;
			dummy_int = (uint32_t)*shaders[i].table_members[j].child->system_id;
			if (dummy_int)
			{

				shaders[i].root_nodes[j] = *seek_parent( dbptr, shaders[i].table_members[j].child->system_id,&shaders[i].root_node_idx[j]);
				shaders[i].table_members[j].child->system_id = systems_ref[shaders[i].root_node_idx[j]].guid; // the guid now points to the systems ref instead of the child table element
				shaders[i].table_members[j].child->has_parent = TRUE;
				shaders[i].is_child = TRUE;
			}			
			else
			{				
				shaders[i].root_node_idx[j] = -1;
				shaders[i].is_child = FALSE;
				shaders[i].table_members[j].child->has_parent = FALSE;
			}
			shaders[i].selected = FALSE;		
			shaders[i].initialize_items = TRUE;
		}	
	}	
	return shaders;
}

void shader_ref_look(void*& dbfile, database*& dbptr)
{
	int test = 0;
	int acc = 0;
	uint32_t* nelements = (uint32_t*)dbfile;
	SWAP32_P(nelements);
	LOG("shader_ref_look: nelements = %u\n", *nelements);
	shader_ref_struct* shader_refs = new shader_ref_struct[*nelements];
	char* dummy = (char*)dbfile + 4;
	for (uint32_t i = 0; i < *nelements; i++)
	{
		shader_refs[i].id = dummy;
		dummy += 16;
		uint32_t* nt = (uint32_t*)dummy;
		shader_refs[i].n_table_refs = SWAP32_P(nt);
		dummy += 4;
		shader_refs[i].table_refs = (uint16_t*)dummy;
		for (uint32_t j = 0; j < shader_refs[i].n_table_refs; j++) {
			SWAP16_P(&shader_refs[i].table_refs[j]);
		}
		dummy += (2 * shader_refs[i].n_table_refs);
		acc += shader_refs[i].n_table_refs;
	}
	dbptr->shader_refs = shader_refs;
	dbfile = dummy;
	return;
}

void string_pass(void*& dbfile, database*& dbptr, uint32_t elements, string_ref* &str_ref, int pass)
{
	char* dummy = (char*)dbfile + 4;
	int j = 0;
	for (int i = 0; i < elements; i++)
	{
		str_ref[i].name = dummy;	    
		while (str_ref[i].name[j] != 0)
		{
			j++;
		}
		dummy += (j+1);
		str_ref[i].guid = dummy;
		dummy += 16;
		j = 0;
		if (pass < 2)
		{
			str_ref[i].vx_buffer = nullptr;
		}
		else
		{
			str_ref[i].vx_buffer = seek_vxbuffer(dbptr,str_ref[i].guid);
		}
		str_ref[i].selected = FALSE;
		str_ref[i].initialize_items = TRUE;
	}
	dbfile = dummy;
	return;
}

void stringslook(void* &dbfile,database* &dbptr) 
{
	dbptr->strings = new string_ref*[3];
	for (int i = 0; i < 3; i++)
	{
		uint32_t* nelements = (uint32_t*)dbfile;
		SWAP32_P(nelements);
		LOG("stringslook[%d]: n_strings = %u\n", i, *nelements);
		dbptr->n_strings[i] = *nelements;
		dbptr->strings[i] = new string_ref[*nelements];
		string_pass(dbfile,dbptr,*nelements, dbptr->strings[i], i);
	}
	return;
}

void vbufferslook(void*& dbfile, database*& dbptr)
{
	uint32_t* nelements = (uint32_t*)dbfile;
	SWAP32_P(nelements);
	LOG("vbufferslook: n_vx_buffers = %u\n", *nelements);
	vxbuffer* vxbuffers = new vxbuffer[*nelements];
	char* dummy = (char*)dbfile + 4;
	if (is_ps3) {
		for (uint32_t i = 0; i < *nelements; i++)
		{
			vxbuffers[i].vxbuffer_id = dummy;
			vxbuffers[i].n_data_types = (uint8_t*)(dummy + 16);
			vxbuffers[i].type = (uint32_t*)(dummy + 17);
			vxbuffers[i].vx_stride = (uint8_t*)(dummy + 21);
			vxbuffers[i].unknown_data = (dummy + 16);
			vxbuffers[i].n_wnds = 3;
			vxbuffers[i].initialize_items = TRUE;
			dummy += 175;
		}
	} else {
		char* arrayread;
		for (uint32_t i = 0; i < *nelements; i++)
		{
			arrayread = (dummy+16);
			vxbuffers[i].vxbuffer_id = dummy;
			vxbuffers[i].n_data_types = (uint8_t*)(dummy + 80);
			vxbuffers[i].type = (uint32_t*)(dummy + 81);
			vxbuffers[i].vx_stride = (uint8_t*)(dummy + 85);
			if (*vxbuffers[i].type != 4294967040)
			{	
				vxbuffers[i].unknown_data = (dummy + 16);
				vxbuffers[i].n_wnds = 3;
			}
			else 
			{
				vxbuffers[i].types_array = new uint16_t*[*vxbuffers[i].n_data_types];
				for (uint32_t j = 0; j < *vxbuffers[i].n_data_types; j++)
				{
					vxbuffers[i].types_array[j] = (uint16_t*)(arrayread);
					arrayread += 4;
				}
				vxbuffers[i].n_wnds = 4 + *vxbuffers[i].n_data_types;
			}
			dummy += 89;
			vxbuffers[i].initialize_items = TRUE;
		}
	}
	dbptr->vx_buffers = vxbuffers;
	dbptr->n_vx_buffers = *nelements;
	dbfile = dummy;
	return;
}

void tableslook(void* &dbfile,database* &dbptr)
{
	uint32_t* nelements = (uint32_t*)dbfile;
	SWAP32_P(nelements);
	LOG("tableslook: n_table_elements = %u\n", *nelements);
	table_parent* table_p = new table_parent[*nelements];
	table_child* table_c = new table_child[*nelements];
	char* dummy = (char*)dbfile + 4;
	for (uint32_t i = 0; i < *nelements; i++) 
	{
		table_p[i].element_id = dummy;
		dummy += 16;
		table_p[i].unknown1 = (uint32_t*)(dummy); SWAP32_P(table_p[i].unknown1);
		table_p[i].unknown2 = (uint32_t*)(dummy + 4); SWAP32_P(table_p[i].unknown2);
		table_p[i].temp_hash = (uint32_t*)(dummy + 8); SWAP32_P(table_p[i].temp_hash);
		table_p[i].null_int = (uint32_t*)(dummy + 12); SWAP32_P(table_p[i].null_int);
		table_p[i].vs_ref = (uint32_t*)(dummy + 16); SWAP32_P(table_p[i].vs_ref);
		table_p[i].ps_ref = (uint32_t*)(dummy + 20); SWAP32_P(table_p[i].ps_ref);
		table_p[i].txe_ref1 = (uint32_t*)(dummy + 24); SWAP32_P(table_p[i].txe_ref1);
		table_p[i].txe_ref2 = (uint32_t*)(dummy + 28); SWAP32_P(table_p[i].txe_ref2);
		table_p[i].ps_shader = &dbptr->ps_shader[*table_p[i].ps_ref];
		table_p[i].vs_shader = &dbptr->vs_shader[*table_p[i].vs_ref];
		table_p[i].txeptr_null = &dbptr->txeptr[*table_p[i].txe_ref1];
		table_p[i].txeptr_2 = &dbptr->txeptr[*table_p[i].txe_ref2];
		table_p[i].lparam_data.selected = FALSE;
		table_p[i].initialize_items = TRUE;
		dummy += 32;
	}
	uint32_t* dummy_cnt = (uint32_t*)dummy;
	SWAP32_P(dummy_cnt);
	dummy += 4;
	for (uint32_t i = 0; i < *nelements; i++)
	{
		table_c[i].shader_id = dummy;
		table_c[i].system_id = (dummy+16);
		table_c[i].vbuffer_id = (dummy+32);
		table_c[i].unknown_data = (dummy+48);
		table_p[i].child = &table_c[i]; 
		dummy += (is_ps3 ? 92 : 88);
	}
	dbptr->table = table_p;
	dbptr->n_table_elements = *nelements;
	dbfile = dummy;
	return;
}

void pslook(void*& dbfile, database*& dbptr)
{
	uint32_t* nelements = (uint32_t*)dbfile;
	SWAP32_P(nelements);
	LOG("pslook: n_ps_shader = %u\n", *nelements);
	uint32_t dummyint;
	bc2_ps* pxshdr = new bc2_ps[*nelements];
	char* dummy = (char*)dbfile + 4;
	for (uint32_t i = 0; i < *nelements; i++) {
		pxshdr[i].psid = dummy;
		dummy += 16;
		pxshdr[i].ps_size = (uint32_t*)dummy;
		dummyint = SWAP32_P(pxshdr[i].ps_size);
		dummy += 4;
		pxshdr[i].shader = dummy;
		dummy += dummyint;
		pxshdr[i].txeindex = (uint32_t*)dummy;
		SWAP32_P(pxshdr[i].txeindex);
		dummy += 4;
		pxshdr[i].d3d1p = (uint32_t*)dummy;
		SWAP32_P(pxshdr[i].d3d1p);
		dummy += 4;
		pxshdr[i].d3d2p = (uint32_t*)dummy;
		SWAP32_P(pxshdr[i].d3d2p);
		dummy += 4;
		pxshdr[i].total_size = (dummy - pxshdr[i].psid);
		pxshdr[i].txeptr = &dbptr->txeptr[*pxshdr[i].txeindex];
		pxshdr[i].initialize_items = TRUE;
		pxshdr[i].lparam_data.selected = FALSE;
	}
	dbptr->n_ps_shader = *nelements;
	dbptr->ps_shader = pxshdr;
	dbfile = dummy;
	return;
}

void vslook(void*& dbfile, database*& dbptr)
{
	uint32_t* nelements = (uint32_t*)dbfile;
	SWAP32_P(nelements);
	LOG("vslook: n_vs_shader = %u\n", *nelements);
	uint32_t dummyint;
	bc2_vs* vxshdr = new bc2_vs[*nelements];
	char* dummy = (char*)dbfile + 4;
	for (uint32_t i = 0; i < *nelements; i++) {
		vxshdr[i].vsid = dummy; 
		dummy += 16;
		vxshdr[i].vs_size = (uint32_t*)dummy;
		dummyint = SWAP32_P(vxshdr[i].vs_size);
		dummy += 4;
		vxshdr[i].shader = dummy; 
		dummy += dummyint; 
		vxshdr[i].txeindex = (uint32_t*)dummy;
		SWAP32_P(vxshdr[i].txeindex);
		dummy += 4;
		vxshdr[i].d3d1p = (uint32_t*)dummy;
		SWAP32_P(vxshdr[i].d3d1p);
		dummy += 4;
		vxshdr[i].d3d2p = (uint32_t*)dummy;
		SWAP32_P(vxshdr[i].d3d2p);
		dummy += 4;
		if (is_ps3) {
			vxshdr[i].obf_size = nullptr;
			vxshdr[i].obf_shader = nullptr;
			vxshdr[i].n_txcoord = nullptr;
		} else {
			vxshdr[i].obf_size = (uint32_t*)dummy;
			dummyint = SWAP32_P(vxshdr[i].obf_size);
			dummy += 4;
			vxshdr[i].obf_shader = dummy;
			dummy += dummyint;
			vxshdr[i].n_txcoord = (uint32_t*)dummy;
			dummyint = SWAP32_P(vxshdr[i].n_txcoord);
			dummy += ((dummyint * 7) * 4) + (dummyint*9) + 8;
		}
		vxshdr[i].total_size = (dummy - vxshdr[i].vsid);
		vxshdr[i].txeptr = &dbptr->txeptr[*vxshdr[i].txeindex];
		vxshdr[i].initialize_items = TRUE;
		vxshdr[i].lparam_data.selected = FALSE;
	}
	dbptr->n_vs_shader = *nelements;
	dbptr->vs_shader = vxshdr;
	dbfile = dummy;
	return;
}

void d3dparamslook(void* &dbfile, database*& dbptr)
{
	uint32_t* nelements = (uint32_t*)dbfile;
	SWAP32_P(nelements);
	LOG("d3dparamslook: n_d3d_p1 = %u\n", *nelements);
	uint32_t* ebase2;
	uint16_t* ebase;    
	dbptr->d3dparams_block = (char*)dbfile;
	dbptr->n_d3d_p1 = *nelements;
	char* dummy = (char*)dbfile + 4;
	for (uint32_t i = 0; i < *nelements; i++) {
		ebase = (uint16_t*)dummy;
		SWAP16_P(ebase);
		dummy += (*ebase * 4) + 4;
	}
	dbptr->d3d_p1_size = dummy - (char*)dbfile;
	nelements = (uint32_t*)dummy;
	SWAP32_P(nelements);
	LOG("d3dparamslook: n_d3d_p2 = %u\n", *nelements);
	dummy += 4;
	for (uint32_t i = 0; i < *nelements; i++) {
		ebase2 = (uint32_t*)dummy;
		SWAP32_P(ebase2);
		dummy += (*ebase2 * 4) + 4;
	}
	dbptr->d3dparams_size = (dummy - (char*)dbfile);
	dbfile = dummy; 
	return;
}

void txequicklook(void* &dbfile, database* &dbptr) 
{
	char* dummy = (char*)dbfile + 5;
	char* traverse; 
	uint32_t* ntxeptr = (uint32_t*)dummy;
	SWAP32_P(ntxeptr);
	LOG("txequicklook: ntxe = %u\n", *ntxeptr);
	uint32_t* ptr1;
	TxE* txelist;
	txe_e* txnames;
	txe_p* txparams;
	txe_t* txtypes;
	dbptr->ntxe = *ntxeptr;
    txelist = new TxE[dbptr->ntxe];
	dummy += 4;
	for (int i = 0; i < dbptr->ntxe; i++) {
		ptr1 = (uint32_t*)dummy;
		txelist[i].esize = SWAP32_P(ptr1);
		txelist[i].eptr = (uint32_t*)dummy;
		traverse = dummy + 8;
		txelist[i].hsize = (uint32_t*)traverse; SWAP32_P(txelist[i].hsize);
		traverse += 4;
		txelist[i].gameparamsoffset = (uint32_t*)traverse; SWAP32_P(txelist[i].gameparamsoffset);
		traverse += 4;
		txelist[i].filetypeoffset = (uint32_t*)traverse; SWAP32_P(txelist[i].filetypeoffset);
		traverse += 4;
		txelist[i].unknowndata = (uint32_t*)traverse; SWAP32_P(txelist[i].unknowndata);
		traverse += 7;
		txelist[i].ntxt = (uint8_t*)traverse;
		traverse += 1;
		txelist[i].nparams = (uint8_t*)traverse;
		traverse += 1;
		txelist[i].ntypes = (uint8_t*)traverse;
		traverse += 1;
		txelist[i].ff7ffstring = (uint8_t*)traverse;
		traverse += 2;
		if (*txelist[i].ntxt > 0) {
			txnames = new txe_e[*txelist[i].ntxt];
			for (int j = 0; j < *txelist[i].ntxt; j++) {
				txnames[j].index = (uint32_t*)traverse;
				SWAP32_P(txnames[j].index);
				txnames[j].name = traverse+4; 
				traverse += 140;
			}
			txelist[i].txtptrs = txnames;
		}
		if (*txelist[i].nparams > 0) {
			txparams = new txe_p[*txelist[i].nparams];
			for (int j = 0; j < *txelist[i].nparams; j++) {
				txparams[j].name = traverse;
				traverse += 56;
			}		
			txelist[i].paramptrs = txparams;

		}
		if (*txelist[i].ntypes > 0) {
			txtypes = new txe_t[*txelist[i].ntypes];
			for (int j = 0; j < *txelist[i].ntypes; j++) {
				txtypes[j].name = traverse;
				traverse += 40;
			}
			txelist[i].typeptrs = txtypes; 
		}
		dummy += txelist[i].esize;
		txelist[i].lparam_data.selected = FALSE;
		txelist[i].initialize_items = TRUE;
	}
	dbptr->txeptr = txelist; 
	dbfile = dummy; 
	return;
}


TxE* txe_individual_assign(char* src)
{
	char* dummy = src;
	char* traverse;
	uint32_t* ptr1;
	TxE* txelist;
	txe_e* txnames;
	txe_p* txparams;
	txe_t* txtypes;
	txelist = new TxE[1];
	for (int i = 0; i < 1; i++) {
		ptr1 = (uint32_t*)dummy;
		txelist[i].esize = SWAP32_P(ptr1);
		txelist[i].eptr = (uint32_t*)dummy;
		traverse = dummy + 8;
		txelist[i].hsize = (uint32_t*)traverse; SWAP32_P(txelist[i].hsize);
		traverse += 4;
		txelist[i].gameparamsoffset = (uint32_t*)traverse; SWAP32_P(txelist[i].gameparamsoffset);
		traverse += 4;
		txelist[i].filetypeoffset = (uint32_t*)traverse; SWAP32_P(txelist[i].filetypeoffset);
		traverse += 4;
		txelist[i].unknowndata = (uint32_t*)traverse; SWAP32_P(txelist[i].unknowndata);
		traverse += 7;
		txelist[i].ntxt = (uint8_t*)traverse;
		traverse += 1;
		txelist[i].nparams = (uint8_t*)traverse;
		traverse += 1;
		txelist[i].ntypes = (uint8_t*)traverse;
		traverse += 1;
		txelist[i].ff7ffstring = (uint8_t*)traverse;
		traverse += 2;
		if (*txelist[i].ntxt > 0) {
			txnames = new txe_e[*txelist[i].ntxt];
			for (int j = 0; j < *txelist[i].ntxt; j++) {
				txnames[j].index = (uint32_t*)traverse; SWAP32_P(txnames[j].index);
				txnames[j].name = traverse + 4;
				traverse += 140;
			}
			txelist[i].txtptrs = txnames;
		}
		if (*txelist[i].nparams > 0) {
			txparams = new txe_p[*txelist[i].nparams];
			for (int j = 0; j < *txelist[i].nparams; j++) {
				txparams[j].name = traverse;
				traverse += 56;
			}
			txelist[i].paramptrs = txparams;
		}
		if (*txelist[i].ntypes > 0) {
			txtypes = new txe_t[*txelist[i].ntypes];
			for (int j = 0; j < *txelist[i].ntypes; j++) {
				txtypes[j].name = traverse;
				traverse += 40;
			}
			txelist[i].typeptrs = txtypes;
		}
		dummy += txelist[i].esize;
		txelist[i].lparam_data.selected = FALSE;
		txelist[i].initialize_items = TRUE;
	}
	return &txelist[0];
}
