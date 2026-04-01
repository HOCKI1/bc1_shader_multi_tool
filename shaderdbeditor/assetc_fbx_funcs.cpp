#include "assetc_fbx_funcs.h"

#ifdef IOS_REF
#undef  IOS_REF
#define IOS_REF (*(pSdkManager->GetIOSettings()))
#endif


void DestroySdkObjects(FbxManager* pManager, bool pExitStatus)
{
	//Delete the FBX Manager. All the objects that have been allocated using the FBX Manager and that haven't been explicitly destroyed are also automatically destroyed.
	if (pManager) pManager->Destroy();
}


void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene)
{
	//The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
	pManager = FbxManager::Create();
	if (!pManager)
	{
		MessageBoxA(hwndAsset, "Error: Unable to create FBX Manager!", "Error", MB_OK);
		exit(1);
	}

	//Create an IOSettings object. This object holds all import/export settings.
	FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
	pManager->SetIOSettings(ios);

	//Create an FBX scene. This object holds most objects imported/exported from/to files.
	pScene = FbxScene::Create(pManager, "My Scene");
	if (!pScene)
	{
		MessageBoxA(hwndAsset, "Error: Unable to create FBX scene!", "Error", MB_OK);
		exit(1);
	}
	return;
}

FbxNode* CreateNode(FbxScene* pScene, char* pName) {

	FbxNode* PNode = FbxNode::Create(pScene, pName);
	return PNode;
}


FbxSurfacePhong* CreateMaterial(FbxScene* pScene, meshset* mesh_set, int current_node, int current_lod)
{
	
	char* material_name = mesh_set->lods[current_lod].shader_sets->shaders[current_node];
	FbxString lMaterialName = material_name;
	FbxString lShadingName = "Phong";
	FbxDouble3 lBlack(0.0, 0.0, 0.0);
	FbxDouble3 lRed(0.0, 0.0, 0.0);
	FbxDouble3 lColor;
	FbxSurfacePhong* lMaterial = FbxSurfacePhong::Create(pScene, lMaterialName.Buffer());

	lMaterial->Emissive.Set(lBlack);
	lMaterial->Ambient.Set(lRed);
	lColor = FbxDouble3(1.0, 1.0, 1.0);
	lMaterial->Diffuse.Set(lColor);
	lMaterial->TransparencyFactor.Set(0.0);
	lMaterial->ShadingModel.Set(lShadingName);
	lMaterial->Shininess.Set(0.5);
	return lMaterial;
}

bool SaveScene(FbxManager* pSdkManager, FbxDocument* pScene, const char* pFilename, int pFileFormat, bool pEmbedMedia)
{
    if (pSdkManager == NULL) return false;
    if (pScene == NULL) return false;
    if (pFilename == NULL) return false;

    bool lStatus = true;

    // Create an exporter.
    FbxExporter* lExporter = FbxExporter::Create(pSdkManager, "");

    if (pFileFormat < 0 || pFileFormat >= pSdkManager->GetIOPluginRegistry()->GetWriterFormatCount())
    {
        // Write in fall back format if pEmbedMedia is true
        pFileFormat = pSdkManager->GetIOPluginRegistry()->GetNativeWriterFormat();

        if (!pEmbedMedia)
        {
            //Try to export in ASCII if possible
            int lFormatIndex, lFormatCount = pSdkManager->GetIOPluginRegistry()->GetWriterFormatCount();

            for (lFormatIndex = 0; lFormatIndex < lFormatCount; lFormatIndex++)
            {
                if (pSdkManager->GetIOPluginRegistry()->WriterIsFBX(lFormatIndex))
                {
                    FbxString lDesc = pSdkManager->GetIOPluginRegistry()->GetWriterFormatDescription(lFormatIndex);
                    const char* lASCII = "binary";
                    if (lDesc.Find(lASCII) >= 0)
                    {
                        pFileFormat = lFormatIndex;
                        break;
                    }
                }
            }
        }
    }

    // Initialize the exporter by providing a filename.
    if (lExporter->Initialize(pFilename, pFileFormat, pSdkManager->GetIOSettings()) == false)
    {
        return false;
    }

    // Set the export states. By default, the export states are always set to 
    // true except for the option eEXPORT_TEXTURE_AS_EMBEDDED. The code below 
    // shows how to change these states.
    IOS_REF.SetBoolProp(EXP_FBX_MATERIAL, true);
    IOS_REF.SetBoolProp(EXP_FBX_TEXTURE, true);
    IOS_REF.SetBoolProp(EXP_FBX_EMBEDDED, pEmbedMedia);
    IOS_REF.SetBoolProp(EXP_FBX_SHAPE, true);
    IOS_REF.SetBoolProp(EXP_FBX_GOBO, true);
    IOS_REF.SetBoolProp(EXP_FBX_ANIMATION, true);
    IOS_REF.SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);

    // Export the scene.
    lStatus = lExporter->Export(pScene);

    // Destroy the exporter.
    lExporter->Destroy();

    return lStatus;
}



