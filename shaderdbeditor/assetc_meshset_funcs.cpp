#include "assetc_meshset_funcs.h"

#define RIGID_MESH 0
#define SKINNED_MESH 1 
#define COMPOSITE_MESH 2
#define TREE_MESH 4

typedef uint32_t u32;
typedef uint8_t u8;

meshset* meshset_reader(uint32_t type, char* src);
char* composite_data_reader(char* src, comp_n_tree_data* data, uint32_t n_nodes);
char* skinned_data_reader(char* src, skinned_mesh_data* data);
void to_lower_inplace(char* src);

meshset* meshset_handler(const char* src_file, uint32_t* type)
{
	meshset* result = nullptr;
	FILE* meshset = fopen(src_file, "r+b");
	int fsize;
	fseek(meshset, SEEK_SET, SEEK_END);
	fsize = ftell(meshset);
	fseek(meshset, 0, SEEK_SET);
	char* file_buffer = &ac_data_pool->pool[ac_data_pool->current_pos];
	fread(file_buffer, 1, fsize, meshset);
	ac_data_pool->current_pos += fsize;
	fclose(meshset);
	const char* dot = strstr(src_file, ".");
    to_lower_inplace(const_cast<char*>(dot));
	if (strstr(src_file, ".rigidmeshset"))
	{
		*type = RIGID_MESH;
		result = meshset_reader(*type, file_buffer);
	}
	else if (strstr(src_file, ".skinnedmeshset"))
	{
		*type = SKINNED_MESH;
		result = meshset_reader(*type, file_buffer);
	}
	else if (strstr(src_file, ".compositemeshset"))
	{
		*type = COMPOSITE_MESH;
		result = meshset_reader(*type, file_buffer);
	}
	else if (strstr(src_file, ".treemeshset"))
	{
		*type = TREE_MESH;
		result = meshset_reader(*type, file_buffer);
	}
	return result;
}



void to_lower_inplace(char* src)
{
	for (int i = 0; src[i]; i++)
	{
		src[i] = tolower(src[i]);
	}
	return;
}


