#define _CRT_SECURE_NO_WARNINGS
#include "assetc_meshdata_funcs.h"
#include "assetc_vbuf_actuators.h"
#include <map>

uint32_t meshdata_write_header(char* buffer, int buffer_pos, int mesh_type, int n_nodes);
uint32_t meshdata_write_node(meshdata_node* node, char* buffer, int buffer_pos);
void meshdata_node_setup(meshdata* mesh_data);
void meshdata_get_node_geo(node_geometry* node_geo, vxbuffer* vbuffer, meshdata_node* node, int mesh_type);
uint8_t* meshdata_get_ro(char* text, uint32_t* array_size, int n_nodes);
void node_decompressor(node_geometry* node_geo, vxbuffer* vbuffer, meshdata_node* node, int mesh_type);
void indices_compressor(FbxMesh* shape, uint16_t* idx_fbx, int base_val, int idx_buffer_pos, int n_idx, char* buffer);
uint32_t vertex_compressor(FbxMesh* shape, FbxVector4 scale, uint16_t* idx_fbx, vxbuffer* vx_buffer, int shape_index, int tg_uv, int n_vx, int n_idx, int vx_buffer_pos, int mesh_type, char* buffer);
int find_shape(shape_struct* shapes, const char* target, int n_shapes);



void meshdata_to_txt(meshdata* meshdata, char* output_filename)
{
	FILE* file = fopen(&output_filename[0], "w");
	fprintf(file, "\n");
	fprintf(file, "MeshData info\n\n");
	fprintf(file, "Number of Meshdata Nodes: %u\n",meshdata->n_nodes);
	for (int i = 0; i < meshdata->n_nodes; i++)
	{
		fprintf(file, "Node %u: %s\n", i,meshdata->nodes[i].name);
	}
	fprintf(file, "\n");
	write_ro_only(file, meshdata->info);
	fclose(file);
	return;
}


int32_t check_vx_existence(uint16_t* idx_array, uint16_t array_size,uint16_t target)
{
	for (int i = 0; i < array_size; i++)
	{
		if (target == idx_array[i])
		{
			return i;
		}
	}
	return -1;
}

void get_shape_data(shape_geo* shape, meshdata_node* node, node_geometry* node_geo, int shape_index)
{
	shape->n_vx_array = new uint16_t[node->n_vertices];
	shape->indices_in = new uint16_t[node->n_faces * 3];
	shape->indices_out = new uint16_t[node->n_faces * 3];
	uint16_t vx_idx;
	uint16_t count = 0;
	int32_t existence;
	shape->n_vx = 0;
	for (int i = 0; i < (node->n_faces * 3); i++)
	{
		vx_idx = node_geo->indices[i];
		if (node_geo->bone_id[vx_idx][0] == shape_index)
		{
			shape->indices_in[count] = vx_idx;
			existence = check_vx_existence(shape->indices_in, count, vx_idx);
			if (existence == -1)
			{
				shape->indices_out[count] = shape->n_vx;
				shape->n_vx_array[shape->n_vx] = vx_idx;
				shape->n_vx++;
				count++;			
			}
			else
			{
				shape->indices_out[count] = shape->indices_out[existence];
				count++;
			}
		}
	}
	shape->idx_array_size = count;
	return;
}

void get_shape_data_rigid(shape_geo* shape, meshdata_node* node, node_geometry* node_geo, int shape_index)
{
	shape->n_vx_array = new uint16_t[node->n_vertices];
	shape->indices_in = new uint16_t[node->n_faces * 3];
	shape->indices_out = new uint16_t[node->n_faces * 3];
	uint16_t vx_idx;
	uint16_t count = 0;
	int32_t existence;
	shape->n_vx = 0;
	for (int i = 0; i < (node->n_faces * 3); i++)
	{
		vx_idx = node_geo->indices[i];
		shape->indices_in[count] = vx_idx;
		existence = check_vx_existence(shape->indices_in, count, vx_idx);
		if (existence == -1)
		{
			shape->indices_out[count] = shape->n_vx;
			shape->n_vx_array[shape->n_vx] = vx_idx;
			shape->n_vx++;
			count++;
		}
		else
		{
			shape->indices_out[count] = shape->indices_out[existence];
			count++;
		}
	}
	shape->idx_array_size = count;
	return;
}



