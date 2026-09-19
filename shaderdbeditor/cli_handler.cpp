#define _CRT_SECURE_NO_WARNINGS
#include "framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <math.h>
#include <vector>
#include <string>
#include "structs.h"
#include "funcs.h"
#include "cli_handler.h"

static void print_guid(char* guid, char* out) {
    for (int i = 0; i < 16; i++) {
        sprintf(out + (i * 2), "%02X", (unsigned char)guid[i]);
    }
    out[32] = '\0';
}

static float half_to_float_be(uint16_t h_be) {
    uint16_t h = _byteswap_ushort(h_be);
    uint32_t sign = (h >> 15) & 0x0001;
    uint32_t exp = (h >> 10) & 0x001f;
    uint32_t mant = h & 0x03ff;

    uint32_t f;
    if (exp == 0) {
        if (mant == 0) {
            f = sign << 31;
        } else {
            while (!(mant & 0x0400)) {
                mant <<= 1;
                exp--;
            }
            exp++;
            mant &= ~0x0400;
            f = (sign << 31) | ((exp + (127 - 15)) << 23) | (mant << 13);
        }
    } else if (exp == 31) {
        f = (sign << 31) | (0xff << 23) | (mant << 13);
    } else {
        f = (sign << 31) | ((exp + (127 - 15)) << 23) | (mant << 13);
    }
    float res;
    memcpy(&res, &f, 4);
    return res;
}

static void unpack_normal_be(uint32_t norm_be, float& nx, float& ny, float& nz) {
    uint32_t val = _byteswap_ulong(norm_be);
    int32_t x_raw = (val >> 22) & 0x3ff;
    int32_t y_raw = (val >> 12) & 0x3ff;
    int32_t z_raw = (val >> 2) & 0x3ff;
    if (x_raw & 0x200) x_raw -= 0x400;
    if (y_raw & 0x200) y_raw -= 0x400;
    if (z_raw & 0x200) z_raw -= 0x400;
    nx = (float)x_raw / 511.0f;
    ny = (float)y_raw / 511.0f;
    nz = (float)z_raw / 511.0f;
    float len = sqrtf(nx*nx + ny*ny + nz*nz);
    if (len > 0.0001f) {
        nx /= len; ny /= len; nz /= len;
    } else {
        nx = 0.0f; ny = 1.0f; nz = 0.0f;
    }
}

struct ParsedNode {
    std::string name;
    uint32_t faces;
    uint32_t verts;
    uint32_t idx_start;
    uint32_t v_start;
    uint32_t stride;
};

static std::vector<ParsedNode> scan_nodes(const char* buf, long payload_off) {
    std::vector<ParsedNode> nodes;
    long p = 7;
    while (p < payload_off - 30) {
        unsigned char c = (unsigned char)buf[p];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            const char* null_pos = (const char*)memchr(buf + p, 0, 128);
            if (null_pos && (null_pos - (buf + p) > 0)) {
                size_t nlen = null_pos - (buf + p);
                bool valid_id = true;
                for (size_t k = 0; k < nlen; k++) {
                    char ch = buf[p + k];
                    if (!isalnum((unsigned char)ch) && ch != '_') { valid_id = false; break; }
                }
                if (valid_id) {
                    long q = p + (long)nlen + 1;
                    if (q + 22 <= payload_off) {
                        uint32_t n_faces = _byteswap_ulong(*(const uint32_t*)(buf + q));
                        uint32_t n_verts = _byteswap_ulong(*(const uint32_t*)(buf + q + 4));
                        uint32_t idx_start = _byteswap_ulong(*(const uint32_t*)(buf + q + 8));
                        uint32_t v_start = _byteswap_ulong(*(const uint32_t*)(buf + q + 12));
                        uint8_t vstride = *(const uint8_t*)(buf + q + 16);
                        uint8_t prim = *(const uint8_t*)(buf + q + 17);
                        uint8_t s1 = *(const uint8_t*)(buf + q + 19);
                        uint16_t s2 = _byteswap_ushort(*(const uint16_t*)(buf + q + 20));

                        if (prim == 3 && s1 == s2 && vstride >= 8 && vstride <= 64 && n_faces > 0 && n_verts > 0) {
                            ParsedNode node;
                            node.name = std::string(buf + p, nlen);
                            node.faces = n_faces;
                            node.verts = n_verts;
                            node.idx_start = idx_start;
                            node.v_start = v_start;
                            node.stride = vstride;
                            nodes.push_back(node);
                            p = q + 6 + s2 * 2;
                            continue;
                        }
                    }
                }
            }
        }
        p++;
    }
    return nodes;
}

