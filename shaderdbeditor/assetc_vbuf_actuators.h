#pragma once
#include "assetc.h"
#include "assetc_structs.h"
//#include "DirectXMath/DirectXMath.h"
//#include "DirectXMath/DirectXPackedVector.h"

uint32_t vec3_fp16_extractor(char* vxbuffer, float* coords_array);
uint32_t vec2_fp16_extractor(char* vxbuffer, float* coords_array);
uint32_t vec3_fp32_extractor(char* vxbuffer, float* coords_array);
uint32_t vec4_fp16_extractor(char* vxbuffer, float* coords_array);
uint32_t uint8_rgba_extractor(char* vxbuffer, uint8_t* rgba_array);
uint32_t vec3_fp16_compressor(char* buffer, int vx_buffer_pos, float* vec3);
uint32_t vec2_fp16_compressor(char* buffer, int vx_buffer_pos, float* vec2);
uint32_t vec4_fp16_compressor(char* buffer, int vx_buffer_pos, float* vec4);
uint32_t vec3_fp32_compressor(char* buffer, int vx_buffer_pos, float* vec3);
uint32_t uint8_rgba_compressor(char* buffer, int vx_buffer_pos, uint8_t* rgba_array);
uint32_t half_compressor(char* buffer, int vx_buffer_pos, float val);