shape_geo* init_shape_struct()
{
	shape_geo* shape = (shape_geo*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(shape_geo);
	return shape;
}

node_geometry* init_node_geo(meshdata_node* node)
{
	node_geometry* node_geo = (node_geometry*)(&ac_data_pool->pool[ac_data_pool->current_pos]);
	node_geo->start_address = &ac_data_pool->pool[ac_data_pool->current_pos];
	ac_data_pool->current_pos += sizeof(node_geometry);
	/*node_geo->indices = (uint16_t*)(&ac_data_pool->pool[ac_data_pool->current_pos]);
	ac_data_pool->current_pos += sizeof(uint16_t) * (node->n_faces * 3);*/
	node_geo->alloc_size = sizeof(node_geometry);
	return node_geo;
}

void get_shape_bbox(FbxMesh* shape, shape_struct* shape_s)
{
	FbxVector4 bboxmin, bboxmax;
	shape->ComputeBBox();
	bboxmin = shape->BBoxMin;
	bboxmax = shape->BBoxMax;
	shape_s->bounding_box[0] = bboxmin[0];
	shape_s->bounding_box[1] = bboxmin[1];
	shape_s->bounding_box[2] = bboxmin[2];
	shape_s->bounding_box[3] = bboxmax[0];
	shape_s->bounding_box[4] = bboxmax[1];
	shape_s->bounding_box[5] = bboxmax[2];
	return;
}

meshset_lod* mesh_generator_main(FbxNode* root, import_settings* import_data, shape_struct* shapes, char* buffer,int buffer_pos, int n_shapes,int c_lod,char* base_name,uint8_t* shape_div)
{
	char temp[120];
	const char* temp_ptr;
	int name_len,n_nodes,idx_buffer_pos,vx_buffer_pos, header_buffer_pos,ruin_template_index;
	int nodes_loop_ctrl[2] = { 0,0 };
	int* node_indices[2];
	bool find_template = true;
	FbxNode* child;
	FbxMesh* shape_mesh;
	n_nodes = root->GetChildCount();
	// form groups. standard and zonly nodes 
	meshdata_plus_info* ms_info = (meshdata_plus_info*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(meshdata_plus_info);
	meshset_lod* mset_lod = (meshset_lod*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(meshset_lod);
	node_indices[0] = (int*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(int) * n_nodes;
	node_indices[1] = (int*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(int) * n_nodes;
	meshdata_node* ms_nodes = (meshdata_node*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(meshdata_node) * n_nodes;
	ruin_template_index = -1;
	for (int i = 0; i < n_nodes; i++)
	{
		child = root->GetChild(i);
		temp_ptr = child->GetName();
		name_len = strlen(temp_ptr) + 1;
		for (int j = 0; j < name_len; j++)
		{
			temp[j] = tolower(temp_ptr[j]);
		}
		if (!strstr(&temp[0], "zonly"))
		{
			node_indices[0][nodes_loop_ctrl[0]] = i;
			nodes_loop_ctrl[0]++; // normal nodes
		}
		else
		{
			node_indices[1][nodes_loop_ctrl[1]] = i;
			nodes_loop_ctrl[1]++; // zonly nodes 
		}
        if ((strstr(&temp[0], "_template")) && (find_template))
		{
			ruin_template_index = i;
			find_template = false;
		}
		ZeroMemory(&temp[0], 120);
	}
	wchar_t uv_text[2];
	ComboBox_GetText(import_data->lods_settings_hwnds[c_lod].tg_select_combo, uv_text,2);
	int node_vx_count,node_idx_count,g_idx_count,poly_type,tg_uv,vx_start, idx_start, g_vx_pos;
	tg_uv = _wtoi(&uv_text[0]);
	//header_buffer_pos = buffer_pos;
	header_buffer_pos = buffer_pos;
	vx_buffer_pos = buffer_pos+ 10000;
	idx_buffer_pos = buffer_pos +10000000;
	vx_start = vx_buffer_pos;
	idx_start = idx_buffer_pos;
	header_buffer_pos += meshdata_write_header(buffer,header_buffer_pos, import_data->mesh_type,n_nodes);
	uint16_t* idx_fbx = nullptr; // the indices as they come from the shape, not the ones that go into the meshdata. this starts and dies after each iteration
	g_idx_count = 0;
	g_vx_pos = 0;
	FbxVector4 bboxmin, bboxmax, bboxcenter,scale;
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < nodes_loop_ctrl[i]; j++)
		{
			child = root->GetChild(node_indices[i][j]);
			ms_nodes[node_indices[i][j]].name = const_cast<char*>(child->GetName());
			node_vx_count = 0;
			node_idx_count = 0;
			if ((import_data->mesh_type == COMPOSITE_MESH) || (import_data->mesh_type == TREE_MESH))
			{
				int shape_vx_count, shape_idx_count,shape_index;
				int node_n_shapes = child->GetChildCount();
				const char* shape_name;
				ms_nodes[node_indices[i][j]].shape_indices = (uint16_t*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
				ac_structs_pool->current_pos += sizeof(uint16_t) * node_n_shapes;
				for (int k = 0; k < node_n_shapes; k++)
				{
					shape_mesh = child->GetChild(k)->GetMesh();
					shape_name = child->GetChild(k)->GetName();
					if (apply_scale)
					{
						scale = child->GetChild(k)->LclScaling;
					}
					else
					{
						scale[0] = 1.0;
						scale[1] = 1.0;
						scale[2] = 1.0;
					}
					shape_index = find_shape(shapes,shape_name,n_shapes);
					if ((i == 0) && (node_indices[i][j] != ruin_template_index))
					{
						shape_div[shape_index]++;
					}
					get_shape_bbox(shape_mesh, &shapes[shape_index]);
					shape_vx_count = shape_mesh->GetControlPointsCount();
					shape_idx_count = shape_mesh->GetPolygonCount() * 3;
					idx_fbx = (uint16_t*)malloc(shape_idx_count * sizeof(uint16_t));
					indices_compressor(shape_mesh,idx_fbx,node_vx_count,idx_buffer_pos,shape_idx_count,buffer);
                    vx_buffer_pos += vertex_compressor(shape_mesh,scale,idx_fbx, &import_data->vx_buffers[c_lod][i], shape_index,tg_uv, shape_vx_count, shape_idx_count, vx_buffer_pos, import_data->mesh_type,buffer);
					node_vx_count += shape_vx_count;
					node_idx_count += shape_idx_count;
					idx_buffer_pos += shape_idx_count * 2;
					ms_nodes[node_indices[i][j]].shape_indices[k] = shape_index;
				}
				ms_nodes[node_indices[i][j]].bones_per_vx = 1;
				ms_nodes[node_indices[i][j]].shapes_in_node = node_n_shapes;
				ms_nodes[node_indices[i][j]].shapes_in_node2 = node_n_shapes;
			}
			else if (import_data->mesh_type == RIGID_MESH)
			{
				int shape_vx_count, shape_idx_count;
				const char* shape_name;
				shape_mesh = child->GetChild(0)->GetMesh();
				if (apply_scale)
				{
					scale = child->GetChild(0)->LclScaling;
				}
				else
				{
					scale[0] = 1.0;
					scale[1] = 1.0;
					scale[2] = 1.0;
				}
				shape_vx_count = shape_mesh->GetControlPointsCount();
				shape_idx_count = shape_mesh->GetPolygonCount() * 3;
				idx_fbx = (uint16_t*)malloc(shape_idx_count * sizeof(uint16_t));
				indices_compressor(shape_mesh, idx_fbx, node_vx_count, idx_buffer_pos, shape_idx_count, buffer);
				vx_buffer_pos += vertex_compressor(shape_mesh,scale, idx_fbx, &import_data->vx_buffers[c_lod][i], 0, tg_uv, shape_vx_count, shape_idx_count, vx_buffer_pos, import_data->mesh_type, buffer);
				node_vx_count += shape_vx_count;
				node_idx_count += shape_idx_count;
				idx_buffer_pos += shape_idx_count * 2;
				ms_nodes[node_indices[i][j]].bones_per_vx = 0;
				ms_nodes[node_indices[i][j]].shapes_in_node = 0;
				ms_nodes[node_indices[i][j]].shapes_in_node2 = 0;
			}
			ms_nodes[node_indices[i][j]].primitive_type = 3;
			ms_nodes[node_indices[i][j]].vertex_stride = *import_data->vx_buffers[c_lod][i].vx_stride;
			ms_nodes[node_indices[i][j]].indices_start = g_idx_count;
			ms_nodes[node_indices[i][j]].vertices_start = g_vx_pos;
			ms_nodes[node_indices[i][j]].n_vertices = node_vx_count;
			g_idx_count += node_idx_count;
			ms_nodes[node_indices[i][j]].n_faces = node_idx_count / 3;
			header_buffer_pos += meshdata_write_node(&ms_nodes[node_indices[i][j]],buffer,header_buffer_pos);
			g_vx_pos += node_vx_count * ms_nodes[node_indices[i][j]].vertex_stride;
			free(idx_fbx);		
		}	
	}
	mset_lod->mesh_size = (g_idx_count * 2) + (vx_buffer_pos - vx_start);
	char ro_temp[40];
	uint32_t* temp_int;
	for (int i = 0; i < 4; i++) // write render order 
	{
		GetWindowTextA(import_data->lods_settings_hwnds[c_lod].ro_vals[i], ro_temp,40);
		ms_info->ro_vals[i] = meshdata_get_ro(ro_temp,&ms_info->ro[i],n_nodes);
		temp_int = (uint32_t*)(&buffer[header_buffer_pos]);
		*temp_int = ms_info->ro[i];
		memcpy(&buffer[(header_buffer_pos + 4)], ms_info->ro_vals[i], ms_info->ro[i] * sizeof(uint8_t));
		header_buffer_pos += 4 + ms_info->ro[i];
	}
	temp_int = (uint32_t*)(&buffer[header_buffer_pos]);
	*temp_int = vx_buffer_pos - vx_start;
	temp_int = (uint32_t*)(&buffer[(header_buffer_pos+4)]);
	*temp_int = g_idx_count * 2;
	header_buffer_pos += 12;
	ZeroMemory(&temp[0], 120);
	strcpy(&temp[0], base_name);
	name_len = strlen(&temp[0]);
	strcpy(&temp[name_len], "_Mesh_lod");
	name_len += 9;
	sprintf(&temp[name_len], "%u", c_lod);
	name_len += 1;
	strcpy(&temp[name_len], "_data");
	name_len += 5;
	strcpy(&temp[name_len], ".meshdata");
	FILE* meshdata_file = fopen(&temp[0], "w+b");
	fwrite(&buffer[buffer_pos], 1, (header_buffer_pos - buffer_pos), meshdata_file);
	fwrite(&buffer[vx_start], 1, (vx_buffer_pos - vx_start), meshdata_file);
	fwrite(&buffer[idx_start], 1, g_idx_count * 2, meshdata_file);
	fclose(meshdata_file);
	return mset_lod;
}

uint8_t* meshdata_get_ro(char* text, uint32_t* array_size,int n_nodes)
{
	int text_length = strlen(text);
	int i = 0;
	int n_vals = 0;
	uint8_t value;
	uint8_t* ro_array = (uint8_t*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(char)*n_nodes;
	while( i < text_length)
	{
		if (text[i]>57)
		{
			MessageBoxA(hwndImportTab, "LETTERS NOT ALLOWED IN RENDER ORDER BOX! MESHDATA WILL BE CORRUPT", "ERROR", MB_OK);
			return 0;
		}
		else if (text[i] < 48 )
		{
			i++;
		}
		else
		{
			value = atoi(&text[i]);
			ro_array[n_vals] = value;
			n_vals++;
			while ((text[i] > 47) && (text[i] < 58))
			{
				i++;
			}
		}
	}
	*array_size = n_vals;
	return ro_array;
}


uint32_t meshdata_write_header(char* buffer, int buffer_pos, int mesh_type, int n_nodes)
{
	uint32_t new_pos = buffer_pos;
	uint8_t* byte = nullptr;
	switch (mesh_type)
	{
	case RIGID_MESH:
		new_pos += 6;
		break;
	case SKINNED_MESH:
		byte = (uint8_t*)(&buffer[new_pos]);
		*byte = SKINNED_MESH;
		new_pos += 6;
		break;
	case COMPOSITE_MESH:
		byte = (uint8_t*)(&buffer[new_pos]);
		*byte = COMPOSITE_MESH;
		new_pos += 6;
		break;
	case TREE_MESH:
		byte = (uint8_t*)(&buffer[new_pos]);
		*byte = TREE_MESH;
		new_pos += 6;
		break;
	}
	byte = (uint8_t*)(&buffer[new_pos]);
	*byte = n_nodes;
	return 7;
}

uint32_t meshdata_write_node(meshdata_node* node, char* buffer, int buffer_pos)
{
	uint32_t new_pos = buffer_pos;
	strcpy(&buffer[new_pos], node->name);
	new_pos += strlen(node->name) + 1;
	memcpy(&buffer[new_pos], &node->n_faces, 22);
	new_pos += 22;
	memcpy(&buffer[new_pos],&node->shape_indices[0], node->shapes_in_node*sizeof(uint16_t));
	new_pos += node->shapes_in_node * sizeof(uint16_t);
	return (new_pos-buffer_pos);
}

void indices_compressor(FbxMesh* shape, uint16_t* idx_fbx, int base_val,int idx_buffer_pos, int n_idx,char* buffer)
{
	uint16_t index;
	uint16_t* idx;
	idx = (uint16_t*)(&buffer[idx_buffer_pos]);
	int new_pos = idx_buffer_pos;
	for (int i = 0; i < n_idx/3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			index = (shape->GetPolygonVertex(i,j));
			idx_fbx[i*3+j] = index;
			*idx = index + base_val;
			idx = (uint16_t*)(&buffer[new_pos+=2]);
		}
	}
	return;
}

int find_vx(uint16_t* idx_array, int idx_size, uint16_t target)
{
	int i = 0;
	while ((idx_array[i] != target) && (i < idx_size))
	{
		i++;	
	}
	return i;
}

uint32_t vertex_compressor(FbxMesh* shape,FbxVector4 scale, uint16_t* idx_fbx,vxbuffer* vx_buffer, int shape_index, int tg_uv, int n_vx, int n_idx, int vx_buffer_pos, int mesh_type,char* buffer)
{
	std::map<int, int> vbuf_types;
	int internal_vals[12] = { 263,259,1032,1288,1797,6,524,780,264,8,4,12 };
	char* current_vertex;
	FbxLayer* normals_layer = nullptr;
	FbxLayer* tangents_layer = nullptr;
	FbxLayerElement* normals_element;
	FbxArray<FbxVector4> normals_array;
	FbxArray<FbxVector2> uv_channels[5];
	FbxLayerElement* tangents_array;
	uint8_t bone_ids[4];
	for (int i = 0; i < 12; i++)
	{
		vbuf_types[internal_vals[i]] = (i + 1);
	}
	short type;
	bool use_normals = false;
	bool use_tangents = false;
	bool use_bones = false;
	int n_uv_chnl = 0;
	dx_data* dxdata = (dx_data*)malloc(sizeof(dx_data));
	FbxVector4* vertices = shape->GetControlPoints();
	for (int j = 0; j < n_vx; j++)
	{
		dxdata->verts[j].x = vertices[j][0];
		dxdata->verts[j].y = vertices[j][1];
		dxdata->verts[j].z = vertices[j][2];
	}
	for (int i = 0; i < *vx_buffer->n_data_types; i++)
	{
		type = *vx_buffer->types_array[i];
		if ((!use_normals) && (vbuf_types[type] == 3))
		{
			use_normals = true;
			if (recompute_normals) // this and tangnets need to be verified to see if it works with all mapping modes and reference modes. 
			{
				//int map_mode, ref_mode;
				//normals_element = shape->GetLayer(0)->GetLayerElementOfType(FbxLayerElement::eNormal);
				//map_mode = normals_element->GetMappingMode();
				//ref_mode = normals_element->GetReferenceMode();
				normals_array.Grow(n_vx);
				DirectX::ComputeNormals(idx_fbx, n_idx/3, dxdata->verts, n_vx, DirectX::CNORM_DEFAULT, dxdata->normals);
				for (int j = 0; j < n_vx; j++)
				{
					normals_array[j][0] = dxdata->normals[j].x;
				    normals_array[j][1] = dxdata->normals[j].y;
					normals_array[j][2] = dxdata->normals[j].z;
				}
			}
		}
		if ((!use_tangents) && (vbuf_types[type] == 4))
		{
			if (!recompute_normals)
			{
				 shape->GetPolygonVertexNormals(normals_array);

			}
			use_tangents = true;
			int idx_dist;
			FbxVector4* vertices = shape->GetControlPoints();
			const char* uv_name = shape->GetLayer(tg_uv)->GetLayerElementOfType(FbxLayerElement::eUV)->GetName();
			FbxArray<FbxVector2> uv_array;
			shape->GetPolygonVertexUVs(uv_name,uv_array);		
			for (int j = 0; j < n_vx; j++)
			{
				if (recompute_normals)
				{ 
					idx_dist = j;
				}
				else
				{
					idx_dist = find_vx(idx_fbx, n_idx, i);
				}			
				dxdata->normals[j].x = normals_array[idx_dist][0];
				dxdata->normals[j].y = normals_array[idx_dist][1];
				dxdata->normals[j].z = normals_array[idx_dist][2];
				dxdata->dxuvs[j].x = uv_array[idx_dist][0];
				dxdata->dxuvs[j].y = uv_array[idx_dist][1]*-1;
			}
			DirectX::ComputeTangentFrame(idx_fbx,n_idx/3,dxdata->verts,dxdata->normals,dxdata->dxuvs,n_vx,dxdata->tangents);
		}
		if ((!use_bones) && (vbuf_types[type] == 7))
		{
			use_bones = true;
			if ((mesh_type == COMPOSITE_MESH) || (mesh_type == TREE_MESH))
			{
				for (int j = 0; j < 4; j++)
				{
					bone_ids[j] = shape_index;
				}
			}
		}
		if (vbuf_types[type] == 6)
		{
			const char* uv_name = shape->GetLayer(n_uv_chnl)->GetLayerElementOfType(FbxLayerElement::eUV)->GetName();
			shape->GetPolygonVertexUVs(uv_name, uv_channels[n_uv_chnl]);
			n_uv_chnl++;
		}
	}
	int vx_pos, current_uv, idx_dist;
	float vec[4];
	FbxVector4 fbx_vec4;
	FbxVector2 fbx_vec2;
	for (int i = 0; i < n_vx; i++)
	{
		vx_pos = vx_buffer_pos + (i * (*vx_buffer->vx_stride));
		current_uv = 0;
		for (int j = 0; j < *vx_buffer->n_data_types; j++)
		{
			type = *vx_buffer->types_array[j];
			switch (vbuf_types[type])
			{
			case 1: // vx coords 16 bit float
				fbx_vec4 = shape->GetControlPointAt(i);
				vec[0] = fbx_vec4[0]*scale[0];
				vec[1] = fbx_vec4[1]*scale[1];
				vec[2] = fbx_vec4[2]*scale[2];
				vx_pos += vec3_fp16_compressor(buffer, vx_pos,vec);
				break;
			case 2: // vx coords 32 bit float
				fbx_vec4 = shape->GetControlPointAt(i);
				vec[0] = fbx_vec4[0];
				vec[1] = fbx_vec4[1];
				vec[2] = fbx_vec4[2];
				vx_pos += vec3_fp32_compressor(buffer, vx_pos, vec);
				break;
			case 3:  // normals 
				if (recompute_normals)
				{
					idx_dist = i;
				}
				else
				{
					idx_dist = find_vx(idx_fbx, n_idx, i);
				}
				vec[0] = normals_array[idx_dist][0];
				vec[1] = normals_array[idx_dist][1];
				vec[2] = normals_array[idx_dist][2];
				vec[3] = 1;
				vx_pos += vec4_fp16_compressor(buffer, vx_pos, vec);
				break;
			case 4: // tangents
				vec[0] = dxdata->tangents[i].x;
				vec[1] = dxdata->tangents[i].y;
				vec[2] = dxdata->tangents[i].z;
				//vec[3] = dxdata->tangents[i].w;
				vec[3] = 1;
				vx_pos += vec4_fp16_compressor(buffer, vx_pos, vec);
				break;
			case 5: // 1 or -1 value. 4th component
				vx_pos += half_compressor(buffer, vx_pos, 1);
 				break;
			case 6: // uv channel or any 2d vector fp16
				idx_dist = find_vx(idx_fbx, n_idx, i);
				vec[0] = uv_channels[current_uv][idx_dist][0];
				vec[1] = uv_channels[current_uv][idx_dist][1]*-1;
				vx_pos += vec2_fp16_compressor(buffer, vx_pos,vec);
				current_uv++;
				break;
			case 7: // bone / shpe index. 4 bytes
				vx_pos += uint8_rgba_compressor(buffer,vx_pos,bone_ids);
				break;
			case 8:  // bone weights. 4 bytes. for now unsupported
				vx_pos += uint8_rgba_compressor(buffer, vx_pos, bone_ids);
				break;
			case 9: // vx coords + 4th component which is always 1
				fbx_vec4 = shape->GetControlPointAt(i);
				vec[0] = fbx_vec4[0];
				vec[1] = fbx_vec4[1];
				vec[2] = fbx_vec4[2];
				vec[3] = 1;
				vx_pos += vec4_fp16_compressor(buffer, vx_pos, vec);
				break;
			case 10: // null vector
				vx_pos += 6;
				vx_pos += half_compressor(buffer, vx_pos, 1);
				break;
			case 11: // unknown
				vx_pos += 16;
				break;
			case 12: // probably vertex color, but the engine doesnt "color" the vertex, probably used for something else
				vx_pos += 4;
				break;
			}		
		}
	}
	char* finish = &buffer[vx_pos];
	free(dxdata);
    return(n_vx*(*vx_buffer->vx_stride));
}

void mesh_extractor_main(FbxManager* pSdkManager, FbxScene* pScene,meshdata* mesh_data, meshset* mesh_set, vxbuffer* vbuffer, int mesh_type, int current_lod,char* file_name,uint8_t* shapes_subdivision)
{
	char temp[120];
	int name_len, ruin_template_index;
	int nodes_loop_ctrl[2] = { 0,0 };
	int* nodes_indices[2];
	meshdata_node* groups[2];
	vxbuffer* active_vbuffer = nullptr;
	bool find_template = true;
	// form groups. standard and zonly nodes 
	nodes_indices[0] = (int*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(int) * mesh_data->n_nodes;
	nodes_indices[1] = (int*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(int) * mesh_data->n_nodes;
	ruin_template_index = -1;
	for (int i = 0; i < mesh_data->n_nodes; i++)
	{
		name_len = strlen(mesh_data->nodes[i].name) + 1;
		for (int j = 0; j < name_len; j++)
		{
			temp[j] = tolower(mesh_data->nodes[i].name[j]);
		}
		if (!strstr(&temp[0], "zonly"))
		{
			nodes_indices[0][nodes_loop_ctrl[0]] = i;
			nodes_loop_ctrl[0]++; // normal nodes
		}
		else
		{
			nodes_indices[1][nodes_loop_ctrl[1]] = i;
			nodes_loop_ctrl[1]++; // zonly nodes 
		}
		if ((strstr(&temp[0], "_template")) && (find_template))
		{
			ruin_template_index = i;
			find_template = false;
		}
	}
	groups[0] = (meshdata_node*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(meshdata_node) * nodes_loop_ctrl[0];
	groups[1] = (meshdata_node*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(meshdata_node) * nodes_loop_ctrl[1];
	// some fbx boilerplate here
		// create scene info
	FbxDocumentInfo* sceneInfo = FbxDocumentInfo::Create(pSdkManager, "SceneInfo");
	sceneInfo->mTitle = "Example scene";
	sceneInfo->mSubject = "Illustrates the creation and animation of a deformed cylinder";
	sceneInfo->mAuthor = "ExportScene01.exe sample program.";
	sceneInfo->mRevision = "rev. 1.0";
	sceneInfo->mKeywords = "deformed cylinder";
	sceneInfo->mComment = "no particular comments required.";

	// we need to add the sceneInfo before calling AddThumbNailToScene because
	// that function is asking the scene for the sceneInfo.
	pScene->SetSceneInfo(sceneInfo);

	FbxNode* lRootNode = pScene->GetRootNode();
	//
	int n_shapes;
	int shape_index;
	char* shape_name;
	float bounding_box[6];
	float x_pos, y_pos, z_pos; // 9,10,11 are shape position in game 
	node_geometry* node_geo = nullptr;
	shape_geo* shape = nullptr;
	FbxMesh* fbx_mesh;
	FbxSurfacePhong* material;
	for (int i = 0; i < 2; i++) // groups level. this loop wont work with skinned meshes. we must use a different one for that type of mesh. 
	{
		active_vbuffer = &vbuffer[i];
		for (int j = 0; j < nodes_loop_ctrl[i]; j++) // nodes level 
		{
			groups[i][j] = mesh_data->nodes[nodes_indices[i][j]];
			char* node_name = groups[i][j].name;
			FbxNode* meshdata_node = CreateNode(pScene, node_name);
			lRootNode->AddChild(meshdata_node);
			material = CreateMaterial(pScene, mesh_set, nodes_indices[i][j], current_lod);
			if (!node_geo)
			{
				node_geo = init_node_geo(&groups[i][j]);
				node_geo->indices = new uint16_t[groups[i][j].n_faces * 3];
				//meshdata_get_node_geo(node_geo, &vbuffer[i], &groups[i][j], 0);
				node_decompressor(node_geo, &vbuffer[i], &groups[i][j], 0);
			}
			else
			{
				char* start_address = node_geo->start_address;
				int node_geo_size = node_geo->alloc_size;
				ZeroMemory(node_geo->start_address, node_geo->alloc_size);
				node_geo->start_address = start_address;
				node_geo->alloc_size = node_geo_size;
				node_geo->indices = new uint16_t[groups[i][j].n_faces * 3];
				//meshdata_get_node_geo(node_geo, &vbuffer[i], &groups[i][j], 0); 
				node_decompressor(node_geo, &vbuffer[i], &groups[i][j], 0);
			}
			if (!shape)
			{
				shape = init_shape_struct();
			}		
			if (mesh_type == RIGID_MESH) // rigid mesh
			{
				n_shapes = 1;
			}
			else
			{
				n_shapes = groups[i][j].shapes_in_node2;
			}
			for (int k = 0; k < n_shapes; k++)  // shapes level
			{
				shape_index = groups[i][j].shape_indices[k];
				if (mesh_type == RIGID_MESH)
				{
					// create geometry node here. no sub shapes needed, use the node_geo directly 
					get_shape_data_rigid(shape, &groups[i][j], node_geo, shape_index);
					shape_name = groups[i][j].name;
					x_pos = 0;
					y_pos = 0;
					z_pos = 0;
					FbxNode* fbx_shape = create_composite_mesh(pScene, shape_name, shape, node_geo, nodes_indices[i][j]);
					fbx_shape->LclTranslation.Set(FbxVector4(double(x_pos), double(y_pos), double(z_pos)));
					fbx_mesh = fbx_shape->GetMesh();
					//lRootNode->GetChild(nodes_indices[i][j]);
					meshdata_node->AddChild(fbx_shape);
					fbx_shape = fbx_mesh->GetNode();
					fbx_shape->AddMaterial(material);
					delete[] shape->n_vx_array;
					delete[] shape->indices_in;
					delete[] shape->indices_out;
					shape->idx_array_size = 0;
					shape->n_vx = 0;
				}
				else
				{
					// create shapes here
					get_shape_data(shape, &groups[i][j], node_geo, shape_index);
					if ((i == 0) && (nodes_indices[i][j] != ruin_template_index))
					{
						shapes_subdivision[shape_index]++;
					}
					shape_name = mesh_set->lods[current_lod].comp_data->shapes[shape_index].name;
					x_pos = mesh_set->lods[current_lod].comp_data->shapes[shape_index].transforms[9];
					y_pos = mesh_set->lods[current_lod].comp_data->shapes[shape_index].transforms[10];
					z_pos = mesh_set->lods[current_lod].comp_data->shapes[shape_index].transforms[11];
					FbxNode* fbx_shape = create_composite_mesh(pScene, shape_name, shape, node_geo, nodes_indices[i][j]);
					fbx_shape->LclTranslation.Set(FbxVector4(double(x_pos), double(y_pos), double(z_pos)));
					fbx_mesh = fbx_shape->GetMesh();
					//lRootNode->GetChild(nodes_indices[i][j]);
					meshdata_node->AddChild(fbx_shape);
					fbx_shape = fbx_mesh->GetNode();
					fbx_shape->AddMaterial(material);
					delete[] shape->n_vx_array;
					delete[] shape->indices_in;
					delete[] shape->indices_out;
					shape->idx_array_size = 0;
					shape->n_vx = 0;
				}			
			}
		    delete[] node_geo->indices;
		}
	}
	const char* lSampleFileName = file_name;
	bool lResult = SaveScene(pSdkManager, pScene, lSampleFileName);
	if (lResult == FALSE)
	{
		MessageBoxA(hwndAsset, "FBX SAVE ERROR", "ERROR", MB_OK);
	}
	return;
}

meshdata* meshdata_reader(char* src)
{
	meshdata* mesh_data = (meshdata*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(meshdata);
	mesh_data->type = *(uint32_t*)src;
	mesh_data->unknown = *(uint16_t*)(src + 4);
	mesh_data->n_nodes = *(src + 6);
	src += 7;
	mesh_data->nodes = (meshdata_node*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(meshdata_node) * mesh_data->n_nodes;
	mesh_data->info = (meshdata_plus_info*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
	ac_structs_pool->current_pos += sizeof(meshdata_plus_info);
	for (int i = 0; i < mesh_data->n_nodes; i++)
	{
		mesh_data->nodes[i].name = src;
		src += strlen(src) + 1;
		mesh_data->nodes[i].n_faces = *(uint32_t*)src;
		mesh_data->nodes[i].n_vertices = *(uint32_t*)(src + 4);
		mesh_data->nodes[i].indices_start = *(uint32_t*)(src + 8);
		mesh_data->nodes[i].vertices_start = *(uint32_t*)(src + 12);
		src += 16;
		mesh_data->nodes[i].vertex_stride = *src;
		mesh_data->nodes[i].primitive_type = *(src + 1);
		mesh_data->nodes[i].bones_per_vx = *(src + 2);
		mesh_data->nodes[i].shapes_in_node = *(src + 3);
		src += 4;
		mesh_data->nodes[i].shapes_in_node2 = *(uint16_t*)src;
		src += 2;
		mesh_data->nodes[i].shape_indices = (uint16_t*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
		ac_structs_pool->current_pos += sizeof(uint16_t*) * mesh_data->nodes[i].shapes_in_node2;
		for (int j = 0; j < mesh_data->nodes[i].shapes_in_node2; j++)
		{
			mesh_data->nodes[i].shape_indices[j] = *(uint16_t*)src;
			src += 2;
		}
	}
	for (int i = 0; i < 4; i++)
	{
		mesh_data->info->ro[i] = *(uint32_t*)src;
		src += 4;
		mesh_data->info->ro_vals[i] = (uint8_t*)(&ac_structs_pool->pool[ac_structs_pool->current_pos]);
		ac_structs_pool->current_pos += sizeof(uint8_t*) * mesh_data->info->ro[i];
		for (int j = 0; j < mesh_data->info->ro[i]; j++)
		{
			mesh_data->info->ro_vals[i][j] = *src;
			src += 1;
		}
	}
	mesh_data->info->vbuffer_size = *(uint32_t*)src;
	mesh_data->info->indices_size = *(uint32_t*)(src + 4);
	mesh_data->info->null = *(uint32_t*)(src + 8);
	src += 12;
	mesh_data->vx_buffer_start = src;
	mesh_data->idx_buffer_start = (src + mesh_data->info->vbuffer_size);
	meshdata_node_setup(mesh_data);
	return mesh_data;
}

void meshdata_node_setup(meshdata* mesh_data)
{
	for (int i = 0; i < mesh_data->n_nodes; i++)
	{
		mesh_data->nodes[i].idx_buffer_pos = mesh_data->idx_buffer_start + (mesh_data->nodes[i].indices_start*2);
		mesh_data->nodes[i].vx_buffer_pos = mesh_data->vx_buffer_start + mesh_data->nodes[i].vertices_start;
	}
	return;
}


void node_decompressor(node_geometry* node_geo, vxbuffer* vbuffer, meshdata_node* node, int mesh_type) // generic function to get the data in a more readable format
{
	std::map<int, int> vbuf_types;
	int internal_vals[12] = { 263,259,1032,1288,1797,6,524,780,264,8,4,12 };
	char* current_vertex;
	for (int i = 0; i < 12; i++)
	{
		vbuf_types[internal_vals[i]] = (i + 1);
	}
	short type;
	bool stay1 = TRUE;
	bool stay2 = TRUE;
	for (int i = 0; i < *vbuffer->n_data_types; i++)
	{
		type = *vbuffer->types_array[i];
		if ((stay1) && (vbuf_types[type] == 3))
		{
			node_geo->export_normals = TRUE;
			stay1 = FALSE;
		}
		if ((stay2) && (vbuf_types[type] == 3))
		{
			node_geo->export_vx_colors = TRUE;
			stay2 = FALSE;
		}
	}
	uint16_t* n_vx_array = (uint16_t*)malloc((node->n_vertices)*sizeof(uint16_t));
	uint16_t* indices_in = (uint16_t*)malloc((node->n_faces*3) * sizeof(uint16_t));
	uint16_t vx_idx;
	uint16_t unique_count = 0;
	int32_t existence;
	uint16_t n_vx = 0;
	uint32_t current_uv;
	for (int i = 0; i < (node->n_faces * 3); i++)
	{
		node_geo->indices[i] = *(uint16_t*)node->idx_buffer_pos;
		node->idx_buffer_pos += 2;
		vx_idx = node_geo->indices[i];
		indices_in[unique_count] = vx_idx;
		existence = check_vx_existence(indices_in, unique_count, vx_idx);
		if (existence == -1)
		{
			n_vx_array[n_vx] = vx_idx;
			current_uv = 0;
			current_vertex = node->vx_buffer_pos + (node->vertex_stride * vx_idx);
			for (int j = 0; j < *vbuffer->n_data_types; j++)
			{
				type = *vbuffer->types_array[j];
				switch (vbuf_types[type])
				{
				case 1: // vx coords 16 bit float
					current_vertex += vec3_fp16_extractor(current_vertex, &node_geo->vx_coords[vx_idx * 3]);
					break;
				case 2: // vx coords 32 bit float
					current_vertex += vec3_fp32_extractor(current_vertex, &node_geo->vx_coords[vx_idx * 3]);
					break;
				case 3:  // normals 
					current_vertex += vec3_fp16_extractor(current_vertex, &node_geo->normals[vx_idx * 3]);
					current_vertex += 2;
					break;
				case 4: // tangents
					current_vertex += vec3_fp16_extractor(current_vertex, &node_geo->tangents[vx_idx * 3]);
					current_vertex += 2;
					break;
				case 5: // 1 or -1 value. 4th component
					current_vertex += 2;
					break;
				case 6: // uv channel or any 2d vector fp16
					current_vertex += vec2_fp16_extractor(current_vertex, &node_geo->uv_channels[current_uv][vx_idx * 2]);
					current_uv++;
					break;
				case 7: // bone / shpe index. 4 bytes
					current_vertex += uint8_rgba_extractor(current_vertex, &node_geo->bone_id[vx_idx][0]);
					break;
				case 8:  // bone weights. 4 bytes
					current_vertex += uint8_rgba_extractor(current_vertex, &node_geo->bone_w[vx_idx][0]);
					break;
				case 9: // vx coords + 4th component which is always 1
					current_vertex += vec3_fp16_extractor(current_vertex, &node_geo->vx_coords[vx_idx * 3]);
					current_vertex += 2; // account for the extra value which is not used on extraction
					break;
				case 10: // null vector
					current_vertex += 8;
					break;
				case 11: // unknown
					current_vertex += 16;
					break;
				case 12: // probably vertex color, but the engine doesnt "color" the vertex, probably used for something else
					current_vertex += 4;
					break;
				}
			}
			n_vx++;
			unique_count++;
		}
	}
	node_geo->n_uvs = current_uv;
	free (n_vx_array);
	free (indices_in);
	return;
}



int find_shape(shape_struct* shapes, const char* target, int n_shapes)
{
	int shape;
	int length;
	char temp[200] = { 0 };
	const char* dotptr;
	dotptr = strstr(target, ".");
	if (dotptr)
	{
		length = dotptr - target;
	}
	else
	{
		length = strlen(target);
	}
	strncpy(&temp[0], target, length);
	for (int i = 0; i < n_shapes; i++)
	{
		if (strstr(shapes[i].name, &temp[0]))
		{
			return i;
		}
	}
	return -1;
}







