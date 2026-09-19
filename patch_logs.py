import re

path = r'shaderdbeditor\dbfuncs.cpp'
c = open(path, 'r', encoding='utf-8').read()

header = '''#include "structs.h"
#include <stdio.h>
FILE* dbg_log = NULL;
#define LOG(...) { if(dbg_log) { fprintf(dbg_log, __VA_ARGS__); fflush(dbg_log); } }

bool is_ps3 = false;
inline uint32_t SWAP32_P(uint32_t* ptr) { if (is_ps3) *ptr = _byteswap_ulong(*ptr); return *ptr; }
inline uint16_t SWAP16_P(uint16_t* ptr) { if (is_ps3) *ptr = _byteswap_ushort(*ptr); return *ptr; }
'''

c = c.replace('#include "structs.h"', header)

patch_endian = '''
    uint32_t test_endian = *(uint32_t*)dbfile;
    is_ps3 = ((test_endian & 0xFF000000) != 0);
    dbg_log = fopen("parse_debug.log", "w");
    LOG("is_ps3 = %d\\n", is_ps3);
    uint32_t position = (uint32_t)ptr_temp;
'''
c = c.replace('uint32_t position = (uint32_t)ptr_temp;', patch_endian)

c = c.replace('txequicklook(dbfile, dbptr);', 'LOG("txequicklook...\\n"); txequicklook(dbfile, dbptr); LOG("txequicklook DONE\\n");')
c = c.replace('d3dparamslook(dbfile,dbptr);', 'LOG("d3dparamslook...\\n"); d3dparamslook(dbfile,dbptr); LOG("d3dparamslook DONE\\n");')
c = c.replace('vslook(dbfile, dbptr);', 'LOG("vslook...\\n"); vslook(dbfile, dbptr); LOG("vslook DONE\\n");')
c = c.replace('pslook(dbfile,dbptr);', 'LOG("pslook...\\n"); pslook(dbfile,dbptr); LOG("pslook DONE\\n");')
c = c.replace('tableslook(dbfile,dbptr);', 'LOG("tableslook...\\n"); tableslook(dbfile,dbptr); LOG("tableslook DONE\\n");')
c = c.replace('vbufferslook(dbfile, dbptr);', 'LOG("vbufferslook...\\n"); vbufferslook(dbfile, dbptr); LOG("vbufferslook DONE\\n");')
c = c.replace('stringslook(dbfile,dbptr);', 'LOG("stringslook...\\n"); stringslook(dbfile,dbptr); LOG("stringslook DONE\\n");')
c = c.replace('shader_ref_look(dbfile, dbptr);', 'LOG("shader_ref_look...\\n"); shader_ref_look(dbfile, dbptr); LOG("shader_ref_look DONE\\n");')

c = re.sub(r'nelements = \(uint32_t\*\)dummy;\s*dummy \+= 4;', r'nelements = (uint32_t*)dummy; SWAP32_P(nelements);\n\tdummy += 4;', c)

open(path, 'w', encoding='utf-8').write(c)