FbxNode* create_composite_mesh(FbxScene* pScene, char* shape_name, shape_geo* shape, node_geometry* node_geo, int current_node)
{
    int arrayindexsize;
    int* arrayindexptr;
    int uvchannels;
    double xc;
    double yc;
    double zc;

    FbxMesh* lMesh = FbxMesh::Create(pScene, shape_name);
    // Create control points.
    lMesh->InitControlPoints(shape->n_vx);
    FbxVector4* lControlPoints = lMesh->GetControlPoints();
    int vx_idx;
    for (int i = 0; i < shape->n_vx; i++) {
        vx_idx = shape->n_vx_array[i];
        xc = double(node_geo->vx_coords[vx_idx*3]);
        yc = double(node_geo->vx_coords[vx_idx*3+1]);
        zc = double(node_geo->vx_coords[vx_idx*3+2]);
        FbxVector4 lControlPointK(xc, yc, zc);
        lControlPoints[i] = lControlPointK;
    }
    if (node_geo->export_normals) 
    {
        FbxGeometryElementNormal* lGeometryElementNormal = lMesh->CreateElementNormal();
        lGeometryElementNormal->SetMappingMode(FbxGeometryElement::eByControlPoint);
        // Set the normal values for every control point.
        lGeometryElementNormal->SetReferenceMode(FbxGeometryElement::eDirect);
        for (int i = 0; i < shape->n_vx; i++) 
        {
            vx_idx = shape->n_vx_array[i];
            xc = double(node_geo->normals[vx_idx * 3]);
            yc = double(node_geo->normals[vx_idx * 3 + 1]);
            zc = double(node_geo->normals[vx_idx * 3 + 2]);
            FbxVector4 lControlPointK(xc, yc, zc);
            lGeometryElementNormal->GetDirectArray().Add(lControlPointK);
        }
    }
    const char* uv_names[5] = {"Other", "Diffuse", "Normal", "Other2", "Other3"};
    FbxGeometryElementUV* UVCHANNEL[6] = {NULL,NULL,NULL,NULL,NULL,NULL};               // max of 6 channels, more than what the game uses, workaround for array
    for (int i = 0; i < node_geo->n_uvs; i++) {
        UVCHANNEL[i] = lMesh->CreateElementUV(uv_names[i]);
        UVCHANNEL[i]->SetMappingMode(FbxGeometryElement::eByControlPoint);
        UVCHANNEL[i]->SetReferenceMode(FbxGeometryElement::eDirect);
        for (int j = 0; j < shape->n_vx; j++) {
            vx_idx = shape->n_vx_array[j];
            xc = double(node_geo->uv_channels[i][vx_idx*2]);
            yc = double(node_geo->uv_channels[i][vx_idx*2+1]*-1);
            FbxVector2 UVECTOR(xc, yc);
            UVCHANNEL[i]->GetDirectArray().Add(UVECTOR);
        }
    }
    for (int i = 0; i < shape->idx_array_size/3; i++)     // i is number of faces. 
    {
        // all faces of the cube have the same texture
        lMesh->BeginPolygon(current_node);
        for (int j = 0; j < 3; j++)  // j is number of coordinates in control point / vertex 
        {
            // Control point index
            lMesh->AddPolygon(shape->indices_out[i * 3 + j]);
        }
        lMesh->EndPolygon();
    }
    // create a FbxNode
    FbxNode* lNode = FbxNode::Create(pScene, shape_name);
    // set the node attribute
    lNode->SetNodeAttribute(lMesh);
    // set the shading mode to view texture
    lNode->SetShadingMode(FbxNode::eTextureShading);
    // return the FbxNode
    return lNode;
}


bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename)
{
    int lFileMajor, lFileMinor, lFileRevision;
    int lSDKMajor, lSDKMinor, lSDKRevision;
    //int lFileFormat = -1;
    int lAnimStackCount;
    bool lStatus;
    char lPassword[1024];

    // Get the file version number generate by the FBX SDK.
    FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

    // Create an importer.
    FbxImporter* lImporter = FbxImporter::Create(pManager, "");

    // Initialize the importer by providing a filename.
    const bool lImportStatus = lImporter->Initialize(pFilename, -1, pManager->GetIOSettings());
    lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

    if (!lImportStatus)
    {
        FbxString error = lImporter->GetStatus().GetErrorString();
      //  FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
      //  FBXSDK_printf("Error returned: %s\n\n", error.Buffer());

        if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
        {
       //     FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
       //     FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);
        }

        return false;
    }

    //FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

    // Import the scene.
    lStatus = lImporter->Import(pScene);
    /*if (lStatus == false && lImporter->GetStatus() == FbxStatus::ePasswordError)
    {
        FBXSDK_printf("Please enter password: ");

        lPassword[0] = '\0';

        FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
            scanf("%s", lPassword);
        FBXSDK_CRT_SECURE_NO_WARNING_END

            FbxString lString(lPassword);


        lStatus = lImporter->Import(pScene);

        if (lStatus == false && lImporter->GetStatus() == FbxStatus::ePasswordError)
        {
            FBXSDK_printf("\nPassword is wrong, import aborted.\n");
        }
    }*/

    if (!lStatus || (lImporter->GetStatus() != FbxStatus::eSuccess))
    {
    //    FBXSDK_printf("********************************************************************************\n");
        if (lStatus)
        {
    //        FBXSDK_printf("WARNING:\n");
    //        FBXSDK_printf("   The importer was able to read the file but with errors.\n");
    //        FBXSDK_printf("   Loaded scene may be incomplete.\n\n");
        }
        else
        {
    //        FBXSDK_printf("Importer failed to load the file!\n\n");
        }

        if (lImporter->GetStatus() != FbxStatus::eSuccess)
        {
        }
   //         FBXSDK_printf("   Last error message: %s\n", lImporter->GetStatus().GetErrorString());

        FbxArray<FbxString*> history;
        lImporter->GetStatus().GetErrorStringHistory(history);
        if (history.GetCount() > 1)
        {
     //       FBXSDK_printf("   Error history stack:\n");
            for (int i = 0; i < history.GetCount(); i++)
            {
        //        FBXSDK_printf("      %s\n", history[i]->Buffer());
            }
        }
        FbxArrayDelete<FbxString*>(history);
       // FBXSDK_printf("********************************************************************************\n");
    }

    // Destroy the importer.
    lImporter->Destroy();

    return lStatus;
}