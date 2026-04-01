#pragma once
#include "assetc.h"
#include "assetc_structs.h"
#include <fbxsdk.h>
#include <fbxsdk/fileio/fbxiosettings.h>


void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene);
void DestroySdkObjects(FbxManager* pManager, bool pExitStatus);
void CreateAndFillIOSettings(FbxManager* pManager);

bool SaveScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename, int pFileFormat = -1, bool pEmbedMedia = false);
bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename);
FbxNode* CreateNode(FbxScene* pScene, char* pName);
FbxSurfacePhong* CreateMaterial(FbxScene* pScene, meshset* mesh_set, int current_node, int current_lod);
FbxNode* create_composite_mesh(FbxScene* pScene, char* shape_name, shape_geo* shape, node_geometry* node_geo, int current_node);