static bool export_mesh_to_obj_impl(const char* meshdata_path, const char* out_obj_path) {
    FILE* f = fopen(meshdata_path, "rb");
    if (!f) {
        printf("[!] Error: Failed to open meshdata file '%s'\n", meshdata_path);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buf = (char*)malloc(fsize);
    if (!buf) {
        fclose(f);
        printf("[!] Error: Out of memory reading %ld bytes\n", fsize);
        return false;
    }
    fread(buf, 1, fsize, f);
    fclose(f);

    printf("[*] Meshdata file: %s (%ld bytes, %.2f KB)\n", meshdata_path, fsize, fsize / 1024.0);

    long payload_offset = -1;
    uint32_t vbuf_size = 0, ibuf_size = 0;

    for (long off = 0; off <= fsize - 12; off++) {
        uint32_t v_sz = _byteswap_ulong(*(uint32_t*)(buf + off));
        uint32_t i_sz = _byteswap_ulong(*(uint32_t*)(buf + off + 4));
        uint32_t null_val = *(uint32_t*)(buf + off + 8);
        if (null_val == 0 && v_sz > 0 && i_sz > 0 && (off + 12 + v_sz + i_sz == (uint32_t)fsize)) {
            payload_offset = off;
            vbuf_size = v_sz;
            ibuf_size = i_sz;
            break;
        }
    }

    if (payload_offset < 0) {
        printf("[!] Error: Could not locate geometry payload header in meshdata!\n");
        free(buf);
        return false;
    }

    const char* vbuf_start = buf + payload_offset + 12;
    const char* ibuf_start = vbuf_start + vbuf_size;
    uint32_t n_total_indices = ibuf_size / 2;
    const uint16_t* p_idx_be = (const uint16_t*)ibuf_start;

    std::vector<ParsedNode> all_nodes = scan_nodes(buf, payload_offset);
    std::vector<ParsedNode> visual_nodes;
    for (size_t i = 0; i < all_nodes.size(); i++) {
        std::string lower_name = all_nodes[i].name;
        for (size_t c = 0; c < lower_name.size(); c++) lower_name[c] = tolower(lower_name[c]);
        if (lower_name.find("zonly") == std::string::npos) {
            visual_nodes.push_back(all_nodes[i]);
        }
    }
    if (visual_nodes.empty() && !all_nodes.empty()) {
        visual_nodes.push_back(all_nodes[0]);
    }

    printf("[+] Detected %zu total nodes, %zu visual submeshes:\n", all_nodes.size(), visual_nodes.size());
    for (size_t i = 0; i < visual_nodes.size(); i++) {
        printf("    [%zu] Submesh: %s (faces: %u, verts: %u, stride: %u, v_start: %u, idx_start: %u)\n",
               i, visual_nodes[i].name.c_str(), visual_nodes[i].faces, visual_nodes[i].verts,
               visual_nodes[i].stride, visual_nodes[i].v_start, visual_nodes[i].idx_start);
    }

    char default_out[MAX_PATH];
    if (!out_obj_path || strlen(out_obj_path) == 0) {
        strcpy(default_out, meshdata_path);
        char* dot = strrchr(default_out, '.');
        if (dot) *dot = '\0';
        strcat(default_out, ".obj");
        out_obj_path = default_out;
    }

    FILE* out_f = fopen(out_obj_path, "w");
    if (!out_f) {
        printf("[!] Error: Failed to open '%s' for writing!\n", out_obj_path);
        free(buf);
        return false;
    }

    fprintf(out_f, "# Exported by Frostbite Mesh Tool\n");
    fprintf(out_f, "# Source: %s\n", meshdata_path);
    fprintf(out_f, "# Submeshes: %zu\n\n", visual_nodes.size());

    struct Tri { uint32_t i0, i1, i2; };
    struct SubmeshExport {
        std::string name;
        std::vector<Tri> tris;
    };
    std::vector<SubmeshExport> exported_submeshes;

    float min_x = 1e9f, max_x = -1e9f;
    float min_y = 1e9f, max_y = -1e9f;
    float min_z = 1e9f, max_z = -1e9f;

    uint32_t vert_base = 0;
    uint32_t total_verts = 0;
    uint32_t total_tris = 0;

    for (size_t s = 0; s < visual_nodes.size(); s++) {
        const ParsedNode& node = visual_nodes[s];
        const char* node_v_start = vbuf_start + node.v_start;

        for (uint32_t i = 0; i < node.verts; i++) {
            const char* vptr = node_v_start + (i * node.stride);
            float x = half_to_float_be(*(const uint16_t*)(vptr + 0));
            float y = half_to_float_be(*(const uint16_t*)(vptr + 2));
            float z = half_to_float_be(*(const uint16_t*)(vptr + 4));

            if (x < min_x) min_x = x;
            if (x > max_x) max_x = x;
            if (y < min_y) min_y = y;
            if (y > max_y) max_y = y;
            if (z < min_z) min_z = z;
            if (z > max_z) max_z = z;

            fprintf(out_f, "v %.6f %.6f %.6f\n", x, y, z);
        }

        // Write UVs
        for (uint32_t i = 0; i < node.verts; i++) {
            const char* vptr = node_v_start + (i * node.stride);
            if (node.stride >= 28) {
                float u = half_to_float_be(*(const uint16_t*)(vptr + 24));
                float v = half_to_float_be(*(const uint16_t*)(vptr + 26));
                fprintf(out_f, "vt %.6f %.6f\n", u, 1.0f - v);
            } else {
                fprintf(out_f, "vt 0.000000 0.000000\n");
            }
        }

        // Write Normals
        for (uint32_t i = 0; i < node.verts; i++) {
            const char* vptr = node_v_start + (i * node.stride);
            if (node.stride >= 20) {
                uint32_t norm_be = *(const uint32_t*)(vptr + 16);
                float nx, ny, nz;
                unpack_normal_be(norm_be, nx, ny, nz);
                fprintf(out_f, "vn %.6f %.6f %.6f\n", nx, ny, nz);
            } else {
                fprintf(out_f, "vn 0.000000 1.000000 0.000000\n");
            }
        }

        SubmeshExport sm;
        sm.name = node.name;
        for (uint32_t i = 0; i < node.faces * 3; i += 3) {
            uint32_t idx0 = _byteswap_ushort(p_idx_be[node.idx_start + i]) + vert_base + 1;
            uint32_t idx1 = _byteswap_ushort(p_idx_be[node.idx_start + i + 1]) + vert_base + 1;
            uint32_t idx2 = _byteswap_ushort(p_idx_be[node.idx_start + i + 2]) + vert_base + 1;
            Tri t = { idx0, idx1, idx2 };
            sm.tris.push_back(t);
        }
        exported_submeshes.push_back(sm);

        vert_base += node.verts;
        total_verts += node.verts;
        total_tris += node.faces;
    }

    fprintf(out_f, "\n");
    for (size_t s = 0; s < exported_submeshes.size(); s++) {
        fprintf(out_f, "g %s\n", exported_submeshes[s].name.c_str());
        for (size_t t = 0; t < exported_submeshes[s].tris.size(); t++) {
            const Tri& tri = exported_submeshes[s].tris[t];
            fprintf(out_f, "f %u/%u/%u %u/%u/%u %u/%u/%u\n",
                    tri.i0, tri.i0, tri.i0,
                    tri.i1, tri.i1, tri.i1,
                    tri.i2, tri.i2, tri.i2);
        }
    }

    fclose(out_f);
    free(buf);

    printf("[+] Successfully exported %u vertices and %u faces across %zu submeshes to:\n    %s\n",
           total_verts, total_tris, exported_submeshes.size(), out_obj_path);
    printf("    Bounds X: [%.4f, %.4f] (width:  %.4f m)\n", min_x, max_x, max_x - min_x);
    printf("    Bounds Y: [%.4f, %.4f] (height: %.4f m)\n", min_y, max_y, max_y - min_y);
    printf("    Bounds Z: [%.4f, %.4f] (length: %.4f m)\n", min_z, max_z, max_z - min_z);

    return true;
}

bool run_cli(int argc, char** argv) {
    if (argc <= 1) {
        HWND hCon = GetConsoleWindow();
        if (hCon) FreeConsole();
        return false;
    }

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    printf("\n============================================\n");
    printf("  Frostbite Shader & Mesh Tool (CLI Mode)   \n");
    printf("============================================\n");

    const char* cmd = argv[1];

    if (strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        printf("Commands:\n");
        printf("  --export-mesh <meshdata_path> [output_obj_path]\n");
        printf("      Exports a PS3/PC .meshdata model directly into .obj format.\n\n");
        printf("  --test-db <shaderdatabase_path>\n");
        printf("      Tests parsing of a PS3/PC shader database without dumping.\n\n");
        printf("  --dump-shaders <shaderdatabase_path> <output_dir>\n");
        printf("      Dumps all vertex and pixel shaders into the specified folder.\n\n");
        printf("  --dump-info <shaderdatabase_path>\n");
        printf("      Dumps database header, string tables, shader and mesh GUIDs.\n");
        return true;
    }

    if (strcmp(cmd, "--export-mesh") == 0) {
        if (argc < 3) {
            printf("[!] Error: Missing meshdata path! Usage: --export-mesh <file.meshdata> [out.obj]\n");
            return true;
        }
        const char* mesh_path = argv[2];
        const char* out_path = (argc >= 4) ? argv[3] : nullptr;
        export_mesh_to_obj_impl(mesh_path, out_path);
        return true;
    }

    if (strcmp(cmd, "--test-db") == 0) {
        if (argc < 3) {
            printf("[!] Error: Missing database path! Usage: --test-db <path>\n");
            return true;
        }
        const char* db_path = argv[2];
        printf("[*] Opening database: %s\n", db_path);
        FILE* f = fopen(db_path, "rb");
        if (!f) {
            printf("[!] Error: Failed to open file '%s'\n", db_path);
            return true;
        }
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        printf("[*] File size: %ld bytes (%.2f MB)\n", fsize, fsize / (1024.0 * 1024.0));

        char* buf = (char*)malloc(fsize);
        if (!buf) {
            printf("[!] Error: Out of memory!\n");
            fclose(f);
            return true;
        }
        fread(buf, 1, fsize, f);
        fclose(f);

        uint32_t ptr_pos = 0;
        printf("[*] Parsing database structures...\n");
        database* db = dbfirstload(buf, &ptr_pos);
        if (!db) {
            printf("[!] Error: dbfirstload failed!\n");
            free(buf);
            return true;
        }
        printf("\n>>> DATABASE PARSED SUCCESSFULLY! <<<\n");
        printf("  Textures (TxE):         %u\n", db->ntxe);
        printf("  D3D Params 1:           %u\n", db->n_d3d_p1);
        printf("  Vertex Shaders:         %u\n", db->n_vs_shader);
        printf("  Pixel Shaders:          %u\n", db->n_ps_shader);
        printf("  Table Elements:         %u\n", db->n_table_elements);
        printf("  Vertex Buffers:         %u\n", db->n_vx_buffers);
        printf("  Strings[0] (Shaders):   %u\n", db->n_strings[0]);
        printf("  Strings[1] (Systems):   %u\n", db->n_strings[1]);
        printf("  Strings[2] (Meshes):    %u\n", db->n_strings[2]);

        printf("[*] Initializing export structures...\n");
        char* file_end = buf + ptr_pos;
        database_export* exp_db = export_db_init(db, file_end);
        if (exp_db) {
            printf("[+] Export database initialized successfully!\n");
        }
        free(buf);
        return true;
    }

    if (strcmp(cmd, "--dump-shaders") == 0) {
        if (argc < 4) {
            printf("[!] Error: Usage: --dump-shaders <shaderdatabase_path> <output_dir>\n");
            return true;
        }
        const char* db_path = argv[2];
        const char* out_dir = argv[3];

        FILE* f = fopen(db_path, "rb");
        if (!f) {
            printf("[!] Error: Failed to open '%s'\n", db_path);
            return true;
        }
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);

        char* buf = (char*)malloc(fsize);
        if (!buf) {
            printf("[!] Error: Out of memory!\n");
            fclose(f);
            return true;
        }
        fread(buf, 1, fsize, f);
        fclose(f);

        uint32_t ptr_pos = 0;
        database* db = dbfirstload(buf, &ptr_pos);
        if (!db) {
            printf("[!] Error: Failed to parse database!\n");
            free(buf);
            return true;
        }

        _mkdir(out_dir);
        char sub_vs[MAX_PATH], sub_ps[MAX_PATH];
        sprintf(sub_vs, "%s/vs", out_dir);
        sprintf(sub_ps, "%s/ps", out_dir);
        _mkdir(sub_vs);
        _mkdir(sub_ps);

        printf("[*] Dumping %u Vertex Shaders...\n", db->n_vs_shader);
        for (uint32_t i = 0; i < db->n_vs_shader; i++) {
            char g[64];
            print_guid(db->vs_shader[i].vsid, g);
            char fpath[MAX_PATH];
            sprintf(fpath, "%s/%s.bin", sub_vs, g);
            FILE* out_f = fopen(fpath, "wb");
            if (out_f) {
                fwrite(db->vs_shader[i].shader, 1, *db->vs_shader[i].vs_size, out_f);
                fclose(out_f);
            }
        }

        printf("[*] Dumping %u Pixel Shaders...\n", db->n_ps_shader);
        for (uint32_t i = 0; i < db->n_ps_shader; i++) {
            char g[64];
            print_guid(db->ps_shader[i].psid, g);
            char fpath[MAX_PATH];
            sprintf(fpath, "%s/%s.bin", sub_ps, g);
            FILE* out_f = fopen(fpath, "wb");
            if (out_f) {
                fwrite(db->ps_shader[i].shader, 1, *db->ps_shader[i].ps_size, out_f);
                fclose(out_f);
            }
        }
        printf("[+] All %u shaders dumped successfully to '%s'!\n", db->n_vs_shader + db->n_ps_shader, out_dir);
        free(buf);
        return true;
    }

    if (strcmp(cmd, "--dump-info") == 0) {
        if (argc < 3) {
            printf("[!] Error: Usage: --dump-info <shaderdatabase_path>\n");
            return true;
        }
        const char* db_path = argv[2];
        FILE* f = fopen(db_path, "rb");
        if (!f) return true;
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        char* buf = (char*)malloc(fsize);
        fread(buf, 1, fsize, f);
        fclose(f);

        uint32_t ptr_pos = 0;
        database* db = dbfirstload(buf, &ptr_pos);
        if (!db) return true;

        printf("\n--- SHADERS (%u entries) ---\n", db->n_strings[0]);
        for (uint32_t i = 0; i < db->n_strings[0]; i++) {
            char g[64];
            print_guid(db->strings[0][i].guid, g);
            printf("[%04u] %s (GUID: %s)\n", i, db->strings[0][i].name, g);
        }

        printf("\n--- SYSTEMS (%u entries) ---\n", db->n_strings[1]);
        for (uint32_t i = 0; i < db->n_strings[1]; i++) {
            char g[64];
            print_guid(db->strings[1][i].guid, g);
            printf("[%04u] %s (GUID: %s)\n", i, db->strings[1][i].name, g);
        }

        printf("\n--- MESH REFERENCES (%u entries) ---\n", db->n_strings[2]);
        for (uint32_t i = 0; i < db->n_strings[2]; i++) {
            char g[64];
            print_guid(db->strings[2][i].guid, g);
            printf("[%04u] %s (GUID: %s)\n", i, db->strings[2][i].name, g);
        }
        free(buf);
        return true;
    }

    printf("[!] Unknown CLI command: '%s'. Use --help for usage.\n", cmd);
    return true;
}
