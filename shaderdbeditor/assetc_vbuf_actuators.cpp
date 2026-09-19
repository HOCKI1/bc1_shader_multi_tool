#define _CRT_SECURE_NO_WARNINGS
#include "assetc_vbuf_actuators.h"

uint32_t vec3_fp16_extractor(char* vxbuffer, float* coords_array)
{   
    uint16_t half = *(uint16_t*)vxbuffer;
    coords_array[0] = DirectX::PackedVector::XMConvertHalfToFloat(half);
    half = *(uint16_t*)(vxbuffer + 2);
    coords_array[1] = DirectX::PackedVector::XMConvertHalfToFloat(half);
    half = *(uint16_t*)(vxbuffer + 4);
    coords_array[2] = DirectX::PackedVector::XMConvertHalfToFloat(half);
    return 6;
}

uint32_t vec2_fp16_extractor(char* vxbuffer, float* coords_array)
{
    uint16_t half = *(uint16_t*)vxbuffer;
    coords_array[0] = DirectX::PackedVector::XMConvertHalfToFloat(half);
    half = *(uint16_t*)(vxbuffer + 2);
    coords_array[1] = DirectX::PackedVector::XMConvertHalfToFloat(half);
    return 4;
}

uint32_t vec3_fp32_extractor(char* vxbuffer, float* coords_array)
{
    coords_array[0] = *(float*)vxbuffer;
    coords_array[1] = *(float*)(vxbuffer+4);
    coords_array[2] = *(float*)(vxbuffer+8);
    return 12;
}

uint32_t vec4_fp16_extractor(char* vxbuffer, float* coords_array)
{
    uint16_t half = *(uint16_t*)vxbuffer;
    coords_array[0] = DirectX::PackedVector::XMConvertHalfToFloat(half);
    half = *(uint16_t*)(vxbuffer + 2);
    coords_array[1] = DirectX::PackedVector::XMConvertHalfToFloat(half);
    half = *(uint16_t*)(vxbuffer + 4);
    coords_array[2] = DirectX::PackedVector::XMConvertHalfToFloat(half);
    half = *(uint16_t*)(vxbuffer + 6);
    coords_array[3] = DirectX::PackedVector::XMConvertHalfToFloat(half);
    return 8;
}

uint32_t uint8_rgba_extractor(char* vxbuffer, uint8_t* rgba_array)
{
    rgba_array[0] = *vxbuffer;
    rgba_array[1] = *(vxbuffer+1);
    rgba_array[2] = *(vxbuffer+2);
    rgba_array[3] = *(vxbuffer+3);
    return 4;
}

uint32_t vec3_fp16_compressor(char* buffer,int vx_buffer_pos,float* vec3)
{
    uint16_t* half = (uint16_t*)(&buffer[vx_buffer_pos]);
    *half = DirectX::PackedVector::XMConvertFloatToHalf(vec3[0]);
    vx_buffer_pos += 2;
    half = (uint16_t*)(&buffer[vx_buffer_pos]);
    *half = DirectX::PackedVector::XMConvertFloatToHalf(vec3[1]);
    vx_buffer_pos += 2;
    half = (uint16_t*)(&buffer[vx_buffer_pos]);
    *half = DirectX::PackedVector::XMConvertFloatToHalf(vec3[2]);
    return 6;
}

uint32_t vec2_fp16_compressor(char* buffer, int vx_buffer_pos, float* vec2)
{
    uint16_t* half = (uint16_t*)(&buffer[vx_buffer_pos]);
    *half = DirectX::PackedVector::XMConvertFloatToHalf(vec2[0]);
    vx_buffer_pos += 2;
    half = (uint16_t*)(&buffer[vx_buffer_pos]);
    *half = DirectX::PackedVector::XMConvertFloatToHalf(vec2[1]);
    return 4;
}

uint32_t vec4_fp16_compressor(char* buffer, int vx_buffer_pos, float* vec4)
{
    uint16_t* half = (uint16_t*)(&buffer[vx_buffer_pos]);
    *half = DirectX::PackedVector::XMConvertFloatToHalf(vec4[0]);
    vx_buffer_pos += 2;
    half = (uint16_t*)(&buffer[vx_buffer_pos]);
    *half = DirectX::PackedVector::XMConvertFloatToHalf(vec4[1]);
    vx_buffer_pos += 2;
    half = (uint16_t*)(&buffer[vx_buffer_pos]);
    *half = DirectX::PackedVector::XMConvertFloatToHalf(vec4[2]);
    vx_buffer_pos += 2;
    half = (uint16_t*)(&buffer[vx_buffer_pos]);
    *half = DirectX::PackedVector::XMConvertFloatToHalf(vec4[3]);
    return 8;
}

uint32_t vec3_fp32_compressor(char* buffer, int vx_buffer_pos, float* vec3)
{
    float* fp32_flt = (float*)(&buffer[vx_buffer_pos]);
    *fp32_flt = vec3[0];
    vx_buffer_pos += 4;
    fp32_flt = (float*)(&buffer[vx_buffer_pos]);
    *fp32_flt = vec3[1];
    vx_buffer_pos += 4;
    fp32_flt = (float*)(&buffer[vx_buffer_pos]);
    *fp32_flt = vec3[2];
    return 12;
}

uint32_t uint8_rgba_compressor(char* buffer, int vx_buffer_pos, uint8_t* rgba_array)
{
    uint8_t* byte = (uint8_t*)(&buffer[vx_buffer_pos]);
    *byte = rgba_array[0];
    vx_buffer_pos++;
    byte = (uint8_t*)(&buffer[vx_buffer_pos]);
    *byte = rgba_array[1];
    vx_buffer_pos++;   
    byte = (uint8_t*)(&buffer[vx_buffer_pos]);
    *byte = rgba_array[2];
    vx_buffer_pos++;
    byte = (uint8_t*)(&buffer[vx_buffer_pos]);
    *byte = rgba_array[3];
    return 4;
}

uint32_t half_compressor(char* buffer, int vx_buffer_pos, float val)
{
    uint16_t* half = (uint16_t*)(&buffer[vx_buffer_pos]);
    *half = DirectX::PackedVector::XMConvertFloatToHalf(val);
    return 2;
}