meshset* meshset_reader(uint32_t type, char* src) 
{ 
	// initialize meshset header
	meshset* mesh_set = (meshset*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(meshset);
	mesh_set->magic = src;
	src += 5;
	mesh_set->lods_file = src;
	int length = strlen(mesh_set->lods_file);
	mesh_set->n_lods = *(uint32_t*)(src + length + 1);
	src += length + 5; // it now points to the first lod
	// now initialize lods
	mesh_set->lods = (meshset_lod*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(meshset_lod)*mesh_set->n_lods;
	for (int i = 0; i < mesh_set->n_lods; i++)
	{
		mesh_set->lods[i].unknown= *src;
		mesh_set->lods[i].n_shaders_per_set = *(src+1);
		mesh_set->lods[i].n_shader_sets = *(src+2);  // IF THE NEXT BYTE IS 0, SHADER SET DOES NOT HAVE MESH DBX 
		src += 3;
		mesh_set->lods[i].shader_sets = (shader_set*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
		ac_structs_pool->current_pos += sizeof(shader_set)*mesh_set->lods[i].n_shader_sets;
		for (int j = 0; j < mesh_set->lods[i].n_shader_sets; j++) // shader set level. this is the same for all mesh sets
		{
			mesh_set->lods[i].shader_sets[j].mesh_dbx_path = src;
			src+= strlen(src) + 1;			
			for (int k = 0; k < mesh_set->lods[i].n_shaders_per_set; k++)
			{
				mesh_set->lods[i].shader_sets[j].shaders[k] = src;
				src += strlen(src) + 1;
			}
		}
		for (int j = 0; j < 6; j++)
		{
			mesh_set->lods[i].bounding_box[j] = *(float*)src;
			src += 4;
		}
		mesh_set->lods[i].mesh_size = *(uint32_t*)src;
		src += 4;
		// type specific data
		switch (type)
		{
		case RIGID_MESH: // rigid mesh set
		{
			mesh_set->lods[i].rigid_data = (rigid_mesh_data*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
			ac_structs_pool->current_pos += sizeof(rigid_mesh_data*);
			mesh_set->lods[i].rigid_data->data = src;
			src += 5;
			break;
		}
		case SKINNED_MESH: // skinned 
		{
			mesh_set->lods[i].skinned_data = (skinned_mesh_data*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
			ac_structs_pool->current_pos += sizeof(skinned_mesh_data);
			src = skinned_data_reader(src, mesh_set->lods[i].skinned_data);
			break;
		}
		case COMPOSITE_MESH: 	// composite 
		{
		    mesh_set->lods[i].comp_data = (comp_n_tree_data*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
			ac_structs_pool->current_pos += sizeof(comp_n_tree_data);
			src = composite_data_reader(src, mesh_set->lods[i].comp_data, mesh_set->lods[i].n_shaders_per_set);
			break;
		}
		case TREE_MESH:     // tree 
			mesh_set->lods[i].comp_data = (comp_n_tree_data*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
			ac_structs_pool->current_pos += sizeof(comp_n_tree_data);
			src = composite_data_reader(src, mesh_set->lods[i].comp_data, mesh_set->lods[i].n_shaders_per_set);
			break;
		}
	
	}
	return mesh_set;
}


char* composite_data_reader(char* src, comp_n_tree_data* data, uint32_t n_nodes)
{
	data->data = src;
	data->n_shapes = *(uint32_t*)(src + 5);
	src += 9;
	data->shapes = (shape_struct*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(shape_struct)*data->n_shapes;
	for (int i = 0; i < data->n_shapes; i++)
	{
		data->shapes[i].hash = *(uint32_t*)src;
		src += 4;
		data->shapes[i].name = src; 
		src += strlen(src) + 1;
		for (int j = 0; j < 6; j++)
		{
			data->shapes[i].bounding_box[j] = *(float*)src;	
			src += 4;
		}
		for (int j = 0; j < 12; j++)
		{
			data->shapes[i].transforms[j] = *(float*)src;
			src += 4;
		}	
	}
	data->unknown_data = src;
	src += n_nodes * 24;
	if (!(*src)) // CHECK FOR NULL BYTE. IF ITS NOT, THIS MESH SET HAS A BILLBOARD SHADER WHATEVER
	{
		return src;
	}
	else
	{
		data->billboard = src;
		src += strlen(src) + 5;
	}
	return src;
}

char* skinned_data_reader(char* src, skinned_mesh_data* data) 
{
	data->data = src;
	src += 4;
	data->n_bones = *(uint32_t*)src;
	src += 4;
	data->bone_ids = (uint32_t*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(uint32_t*)*data->n_bones;
	data->unknown_data = (uint16_t*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(uint16_t*)*(data->n_bones*2);
	for (int i = 0; i < data->n_bones; i++)
	{
		data->bone_ids[i] = *(uint32_t*)src;
		src += 4;
	}
	for (int i = 0; i < data->n_bones*2; i++)
	{
		data->unknown_data[i] = *(uint16_t*)src;
		src += 2;
	}
	return src;
}


uint32_t shapenamehasher(std::string strhashable) {
	std::string shapename;
	char* cstring;
	int begin;
	int strlength;
	uint32_t accumulator;
	uint32_t temp;
	uint32_t charwalker;
	uint32_t tempchar;

	begin = 0;
	shapename = strhashable;
	strlength = shapename.length();
	for (int i = 0; i < strlength; i++) { // skip the | characters 
		if (shapename[i] == 124) {
			begin = i + 1;
		}
	}
	cstring = new char[128];
	for (int i = begin; i < strlength; i++) {
		cstring[i - begin] = shapename[i];
	}
	cstring[strlength - begin] = 0; //append 0 to string 
	accumulator = 5381;
	begin = 0;
	charwalker = uint32_t(cstring[begin]);
	while (charwalker != 0) {
		tempchar = charwalker - 65;
		if (tempchar < 25) {
			charwalker += 32;
		}
		temp = accumulator << 5;
		accumulator += temp;
		accumulator = accumulator ^ charwalker;
		begin++;
		charwalker = uint32_t(cstring[begin]);
	}
	delete[] cstring;
	return accumulator;
}

int write_shapes(shape_struct* shapes, int n_nodes, int n_shapes, int buffer_pos, char* buffer)
{
	int new_pos = buffer_pos;
	int str_len;
	uint32_t* temp_int = (uint32_t*)(&buffer[new_pos]);
	*temp_int = n_shapes;
	new_pos += 4;
	for (int i = 0; i < n_shapes; i++)
	{
		uint32_t* temp_int = (uint32_t*)(&buffer[new_pos]);
		*temp_int = shapes[i].hash;
		str_len = strlen(shapes[i].name);
		strncpy(&buffer[(new_pos + 4)], shapes[i].name, str_len + 1);
		new_pos += 5 + str_len;
		memcpy(&buffer[new_pos], &shapes[i].bounding_box[0], 72);
		new_pos += 72;
	}
	for (int i = 0; i < n_nodes; i++)
	{
		uint32_t* temp_int = (uint32_t*)(&buffer[new_pos]);
		*temp_int = 0xFFFFFFFF;
		new_pos += 24;
	}
	return (new_pos-buffer_pos);
}

void meshset_writer(meshset* meshset_file,char* output_filename, int mesh_type)  
{
	char output_name[128] = { 0 };
	strcpy(&output_name[0], output_filename);
	uint32_t dot_index = strlen(&output_name[0]);
	uint32_t filesize;
	uint32_t* u32_wr = nullptr;
	uint8_t* u8_wr = nullptr;
	char* buffer = (char*)malloc(sizeof(char) * 40000);
	ZeroMemory(buffer, 40000);
	int pos = 5;
	buffer[0] = 0x2A;
	strcpy(&buffer[pos], meshset_file->lods_file);
	pos += strlen(&buffer[pos]) + 1;
	u32_wr = (uint32_t*)&buffer[pos];
	*u32_wr = meshset_file->n_lods;
	pos += 4;
	for (int i = 0; i < meshset_file->n_lods; i++)
	{
		pos++;
		u8_wr = (uint8_t*)&buffer[pos];
		*u8_wr = meshset_file->lods[i].n_shaders_per_set;
		u8_wr++;
		*u8_wr = meshset_file->lods[i].n_shader_sets;
		pos += 2;
		if (meshset_file->lods[i].shader_sets->mesh_dbx_path[0])
		{
			strcpy(&buffer[pos], meshset_file->lods[i].shader_sets->mesh_dbx_path);
			pos += strlen(&buffer[pos]) + 1;
		}
		else
		{
			pos++;
		}
		for (int j = 0; j<meshset_file->lods[i].n_shaders_per_set; j++)
		{
			strcpy(&buffer[pos], meshset_file->lods[i].shader_sets->shaders[j]);
			pos += strlen(&buffer[pos]) + 1;
		}
		memcpy(&buffer[pos], &meshset_file->lods[i].bounding_box[0], 24);
		pos += 24; 
		u32_wr = (uint32_t*)&buffer[pos];
		*u32_wr = meshset_file->lods[i].mesh_size;
		switch (mesh_type)
		{
		case RIGID_MESH:
			pos += 9;
			strcpy(&output_name[dot_index], ".rigidmeshset");
			break;
		case SKINNED_MESH: // to do
			strcpy(&output_name[dot_index], ".skinnedmeshset");
			break;
		case COMPOSITE_MESH: // to write shapes only use lod 0 information. its the same for all lods anyways
			pos += 8;
			u8_wr = (uint8_t*)&buffer[pos];
			*u8_wr = 0x01;
			pos++;
			pos += write_shapes(meshset_file->lods[0].comp_data->shapes, meshset_file->lods[i].n_shaders_per_set, meshset_file->lods[0].comp_data->n_shapes, pos, buffer);
			if (meshset_file->lods[i].comp_data->billboard)
			{
				strcpy(&buffer[pos], meshset_file->lods[i].comp_data->billboard);
				pos += (strlen(meshset_file->lods[i].comp_data->billboard)) + 1;
			}
			strcpy(&output_name[dot_index], ".compositemeshset");
			break;
		case TREE_MESH:
			pos += 8;
			u8_wr = (uint8_t*)&buffer[pos];
			*u8_wr = 0x01;
			pos++;
			pos += write_shapes(meshset_file->lods[0].comp_data->shapes, meshset_file->lods[i].n_shaders_per_set, meshset_file->lods[0].comp_data->n_shapes, pos, buffer);	
			if (meshset_file->lods[i].comp_data->billboard)
			{
				strcpy(&buffer[pos], meshset_file->lods[i].comp_data->billboard);
				pos += (strlen(meshset_file->lods[i].comp_data->billboard)) + 1;
			}
			strcpy(&output_name[dot_index], ".treemeshset");
			break;	
		}
	}
	FILE* meshset = fopen(&output_name[0], "w+b");
	fwrite(&buffer[0], 1, pos, meshset);
	fclose(meshset);
	return;
}

void meshset_to_txt(meshset* meshset_file, char* output_filename, int mesh_type)
{
	uint32_t extra_flag;
	shape_struct* rigid_shape = (shape_struct*)malloc(sizeof(shape_struct));
	char output_name[384] = { 0 };
	strcpy(&output_name[0], output_filename);
	uint32_t dot_index = strlen(&output_name[0]);
	//strcpy(&output_name[dot_index], "_Meshset.txt");
	FILE* file = fopen(&output_name[0], "w");
	fprintf(file, "\n");
	switch (mesh_type)
	{
	case RIGID_MESH:
		fprintf(file, "Asset type: rigid \n\n");
		extra_flag = 0;
		break;
	case SKINNED_MESH: 
		fprintf(file, "Asset type: skinned \n\n");
		extra_flag = 0;
		break;
	case COMPOSITE_MESH:
		fprintf(file, "Asset type: composite \n\n");
		extra_flag = meshset_file->lods[0].comp_data->data[4];
		break;
	case TREE_MESH:
		fprintf(file, "Asset type: tree \n\n");
		extra_flag = meshset_file->lods[0].comp_data->data[4];
		break;
	}
	fprintf(file, "LOD Settings file: %s\n", meshset_file->lods_file);
	fprintf(file, "Number of LODs: %u\n", meshset_file->n_lods);
	fprintf(file, "\n\n");
	fprintf(file, "-----------------------------------------\n\n");
	for (int i = 0; i < meshset_file->n_lods; i++)
	{
		fprintf(file, "LOD %u\n\n", i);
		fprintf(file, "Shaders Per Set: %u \n", meshset_file->lods[i].n_shaders_per_set);
		fprintf(file, "Shaders Sets: %u \n\n", meshset_file->lods[i].n_shader_sets);
		for (int k = 0; k < meshset_file->lods[i].n_shader_sets; k++)
		{
			fprintf(file, "Shader Set %u \n", k);
			fprintf(file, "Mesh DBX settings: %s \n", meshset_file->lods[i].shader_sets[k].mesh_dbx_path);
			for (int j = 0; j < meshset_file->lods[i].n_shaders_per_set; j++)
			{
				fprintf(file, "Shader %u:", j);
				fprintf(file, " %s \n", meshset_file->lods[i].shader_sets[k].shaders[j]);
			}
			fprintf(file, "\n");
		}
		fprintf(file, "Bounding Box:");
		for (int j = 0; j < 6; j++)
		{
			fprintf(file, " %f ", meshset_file->lods[i].bounding_box[j]);
		}
		fprintf(file, "\n");
		fprintf(file, "MeshData Size: %u\n", meshset_file->lods[i].mesh_size);
		fprintf(file, "Unknown Flag: %u\n", extra_flag);
		if (meshset_file->lods[0].comp_data)
		{
			if (meshset_file->lods[0].comp_data->billboard)
			{
				fprintf(file, "Billboard: %s\n", meshset_file->lods[i].comp_data->billboard);
			}
			else
			{
				fprintf(file, "Billboard: \n");
			}
		}
		fprintf(file, "\n\n");
		fprintf(file, "-----------------------------------------\n\n");
	}
	fprintf(file, "Meshset Specific Data: ");
	switch (mesh_type)
	{
	case RIGID_MESH:
		break;
	case SKINNED_MESH:
		fprintf(file, "Bones\n\n");
		break;
	case COMPOSITE_MESH:
	{
		fprintf(file, "Shapes\n");
		fprintf(file, "Number of Shapes: %u\n\n", meshset_file->lods[0].comp_data->n_shapes);
		uint8_t* shape_div = (uint8_t*)malloc(sizeof(char) * meshset_file->lods[0].comp_data->n_shapes);
		memset(shape_div, 0, sizeof(char) * meshset_file->lods[0].comp_data->n_shapes);
		write_shapes_composite(file, shape_div, meshset_file->lods[0].comp_data->n_shapes, meshset_file->lods[0].comp_data->shapes);
		free(shape_div);
		break;
	}
	case TREE_MESH:
	{
		fprintf(file, "Shapes\n");
		fprintf(file, "Number of Shapes: %u\n\n", meshset_file->lods[0].comp_data->n_shapes);
		uint8_t* shape_div = (uint8_t*)malloc(sizeof(char) * meshset_file->lods[0].comp_data->n_shapes);
		memset(shape_div, 0, sizeof(char) * meshset_file->lods[0].comp_data->n_shapes);
		write_shapes_composite(file, shape_div, meshset_file->lods[0].comp_data->n_shapes, meshset_file->lods[0].comp_data->shapes);
		free(shape_div);
		break;
	}
	}
	fclose(file);
	free(rigid_shape);
	return; 
}


void meshset_txt_to_binary(char** lines, char* buffer, char* filename,uint32_t n_lines)
{
	shape_struct* shapes = nullptr;
	uint8_t n_shader_sets;
	uint8_t n_shaders_per_set;
	uint8_t* u8_wr;
	uint8_t flag;
	shader_set* shader_sets;
	float bbox[6];
	uint32_t mesh_size;
	uint32_t line_count = 0;
	uint32_t pos = 0;
	uint32_t n_shapes = 0;
	uint32_t type;
	uint32_t* u32_wr;
	uint16_t* u16_wr;
	char outputname[384] = { 0 };
	char c_lod[8] = { "LOD " };
	strcpy(&outputname[0], filename);
	char* dot_index = strstr(&outputname[0], ".");
	char* txtptr;
	char* txt_end;
	while (!(strstr(lines[line_count], "Asset type:")))
	{
		line_count++;
	}
	if (strstr(lines[line_count], "rigid"))
	{
		strcpy(dot_index, ".rigidmeshset");
		type = RIGID_MESH;
	}
	else if (strstr(lines[line_count], "skinned"))
	{	
		strcpy(dot_index, ".skinnedmeshset");
		type = SKINNED_MESH;
	}
	else if (strstr(lines[line_count], "composite"))
	{
		strcpy(dot_index, ".compositemeshset");
		shapes = get_shapes(lines, 0,&n_shapes);
		type = COMPOSITE_MESH;
	}
	else if (strstr(lines[line_count], "tree"))
	{
		strcpy(dot_index, ".treemeshset");
		shapes = get_shapes(lines, 0, &n_shapes);
		type = TREE_MESH;
	}
	u8_wr = (uint8_t*)&buffer[pos];
	*u8_wr = 0x2A;
	pos += 5;
	while (!(strstr(lines[line_count], "LOD Sett")))
	{
		line_count++;
	}
	txtptr = strstr(lines[line_count], ":") + 1;
	txtptr = skip_space(txtptr);
	txt_end = get_line_end(txtptr);
	strncpy(&buffer[pos], txtptr, (txt_end - txtptr));
	pos += (txt_end - txtptr) + 1;
	line_count++;
	txtptr = strstr(lines[line_count], ":") + 1;
	txtptr = skip_space(txtptr);
	uint32_t n_lods = strtoul(txtptr, 0, 10);
	u32_wr = (uint32_t*)&buffer[pos];
	*u32_wr = n_lods; 
	pos += 4;
	for (int i = 0; i < n_lods; i++)
	{
		sprintf(&c_lod[4], "%u", i);
		while (!(strstr(lines[line_count], &c_lod[0])))
		{
			line_count++;
		}
		line_count += 2;
		pos++;
		txtptr = strstr(lines[line_count], ":") + 1;
		txtptr = skip_space(txtptr);
		n_shaders_per_set = strtoul(txtptr, 0, 10);
		u8_wr = (uint8_t*)&buffer[pos];
		*u8_wr = n_shaders_per_set;
		pos++;
		line_count++;
		txtptr = strstr(lines[line_count], ":") + 1;
		txtptr = skip_space(txtptr);
		n_shader_sets = strtoul(txtptr, 0, 10);
		u8_wr = (uint8_t*)&buffer[pos];
		*u8_wr = n_shader_sets;
		pos++;
		for (int j = 0; j < n_shader_sets; j++)
		{
			while (!(strstr(lines[line_count], "Shader Set ")))
			{
				line_count++;
			}
			line_count++;
			txtptr = strstr(lines[line_count], ":") + 1; // needs testing when meshdbx is set to nothing
			txtptr = skip_space(txtptr);
			txt_end = skip_text(txtptr);
			strncpy(&buffer[pos], txtptr, (txt_end - txtptr));
			pos += (txt_end - txtptr) + 1;
			line_count++;
			for (int k = 0; k < n_shaders_per_set; k++)
			{
				txtptr = strstr(lines[line_count], ":") + 1; 
				txtptr = skip_space(txtptr);
				txt_end = skip_text(txtptr);
				strncpy(&buffer[pos], txtptr, (txt_end - txtptr));
				pos += (txt_end - txtptr) + 1;
				line_count++;
			}

		}
		while (!(strstr(lines[line_count], "Bounding Box")))
		{
			line_count++;
		}
		txtptr = strstr(lines[line_count], ":") + 1;
		for (int j = 0; j < 6; j++)
		{
			txtptr = skip_space(txtptr);
			bbox[j] = strtof(txtptr,0);
			txtptr = skip_text(txtptr);
		}
		memcpy(&buffer[pos], &bbox[0], 24);
		pos += 24;
		line_count++;
		txtptr = strstr(lines[line_count], ":") + 1;
		mesh_size = strtoul(txtptr, 0, 10);
		u32_wr = (uint32_t*)&buffer[pos];
		*u32_wr = mesh_size;
		pos += 8;
		switch (type)
		{
		case RIGID_MESH:
			pos++;
			break;
		case SKINNED_MESH: // print number of bones in place of the extra flag, and then bone data 
			break;
		case COMPOSITE_MESH:
			line_count++;
			txtptr = strstr(lines[line_count], ":") + 1;
			flag = strtoul(txtptr, 0, 10);
			u8_wr = (uint8_t*)&buffer[pos];
			*u8_wr = flag;
			pos++;
			pos += write_shapes(shapes, n_shaders_per_set, n_shapes, pos, buffer);
			line_count++; 
			txtptr = strstr(lines[line_count], ":") + 1;
			txt_end = get_line_end(txtptr);
			if ((txt_end - txtptr) > 5)
			{
				txtptr = skip_space(txtptr);
				txt_end = skip_text(txtptr);
				strncpy(&buffer[pos], txtptr, (txt_end - txtptr));
				pos += (txt_end - txtptr) + 1;
				line_count++;
			}
			break;
		case TREE_MESH:
			line_count++;
			txtptr = strstr(lines[line_count], ":") + 1;
			flag = strtoul(txtptr, 0, 10);
			u8_wr = (uint8_t*)&buffer[pos];
			*u8_wr = flag;
			pos++;
			pos += write_shapes(shapes, n_shaders_per_set, n_shapes, pos, buffer);
			line_count++;
			txtptr = strstr(lines[line_count], ":") + 1;
			txt_end = get_line_end(txtptr);
			if ((txt_end - txtptr) > 5)
			{
				txtptr = skip_space(txtptr);
				txt_end = skip_text(txtptr);
				strncpy(&buffer[pos], txtptr, (txt_end - txtptr));
				pos += (txt_end - txtptr) + 1;
				line_count++;
			}
			break;
		}
	}
	FILE* file = fopen(&outputname[0], "wb");
	fwrite(&buffer[0], 1, pos, file);
	fclose(file);
	if (shapes)
	{
		free(shapes);
	}
	return;
}