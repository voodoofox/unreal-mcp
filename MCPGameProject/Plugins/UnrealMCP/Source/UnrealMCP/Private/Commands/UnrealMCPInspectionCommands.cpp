#include "Commands/UnrealMCPInspectionCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/UObjectIterator.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MaterialEditingLibrary.h"
#include "SceneTypes.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Animation/Skeleton.h"
#include "MetasoundSource.h"
#include "MetasoundFrontendDocument.h"
#include "Animation/AnimSequenceBase.h"
#include "Engine/SkeletalMesh.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType.h"

// FindBlueprintFlexible is now in CommonUtils::FindBlueprintByName — use FindBlueprint() directly
static UBlueprint* FindBlueprintFlexible(const FString& BlueprintName)
{
    return FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("get_blueprint_components"))
    {
        return HandleGetBlueprintComponents(Params);
    }
    else if (CommandType == TEXT("get_blueprint_defaults"))
    {
        return HandleGetBlueprintDefaults(Params);
    }
    else if (CommandType == TEXT("list_assets"))
    {
        return HandleListAssets(Params);
    }
    else if (CommandType == TEXT("get_asset_details"))
    {
        return HandleGetAssetDetails(Params);
    }
    else if (CommandType == TEXT("get_material_info"))
    {
        return HandleGetMaterialInfo(Params);
    }
    else if (CommandType == TEXT("get_static_mesh_info"))
    {
        return HandleGetStaticMeshInfo(Params);
    }
    else if (CommandType == TEXT("get_component_properties"))
    {
        return HandleGetComponentProperties(Params);
    }
    else if (CommandType == TEXT("get_blueprint_graph"))
    {
        return HandleGetBlueprintGraph(Params);
    }
    else if (CommandType == TEXT("get_node_pins"))
    {
        return HandleGetNodePins(Params);
    }
    else if (CommandType == TEXT("list_nanite_status"))
    {
        return HandleListNaniteStatus(Params);
    }
    else if (CommandType == TEXT("set_static_mesh_nanite_enabled"))
    {
        return HandleSetStaticMeshNaniteEnabled(Params);
    }
    else if (CommandType == TEXT("get_material_graph"))
    {
        return HandleGetMaterialGraph(Params);
    }
    else if (CommandType == TEXT("set_material_instance_parent"))
    {
        return HandleSetMaterialInstanceParent(Params);
    }
    else if (CommandType == TEXT("add_material_expression"))
    {
        return HandleAddMaterialExpression(Params);
    }
    else if (CommandType == TEXT("delete_material_expression"))
    {
        return HandleDeleteMaterialExpression(Params);
    }
    else if (CommandType == TEXT("connect_material_expressions"))
    {
        return HandleConnectMaterialExpressions(Params);
    }
    else if (CommandType == TEXT("connect_material_property"))
    {
        return HandleConnectMaterialProperty(Params);
    }
    else if (CommandType == TEXT("disconnect_material_property"))
    {
        return HandleDisconnectMaterialProperty(Params);
    }
    else if (CommandType == TEXT("set_material_expression_property"))
    {
        return HandleSetMaterialExpressionProperty(Params);
    }
    else if (CommandType == TEXT("add_custom_node_input"))
    {
        return HandleAddCustomNodeInput(Params);
    }
    else if (CommandType == TEXT("compile_material"))
    {
        return HandleCompileMaterial(Params);
    }
    else if (CommandType == TEXT("get_skeleton_bones"))
    {
        return HandleGetSkeletonBones(Params);
    }
    else if (CommandType == TEXT("get_behavior_tree_info"))
    {
        return HandleGetBehaviorTreeInfo(Params);
    }
    else if (CommandType == TEXT("get_metasound_graph"))
    {
        return HandleGetMetaSoundGraph(Params);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown inspection command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleGetBlueprintComponents(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FindBlueprintFlexible(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    if (!Blueprint->SimpleConstructionScript)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint has no SimpleConstructionScript"));
    }

    TArray<USCS_Node*> AllNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
    TArray<TSharedPtr<FJsonValue>> ComponentArray;

    for (USCS_Node* Node : AllNodes)
    {
        if (!Node || !Node->ComponentTemplate)
        {
            continue;
        }

        TSharedPtr<FJsonObject> CompObj = MakeShared<FJsonObject>();

        // Name and type
        CompObj->SetStringField(TEXT("name"), Node->GetVariableName().ToString());
        CompObj->SetStringField(TEXT("type"), Node->ComponentTemplate->GetClass()->GetName());

        // Parent component
        if (!Node->ParentComponentOrVariableName.IsNone())
        {
            CompObj->SetStringField(TEXT("parent"), Node->ParentComponentOrVariableName.ToString());
        }

        // Transform for scene components
        USceneComponent* SceneComp = Cast<USceneComponent>(Node->ComponentTemplate);
        if (SceneComp)
        {
            FVector Loc = SceneComp->GetRelativeLocation();
            FRotator Rot = SceneComp->GetRelativeRotation();
            FVector Scale = SceneComp->GetRelativeScale3D();

            TArray<TSharedPtr<FJsonValue>> LocArr;
            LocArr.Add(MakeShared<FJsonValueNumber>(Loc.X));
            LocArr.Add(MakeShared<FJsonValueNumber>(Loc.Y));
            LocArr.Add(MakeShared<FJsonValueNumber>(Loc.Z));
            CompObj->SetArrayField(TEXT("location"), LocArr);

            TArray<TSharedPtr<FJsonValue>> RotArr;
            RotArr.Add(MakeShared<FJsonValueNumber>(Rot.Pitch));
            RotArr.Add(MakeShared<FJsonValueNumber>(Rot.Yaw));
            RotArr.Add(MakeShared<FJsonValueNumber>(Rot.Roll));
            CompObj->SetArrayField(TEXT("rotation"), RotArr);

            TArray<TSharedPtr<FJsonValue>> ScaleArr;
            ScaleArr.Add(MakeShared<FJsonValueNumber>(Scale.X));
            ScaleArr.Add(MakeShared<FJsonValueNumber>(Scale.Y));
            ScaleArr.Add(MakeShared<FJsonValueNumber>(Scale.Z));
            CompObj->SetArrayField(TEXT("scale"), ScaleArr);

            CompObj->SetBoolField(TEXT("visible"), SceneComp->GetVisibleFlag());
        }

        // Static mesh path for static mesh components
        UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Node->ComponentTemplate);
        if (MeshComp && MeshComp->GetStaticMesh())
        {
            CompObj->SetStringField(TEXT("static_mesh"), MeshComp->GetStaticMesh()->GetPathName());
        }

        // Mobility
        if (SceneComp)
        {
            switch (SceneComp->Mobility)
            {
            case EComponentMobility::Static:
                CompObj->SetStringField(TEXT("mobility"), TEXT("Static"));
                break;
            case EComponentMobility::Stationary:
                CompObj->SetStringField(TEXT("mobility"), TEXT("Stationary"));
                break;
            case EComponentMobility::Movable:
                CompObj->SetStringField(TEXT("mobility"), TEXT("Movable"));
                break;
            }
        }

        ComponentArray.Add(MakeShared<FJsonValueObject>(CompObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint"), BlueprintName);
    ResultObj->SetNumberField(TEXT("component_count"), ComponentArray.Num());
    ResultObj->SetArrayField(TEXT("components"), ComponentArray);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleGetBlueprintDefaults(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString PropertyFilter;
    Params->TryGetStringField(TEXT("property_filter"), PropertyFilter);

    UBlueprint* Blueprint = FindBlueprintFlexible(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Note: removed auto-compile on read — it was causing material/WPO corruption
    // Compile only happens on set_blueprint_property (write operations)

    if (!Blueprint->GeneratedClass)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint has no GeneratedClass"));
    }

    UObject* CDO = Blueprint->GeneratedClass->GetDefaultObject();
    if (!CDO)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get class default object"));
    }

    TSharedPtr<FJsonObject> PropertiesObj = MakeShared<FJsonObject>();
    int32 PropertyCount = 0;

    for (TFieldIterator<FProperty> PropIt(Blueprint->GeneratedClass); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (!Property)
        {
            continue;
        }

        // Skip properties owned by engine base classes (UObject, AActor, APawn, ACharacter, etc.)
        UStruct* OwnerStruct = Property->GetOwnerStruct();
        if (OwnerStruct)
        {
            FString OwnerName = OwnerStruct->GetName();
            // Only include properties from the Blueprint's own generated class or its direct parents
            // that are Blueprint-generated (not engine C++ classes)
            if (OwnerName == TEXT("Object") ||
                OwnerName == TEXT("Actor") ||
                OwnerName == TEXT("Pawn") ||
                OwnerName == TEXT("Character") ||
                OwnerName == TEXT("Controller") ||
                OwnerName == TEXT("PlayerController") ||
                OwnerName == TEXT("GameModeBase") ||
                OwnerName == TEXT("GameMode") ||
                OwnerName == TEXT("Info") ||
                OwnerName == TEXT("HUD") ||
                OwnerName == TEXT("PlayerState") ||
                OwnerName == TEXT("GameStateBase") ||
                OwnerName == TEXT("GameState"))
            {
                continue;
            }
        }

        FString PropName = Property->GetName();

        // Apply prefix filter if specified
        if (!PropertyFilter.IsEmpty() && !PropName.StartsWith(PropertyFilter))
        {
            continue;
        }

        // Get property value as string using ExportText
        FString ValueStr;
        const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(CDO);
        Property->ExportText_Direct(ValueStr, ValuePtr, ValuePtr, CDO, PPF_None);

        // Build property info
        TSharedPtr<FJsonObject> PropInfo = MakeShared<FJsonObject>();
        PropInfo->SetStringField(TEXT("type"), Property->GetCPPType());
        PropInfo->SetStringField(TEXT("value"), ValueStr);

        // Category
#if WITH_EDITORONLY_DATA
        if (Property->HasMetaData(TEXT("Category")))
        {
            PropInfo->SetStringField(TEXT("category"), Property->GetMetaData(TEXT("Category")));
        }
#endif

        PropertiesObj->SetObjectField(PropName, PropInfo);
        PropertyCount++;
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint"), BlueprintName);
    ResultObj->SetNumberField(TEXT("property_count"), PropertyCount);
    ResultObj->SetObjectField(TEXT("properties"), PropertiesObj);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleListAssets(const TSharedPtr<FJsonObject>& Params)
{
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'path' parameter"));
    }

    FString TypeFilter;
    Params->TryGetStringField(TEXT("type"), TypeFilter);

    bool bRecursive = true;
    if (Params->HasField(TEXT("recursive")))
    {
        bRecursive = Params->GetBoolField(TEXT("recursive"));
    }

    IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

    TArray<FAssetData> Assets;
    AssetRegistry.GetAssetsByPath(FName(*Path), Assets, bRecursive);

    TArray<TSharedPtr<FJsonValue>> AssetArray;
    for (const FAssetData& Asset : Assets)
    {
        // Type filter
        if (!TypeFilter.IsEmpty())
        {
            FString AssetClassName = Asset.AssetClassPath.GetAssetName().ToString();
            if (!AssetClassName.Contains(TypeFilter))
            {
                continue;
            }
        }

        TSharedPtr<FJsonObject> AssetObj = MakeShared<FJsonObject>();
        AssetObj->SetStringField(TEXT("name"), Asset.AssetName.ToString());
        AssetObj->SetStringField(TEXT("path"), Asset.GetObjectPathString());
        AssetObj->SetStringField(TEXT("class"), Asset.AssetClassPath.GetAssetName().ToString());
        AssetObj->SetStringField(TEXT("package"), Asset.PackageName.ToString());

        // Disk size if available
        int64 DiskSize = Asset.GetTagValueRef<int64>(FName("Stage"));
        // Use package file size instead
        const FString PackagePath = Asset.PackageName.ToString();
        FString FilePath;
        if (FPackageName::TryConvertLongPackageNameToFilename(PackagePath, FilePath, FPackageName::GetAssetPackageExtension()))
        {
            int64 FileSize = IFileManager::Get().FileSize(*FilePath);
            if (FileSize >= 0)
            {
                AssetObj->SetNumberField(TEXT("disk_size_bytes"), (double)FileSize);
            }
        }

        AssetArray.Add(MakeShared<FJsonValueObject>(AssetObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("path"), Path);
    ResultObj->SetNumberField(TEXT("asset_count"), AssetArray.Num());
    ResultObj->SetArrayField(TEXT("assets"), AssetArray);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleGetAssetDetails(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

    FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(AssetPath));
    if (!AssetData.IsValid())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
    ResultObj->SetStringField(TEXT("path"), AssetData.GetObjectPathString());
    ResultObj->SetStringField(TEXT("class"), AssetData.AssetClassPath.GetAssetName().ToString());
    ResultObj->SetStringField(TEXT("package"), AssetData.PackageName.ToString());

    // Disk size
    const FString PackagePath = AssetData.PackageName.ToString();
    FString FilePath;
    if (FPackageName::TryConvertLongPackageNameToFilename(PackagePath, FilePath, FPackageName::GetAssetPackageExtension()))
    {
        int64 FileSize = IFileManager::Get().FileSize(*FilePath);
        if (FileSize >= 0)
        {
            ResultObj->SetNumberField(TEXT("disk_size_bytes"), (double)FileSize);
        }
    }

    // Dependencies
    TArray<FName> Dependencies;
    AssetRegistry.GetDependencies(AssetData.PackageName, Dependencies);

    TArray<TSharedPtr<FJsonValue>> DepArray;
    for (const FName& Dep : Dependencies)
    {
        DepArray.Add(MakeShared<FJsonValueString>(Dep.ToString()));
    }
    ResultObj->SetArrayField(TEXT("dependencies"), DepArray);

    // Referencers
    TArray<FName> Referencers;
    AssetRegistry.GetReferencers(AssetData.PackageName, Referencers);

    TArray<TSharedPtr<FJsonValue>> RefArray;
    for (const FName& Ref : Referencers)
    {
        RefArray.Add(MakeShared<FJsonValueString>(Ref.ToString()));
    }
    ResultObj->SetArrayField(TEXT("referencers"), RefArray);

    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleGetMaterialInfo(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *AssetPath);
    if (!Material)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *AssetPath));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), Material->GetName());
    ResultObj->SetStringField(TEXT("path"), Material->GetPathName());
    ResultObj->SetStringField(TEXT("class"), Material->GetClass()->GetName());

    // Try casting to MaterialInstance for parameter values
    UMaterialInstance* MatInstance = Cast<UMaterialInstance>(Material);
    if (MatInstance)
    {
        // Parent material
        if (MatInstance->Parent)
        {
            ResultObj->SetStringField(TEXT("parent"), MatInstance->Parent->GetPathName());
        }

        // Scalar parameters
        TSharedPtr<FJsonObject> ScalarParams = MakeShared<FJsonObject>();
        for (const FScalarParameterValue& Param : MatInstance->ScalarParameterValues)
        {
            ScalarParams->SetNumberField(Param.ParameterInfo.Name.ToString(), Param.ParameterValue);
        }
        ResultObj->SetObjectField(TEXT("scalar_params"), ScalarParams);

        // Vector parameters
        TSharedPtr<FJsonObject> VectorParams = MakeShared<FJsonObject>();
        for (const FVectorParameterValue& Param : MatInstance->VectorParameterValues)
        {
            TArray<TSharedPtr<FJsonValue>> ColorArr;
            ColorArr.Add(MakeShared<FJsonValueNumber>(Param.ParameterValue.R));
            ColorArr.Add(MakeShared<FJsonValueNumber>(Param.ParameterValue.G));
            ColorArr.Add(MakeShared<FJsonValueNumber>(Param.ParameterValue.B));
            ColorArr.Add(MakeShared<FJsonValueNumber>(Param.ParameterValue.A));
            VectorParams->SetArrayField(Param.ParameterInfo.Name.ToString(), ColorArr);
        }
        ResultObj->SetObjectField(TEXT("vector_params"), VectorParams);

        // Texture parameters
        TSharedPtr<FJsonObject> TextureParams = MakeShared<FJsonObject>();
        for (const FTextureParameterValue& Param : MatInstance->TextureParameterValues)
        {
            FString TexPath = Param.ParameterValue ? Param.ParameterValue->GetPathName() : TEXT("None");
            TextureParams->SetStringField(Param.ParameterInfo.Name.ToString(), TexPath);
        }
        ResultObj->SetObjectField(TEXT("texture_params"), TextureParams);
    }

    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleGetStaticMeshInfo(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *AssetPath);
    if (!Mesh)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Static mesh not found: %s"), *AssetPath));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), Mesh->GetName());
    ResultObj->SetStringField(TEXT("path"), Mesh->GetPathName());

    // Bounding box
    FBox BBox = Mesh->GetBoundingBox();
    TSharedPtr<FJsonObject> BoundsObj = MakeShared<FJsonObject>();

    TArray<TSharedPtr<FJsonValue>> MinArr;
    MinArr.Add(MakeShared<FJsonValueNumber>(BBox.Min.X));
    MinArr.Add(MakeShared<FJsonValueNumber>(BBox.Min.Y));
    MinArr.Add(MakeShared<FJsonValueNumber>(BBox.Min.Z));
    BoundsObj->SetArrayField(TEXT("min"), MinArr);

    TArray<TSharedPtr<FJsonValue>> MaxArr;
    MaxArr.Add(MakeShared<FJsonValueNumber>(BBox.Max.X));
    MaxArr.Add(MakeShared<FJsonValueNumber>(BBox.Max.Y));
    MaxArr.Add(MakeShared<FJsonValueNumber>(BBox.Max.Z));
    BoundsObj->SetArrayField(TEXT("max"), MaxArr);

    FVector Size = BBox.GetSize();
    TArray<TSharedPtr<FJsonValue>> SizeArr;
    SizeArr.Add(MakeShared<FJsonValueNumber>(Size.X));
    SizeArr.Add(MakeShared<FJsonValueNumber>(Size.Y));
    SizeArr.Add(MakeShared<FJsonValueNumber>(Size.Z));
    BoundsObj->SetArrayField(TEXT("size"), SizeArr);

    ResultObj->SetObjectField(TEXT("bounds"), BoundsObj);

    // LOD info
    FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
    int32 NumLODs = RenderData ? RenderData->LODResources.Num() : 0;
    ResultObj->SetNumberField(TEXT("lod_count"), NumLODs);

    TArray<TSharedPtr<FJsonValue>> LodArray;
    if (RenderData)
    {
        for (int32 i = 0; i < NumLODs; i++)
        {
            const FStaticMeshLODResources& LOD = RenderData->LODResources[i];
            TSharedPtr<FJsonObject> LodObj = MakeShared<FJsonObject>();
            LodObj->SetNumberField(TEXT("vertices"), LOD.GetNumVertices());
            LodObj->SetNumberField(TEXT("triangles"), LOD.GetNumTriangles());
            LodArray.Add(MakeShared<FJsonValueObject>(LodObj));
        }
    }
    ResultObj->SetArrayField(TEXT("lods"), LodArray);

    // Material slots
    TArray<TSharedPtr<FJsonValue>> MatSlotArray;
    const TArray<FStaticMaterial>& StaticMaterials = Mesh->GetStaticMaterials();
    for (const FStaticMaterial& SlotMat : StaticMaterials)
    {
        TSharedPtr<FJsonObject> SlotObj = MakeShared<FJsonObject>();
        SlotObj->SetStringField(TEXT("name"), SlotMat.MaterialSlotName.ToString());
        FString MatPath = SlotMat.MaterialInterface ? SlotMat.MaterialInterface->GetPathName() : TEXT("None");
        SlotObj->SetStringField(TEXT("material_path"), MatPath);
        MatSlotArray.Add(MakeShared<FJsonValueObject>(SlotObj));
    }
    ResultObj->SetArrayField(TEXT("material_slots"), MatSlotArray);

    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleGetComponentProperties(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    FString PropertyFilter;
    Params->TryGetStringField(TEXT("property_filter"), PropertyFilter);

    UBlueprint* Blueprint = FindBlueprintFlexible(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    if (!Blueprint->SimpleConstructionScript)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint has no SimpleConstructionScript"));
    }

    // Find the component node by variable name
    UActorComponent* ComponentTemplate = nullptr;
    TArray<USCS_Node*> AllNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
    for (USCS_Node* Node : AllNodes)
    {
        if (Node && Node->ComponentTemplate && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentTemplate = Node->ComponentTemplate;
            break;
        }
    }

    if (!ComponentTemplate)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    TSharedPtr<FJsonObject> PropertiesObj = MakeShared<FJsonObject>();
    int32 PropertyCount = 0;

    for (TFieldIterator<FProperty> PropIt(ComponentTemplate->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (!Property)
        {
            continue;
        }

        // Skip properties from engine base classes
        UStruct* OwnerStruct = Property->GetOwnerStruct();
        if (OwnerStruct)
        {
            FString OwnerName = OwnerStruct->GetName();
            if (OwnerName == TEXT("Object") ||
                OwnerName == TEXT("ActorComponent") ||
                OwnerName == TEXT("SceneComponent"))
            {
                continue;
            }
        }

        FString PropName = Property->GetName();

        // Apply prefix filter if specified
        if (!PropertyFilter.IsEmpty() && !PropName.StartsWith(PropertyFilter))
        {
            continue;
        }

        // Get property value as string using ExportText
        FString ValueStr;
        const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(ComponentTemplate);
        Property->ExportText_Direct(ValueStr, ValuePtr, ValuePtr, ComponentTemplate, PPF_None);

        // Build property info
        TSharedPtr<FJsonObject> PropInfo = MakeShared<FJsonObject>();
        PropInfo->SetStringField(TEXT("type"), Property->GetCPPType());
        PropInfo->SetStringField(TEXT("value"), ValueStr);

#if WITH_EDITORONLY_DATA
        if (Property->HasMetaData(TEXT("Category")))
        {
            PropInfo->SetStringField(TEXT("category"), Property->GetMetaData(TEXT("Category")));
        }
#endif

        PropertiesObj->SetObjectField(PropName, PropInfo);
        PropertyCount++;
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint"), BlueprintName);
    ResultObj->SetStringField(TEXT("component"), ComponentName);
    ResultObj->SetStringField(TEXT("component_type"), ComponentTemplate->GetClass()->GetName());
    ResultObj->SetNumberField(TEXT("property_count"), PropertyCount);
    ResultObj->SetObjectField(TEXT("properties"), PropertiesObj);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleGetBlueprintGraph(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeTypeFilter;
    Params->TryGetStringField(TEXT("node_type_filter"), NodeTypeFilter);

    UBlueprint* Blueprint = FindBlueprintFlexible(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    TArray<TSharedPtr<FJsonValue>> NodesArray;

    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (!Node)
        {
            continue;
        }

        FString NodeClassName = Node->GetClass()->GetName();

        // Apply type filter if specified
        if (!NodeTypeFilter.IsEmpty() && !NodeClassName.Contains(NodeTypeFilter))
        {
            continue;
        }

        TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
        NodeObj->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
        NodeObj->SetStringField(TEXT("node_type"), NodeClassName);
        NodeObj->SetStringField(TEXT("node_title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());

        // Position
        TArray<TSharedPtr<FJsonValue>> PosArr;
        PosArr.Add(MakeShared<FJsonValueNumber>(Node->NodePosX));
        PosArr.Add(MakeShared<FJsonValueNumber>(Node->NodePosY));
        NodeObj->SetArrayField(TEXT("position"), PosArr);

        // Compact pin info
        TArray<TSharedPtr<FJsonValue>> PinsArray;
        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (!Pin)
            {
                continue;
            }

            TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
            PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
            PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));
            PinObj->SetStringField(TEXT("category"), Pin->PinType.PinCategory.ToString());
            PinObj->SetBoolField(TEXT("is_connected"), Pin->LinkedTo.Num() > 0);

            // Connections
            if (Pin->LinkedTo.Num() > 0)
            {
                TArray<TSharedPtr<FJsonValue>> ConnArray;
                for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                {
                    if (!LinkedPin || !LinkedPin->GetOwningNode())
                    {
                        continue;
                    }
                    TSharedPtr<FJsonObject> ConnObj = MakeShared<FJsonObject>();
                    ConnObj->SetStringField(TEXT("node_id"), LinkedPin->GetOwningNode()->NodeGuid.ToString());
                    ConnObj->SetStringField(TEXT("pin_name"), LinkedPin->PinName.ToString());
                    ConnArray.Add(MakeShared<FJsonValueObject>(ConnObj));
                }
                PinObj->SetArrayField(TEXT("connected_to"), ConnArray);
            }

            PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
        }
        NodeObj->SetArrayField(TEXT("pins"), PinsArray);

        NodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint"), BlueprintName);
    ResultObj->SetNumberField(TEXT("node_count"), NodesArray.Num());
    ResultObj->SetArrayField(TEXT("nodes"), NodesArray);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleGetNodePins(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeIdStr;
    if (!Params->TryGetStringField(TEXT("node_id"), NodeIdStr))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
    }

    UBlueprint* Blueprint = FindBlueprintFlexible(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Find node by GUID
    FGuid TargetGuid;
    if (!FGuid::Parse(NodeIdStr, TargetGuid))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Invalid GUID: %s"), *NodeIdStr));
    }

    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node && Node->NodeGuid == TargetGuid)
        {
            TargetNode = Node;
            break;
        }
    }

    if (!TargetNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeIdStr));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), NodeIdStr);
    ResultObj->SetStringField(TEXT("node_type"), TargetNode->GetClass()->GetName());
    ResultObj->SetStringField(TEXT("node_title"), TargetNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString());

    TArray<TSharedPtr<FJsonValue>> PinsArray;
    for (UEdGraphPin* Pin : TargetNode->Pins)
    {
        if (!Pin)
        {
            continue;
        }

        TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
        PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
        PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));
        PinObj->SetStringField(TEXT("category"), Pin->PinType.PinCategory.ToString());

        // Sub-category
        FString SubCategory;
        if (Pin->PinType.PinSubCategoryObject.IsValid())
        {
            SubCategory = Pin->PinType.PinSubCategoryObject->GetName();
        }
        PinObj->SetStringField(TEXT("sub_category"), SubCategory);

        // Default value
        PinObj->SetStringField(TEXT("default_value"), Pin->DefaultValue);

        // Connected
        PinObj->SetBoolField(TEXT("is_connected"), Pin->LinkedTo.Num() > 0);

        // Connections with full detail
        TArray<TSharedPtr<FJsonValue>> ConnArray;
        for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
        {
            if (!LinkedPin || !LinkedPin->GetOwningNode())
            {
                continue;
            }
            TSharedPtr<FJsonObject> ConnObj = MakeShared<FJsonObject>();
            ConnObj->SetStringField(TEXT("node_id"), LinkedPin->GetOwningNode()->NodeGuid.ToString());
            ConnObj->SetStringField(TEXT("pin_name"), LinkedPin->PinName.ToString());
            ConnArray.Add(MakeShared<FJsonValueObject>(ConnObj));
        }
        PinObj->SetArrayField(TEXT("connections"), ConnArray);

        PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
    }

    ResultObj->SetNumberField(TEXT("pin_count"), PinsArray.Num());
    ResultObj->SetArrayField(TEXT("pins"), PinsArray);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleListNaniteStatus(const TSharedPtr<FJsonObject>& Params)
{
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'path' parameter"));
    }

    // filter: "all" (default), "enabled", "disabled"
    FString Filter = TEXT("all");
    Params->TryGetStringField(TEXT("filter"), Filter);

    bool bRecursive = true;
    if (Params->HasField(TEXT("recursive")))
    {
        bRecursive = Params->GetBoolField(TEXT("recursive"));
    }

    IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
    TArray<FAssetData> Assets;
    FARFilter ARFilter;
    ARFilter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("StaticMesh")));
    ARFilter.PackagePaths.Add(*Path);
    ARFilter.bRecursivePaths = bRecursive;
    AssetRegistry.GetAssets(ARFilter, Assets);

    int32 TotalCount = 0;
    int32 EnabledCount = 0;
    int32 DisabledCount = 0;
    TArray<TSharedPtr<FJsonValue>> MeshArray;

    for (const FAssetData& Asset : Assets)
    {
        UStaticMesh* Mesh = Cast<UStaticMesh>(Asset.GetAsset());
        if (!Mesh) continue;

        TotalCount++;
        const bool bNaniteEnabled = Mesh->IsNaniteEnabled();
        if (bNaniteEnabled) EnabledCount++; else DisabledCount++;

        if (Filter == TEXT("enabled") && !bNaniteEnabled) continue;
        if (Filter == TEXT("disabled") && bNaniteEnabled) continue;

        TSharedPtr<FJsonObject> MeshObj = MakeShared<FJsonObject>();
        MeshObj->SetStringField(TEXT("name"), Asset.AssetName.ToString());
        MeshObj->SetStringField(TEXT("path"), Asset.GetObjectPathString());
        MeshObj->SetBoolField(TEXT("nanite_enabled"), bNaniteEnabled);
        MeshArray.Add(MakeShared<FJsonValueObject>(MeshObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("path"), Path);
    ResultObj->SetStringField(TEXT("filter"), Filter);
    ResultObj->SetNumberField(TEXT("total_count"), TotalCount);
    ResultObj->SetNumberField(TEXT("nanite_enabled_count"), EnabledCount);
    ResultObj->SetNumberField(TEXT("nanite_disabled_count"), DisabledCount);
    ResultObj->SetNumberField(TEXT("returned_count"), MeshArray.Num());
    ResultObj->SetArrayField(TEXT("meshes"), MeshArray);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleSetStaticMeshNaniteEnabled(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    bool bEnabled = true;
    if (Params->HasField(TEXT("enabled")))
    {
        bEnabled = Params->GetBoolField(TEXT("enabled"));
    }

    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *AssetPath);
    if (!Mesh)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Static mesh not found: %s"), *AssetPath));
    }

    const bool bWasEnabled = Mesh->IsNaniteEnabled();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("path"), Mesh->GetPathName());
    ResultObj->SetBoolField(TEXT("was_enabled"), bWasEnabled);

    if (bWasEnabled == bEnabled)
    {
        ResultObj->SetBoolField(TEXT("now_enabled"), bEnabled);
        ResultObj->SetBoolField(TEXT("changed"), false);
        return ResultObj;
    }

    Mesh->Modify();
    Mesh->NaniteSettings.bEnabled = bEnabled;
    Mesh->PostEditChange();           // triggers rebuild for the new Nanite state
    Mesh->MarkPackageDirty();         // user must Save All to persist

    ResultObj->SetBoolField(TEXT("now_enabled"), Mesh->IsNaniteEnabled());
    ResultObj->SetBoolField(TEXT("changed"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleGetMaterialGraph(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    bool bResolveInstance = true;
    if (Params->HasField(TEXT("resolve_instance")))
    {
        bResolveInstance = Params->GetBoolField(TEXT("resolve_instance"));
    }

    UMaterialInterface* MatIface = LoadObject<UMaterialInterface>(nullptr, *AssetPath);
    if (!MatIface)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *AssetPath));
    }

    // Resolve instance to base UMaterial if requested
    UMaterial* Material = nullptr;
    bool bWasInstance = false;
    FString OriginalInstancePath;

    if (UMaterialInstance* AsInstance = Cast<UMaterialInstance>(MatIface))
    {
        bWasInstance = true;
        OriginalInstancePath = AsInstance->GetPathName();
        if (bResolveInstance)
        {
            UMaterialInterface* Cur = AsInstance->Parent;
            while (UMaterialInstance* CurInst = Cast<UMaterialInstance>(Cur))
            {
                Cur = CurInst->Parent;
            }
            Material = Cast<UMaterial>(Cur);
        }
    }
    else
    {
        Material = Cast<UMaterial>(MatIface);
    }

    if (!Material)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not resolve to base UMaterial. Was instance: %s, resolve_instance: %s"),
                bWasInstance ? TEXT("true") : TEXT("false"),
                bResolveInstance ? TEXT("true") : TEXT("false")));
    }

    UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
    if (!EditorData)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material has no editor-only data"));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), Material->GetName());
    ResultObj->SetStringField(TEXT("path"), Material->GetPathName());
    ResultObj->SetStringField(TEXT("class"), Material->GetClass()->GetName());
    if (bWasInstance)
    {
        ResultObj->SetStringField(TEXT("resolved_from_instance"), OriginalInstancePath);
    }

    // Build expression list with stable indices (used as IDs in connections)
    const TArray<TObjectPtr<UMaterialExpression>>& AllExpressions = EditorData->ExpressionCollection.Expressions;

    TMap<UMaterialExpression*, int32> ExprIndexMap;
    for (int32 i = 0; i < AllExpressions.Num(); i++)
    {
        if (AllExpressions[i])
        {
            ExprIndexMap.Add(AllExpressions[i].Get(), i);
        }
    }

    TArray<TSharedPtr<FJsonValue>> ExpressionsArray;
    TArray<TSharedPtr<FJsonValue>> ConnectionsArray;

    for (int32 i = 0; i < AllExpressions.Num(); i++)
    {
        UMaterialExpression* Expr = AllExpressions[i].Get();
        if (!Expr) continue;

        TSharedPtr<FJsonObject> ExprObj = MakeShared<FJsonObject>();
        ExprObj->SetNumberField(TEXT("id"), i);

        // Type — strip "MaterialExpression" prefix for readability
        FString TypeName = Expr->GetClass()->GetName();
        if (TypeName.StartsWith(TEXT("MaterialExpression")))
        {
            TypeName = TypeName.Mid(18);
        }
        ExprObj->SetStringField(TEXT("type"), TypeName);

        // Editor position
        ExprObj->SetNumberField(TEXT("pos_x"), Expr->MaterialExpressionEditorX);
        ExprObj->SetNumberField(TEXT("pos_y"), Expr->MaterialExpressionEditorY);

        // Type-specific data
        if (UMaterialExpressionMaterialFunctionCall* FuncCall = Cast<UMaterialExpressionMaterialFunctionCall>(Expr))
        {
            if (FuncCall->MaterialFunction)
            {
                ExprObj->SetStringField(TEXT("function_path"), FuncCall->MaterialFunction->GetPathName());
                ExprObj->SetStringField(TEXT("function_name"), FuncCall->MaterialFunction->GetName());
            }
        }
        else if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(Expr))
        {
            ExprObj->SetStringField(TEXT("parameter_name"), ScalarParam->ParameterName.ToString());
            ExprObj->SetNumberField(TEXT("default_value"), ScalarParam->DefaultValue);
            if (!ScalarParam->Group.IsNone())
            {
                ExprObj->SetStringField(TEXT("group"), ScalarParam->Group.ToString());
            }
        }
        else if (UMaterialExpressionVectorParameter* VectorParam = Cast<UMaterialExpressionVectorParameter>(Expr))
        {
            ExprObj->SetStringField(TEXT("parameter_name"), VectorParam->ParameterName.ToString());
            TArray<TSharedPtr<FJsonValue>> ColorArr;
            ColorArr.Add(MakeShared<FJsonValueNumber>(VectorParam->DefaultValue.R));
            ColorArr.Add(MakeShared<FJsonValueNumber>(VectorParam->DefaultValue.G));
            ColorArr.Add(MakeShared<FJsonValueNumber>(VectorParam->DefaultValue.B));
            ColorArr.Add(MakeShared<FJsonValueNumber>(VectorParam->DefaultValue.A));
            ExprObj->SetArrayField(TEXT("default_value"), ColorArr);
            if (!VectorParam->Group.IsNone())
            {
                ExprObj->SetStringField(TEXT("group"), VectorParam->Group.ToString());
            }
        }
        else if (UMaterialExpressionTextureSampleParameter* TexParam = Cast<UMaterialExpressionTextureSampleParameter>(Expr))
        {
            ExprObj->SetStringField(TEXT("parameter_name"), TexParam->ParameterName.ToString());
            if (!TexParam->Group.IsNone())
            {
                ExprObj->SetStringField(TEXT("group"), TexParam->Group.ToString());
            }
        }
        else if (UMaterialExpressionConstant* Const1 = Cast<UMaterialExpressionConstant>(Expr))
        {
            ExprObj->SetNumberField(TEXT("value"), Const1->R);
        }
        else if (UMaterialExpressionConstant2Vector* Const2 = Cast<UMaterialExpressionConstant2Vector>(Expr))
        {
            TArray<TSharedPtr<FJsonValue>> Arr;
            Arr.Add(MakeShared<FJsonValueNumber>(Const2->R));
            Arr.Add(MakeShared<FJsonValueNumber>(Const2->G));
            ExprObj->SetArrayField(TEXT("value"), Arr);
        }
        else if (UMaterialExpressionConstant3Vector* Const3 = Cast<UMaterialExpressionConstant3Vector>(Expr))
        {
            TArray<TSharedPtr<FJsonValue>> Arr;
            Arr.Add(MakeShared<FJsonValueNumber>(Const3->Constant.R));
            Arr.Add(MakeShared<FJsonValueNumber>(Const3->Constant.G));
            Arr.Add(MakeShared<FJsonValueNumber>(Const3->Constant.B));
            ExprObj->SetArrayField(TEXT("value"), Arr);
        }
        else if (UMaterialExpressionConstant4Vector* Const4 = Cast<UMaterialExpressionConstant4Vector>(Expr))
        {
            TArray<TSharedPtr<FJsonValue>> Arr;
            Arr.Add(MakeShared<FJsonValueNumber>(Const4->Constant.R));
            Arr.Add(MakeShared<FJsonValueNumber>(Const4->Constant.G));
            Arr.Add(MakeShared<FJsonValueNumber>(Const4->Constant.B));
            Arr.Add(MakeShared<FJsonValueNumber>(Const4->Constant.A));
            ExprObj->SetArrayField(TEXT("value"), Arr);
        }
        else if (UMaterialExpressionComment* Comment = Cast<UMaterialExpressionComment>(Expr))
        {
            ExprObj->SetStringField(TEXT("text"), Comment->Text);
        }
        else if (UMaterialExpressionAdd* AddExpr = Cast<UMaterialExpressionAdd>(Expr))
        {
            ExprObj->SetNumberField(TEXT("const_a"), AddExpr->ConstA);
            ExprObj->SetNumberField(TEXT("const_b"), AddExpr->ConstB);
        }
        else if (UMaterialExpressionMultiply* MulExpr = Cast<UMaterialExpressionMultiply>(Expr))
        {
            ExprObj->SetNumberField(TEXT("const_a"), MulExpr->ConstA);
            ExprObj->SetNumberField(TEXT("const_b"), MulExpr->ConstB);
        }
        else if (UMaterialExpressionSubtract* SubExpr = Cast<UMaterialExpressionSubtract>(Expr))
        {
            ExprObj->SetNumberField(TEXT("const_a"), SubExpr->ConstA);
            ExprObj->SetNumberField(TEXT("const_b"), SubExpr->ConstB);
        }
        else if (UMaterialExpressionDivide* DivExpr = Cast<UMaterialExpressionDivide>(Expr))
        {
            ExprObj->SetNumberField(TEXT("const_a"), DivExpr->ConstA);
            ExprObj->SetNumberField(TEXT("const_b"), DivExpr->ConstB);
        }
        else if (UMaterialExpressionCustom* CustomExpr = Cast<UMaterialExpressionCustom>(Expr))
        {
            ExprObj->SetStringField(TEXT("code"), CustomExpr->Code);
            ExprObj->SetStringField(TEXT("description"), CustomExpr->Description);
        }

        ExpressionsArray.Add(MakeShared<FJsonValueObject>(ExprObj));

        // Walk inputs to build connection list
        TArrayView<FExpressionInput*> Inputs = Expr->GetInputsView();
        for (int32 InputIdx = 0; InputIdx < Inputs.Num(); InputIdx++)
        {
            FExpressionInput* Input = Inputs[InputIdx];
            if (Input && Input->Expression)
            {
                int32* FromIdx = ExprIndexMap.Find(Input->Expression);
                if (FromIdx)
                {
                    TSharedPtr<FJsonObject> ConnObj = MakeShared<FJsonObject>();
                    ConnObj->SetNumberField(TEXT("from_id"), *FromIdx);
                    ConnObj->SetNumberField(TEXT("from_output"), Input->OutputIndex);
                    ConnObj->SetNumberField(TEXT("to_id"), i);
                    ConnObj->SetNumberField(TEXT("to_input"), InputIdx);

                    FName InputName = Expr->GetInputName(InputIdx);
                    if (!InputName.IsNone())
                    {
                        ConnObj->SetStringField(TEXT("to_input_name"), InputName.ToString());
                    }

                    ConnectionsArray.Add(MakeShared<FJsonValueObject>(ConnObj));
                }
            }
        }
    }

    ResultObj->SetNumberField(TEXT("expression_count"), ExpressionsArray.Num());
    ResultObj->SetArrayField(TEXT("expressions"), ExpressionsArray);
    ResultObj->SetArrayField(TEXT("connections"), ConnectionsArray);

    // Material attribute inputs (top-level pins on the Material output node)
    TSharedPtr<FJsonObject> InputsObj = MakeShared<FJsonObject>();

    auto AddInput = [&](const FString& Key, const FExpressionInput& Input)
    {
        TSharedPtr<FJsonObject> InputObj = MakeShared<FJsonObject>();
        if (Input.Expression)
        {
            int32* IdxPtr = ExprIndexMap.Find(Input.Expression);
            if (IdxPtr)
            {
                InputObj->SetBoolField(TEXT("connected"), true);
                InputObj->SetNumberField(TEXT("expression_id"), *IdxPtr);
                InputObj->SetNumberField(TEXT("output_index"), Input.OutputIndex);
            }
            else
            {
                InputObj->SetBoolField(TEXT("connected"), false);
            }
        }
        else
        {
            InputObj->SetBoolField(TEXT("connected"), false);
        }
        InputsObj->SetObjectField(Key, InputObj);
    };

    AddInput(TEXT("base_color"), EditorData->BaseColor);
    AddInput(TEXT("metallic"), EditorData->Metallic);
    AddInput(TEXT("specular"), EditorData->Specular);
    AddInput(TEXT("roughness"), EditorData->Roughness);
    AddInput(TEXT("anisotropy"), EditorData->Anisotropy);
    AddInput(TEXT("emissive_color"), EditorData->EmissiveColor);
    AddInput(TEXT("opacity"), EditorData->Opacity);
    AddInput(TEXT("opacity_mask"), EditorData->OpacityMask);
    AddInput(TEXT("normal"), EditorData->Normal);
    AddInput(TEXT("tangent"), EditorData->Tangent);
    AddInput(TEXT("world_position_offset"), EditorData->WorldPositionOffset);
    AddInput(TEXT("subsurface_color"), EditorData->SubsurfaceColor);
    AddInput(TEXT("clear_coat"), EditorData->ClearCoat);
    AddInput(TEXT("clear_coat_roughness"), EditorData->ClearCoatRoughness);
    AddInput(TEXT("ambient_occlusion"), EditorData->AmbientOcclusion);
    AddInput(TEXT("refraction"), EditorData->Refraction);
    AddInput(TEXT("pixel_depth_offset"), EditorData->PixelDepthOffset);
    AddInput(TEXT("material_attributes"), EditorData->MaterialAttributes);

    ResultObj->SetObjectField(TEXT("material_inputs"), InputsObj);

    // Material settings
    ResultObj->SetBoolField(TEXT("use_material_attributes"), Material->bUseMaterialAttributes);
    ResultObj->SetStringField(TEXT("blend_mode"),
        UEnum::GetValueAsString(Material->BlendMode));
    ResultObj->SetStringField(TEXT("shading_model"),
        UEnum::GetValueAsString(Material->GetShadingModels().GetFirstShadingModel()));

    return ResultObj;
}

// =============================================================================
// Material editing helpers
// =============================================================================

// Resolve a material expression class name (with or without "MaterialExpression" prefix)
// to a UClass*. Returns nullptr if not found.
static UClass* ResolveMaterialExpressionClass(const FString& TypeName)
{
    FString ClassName = TypeName;
    if (!ClassName.StartsWith(TEXT("MaterialExpression")))
    {
        ClassName = TEXT("MaterialExpression") + ClassName;
    }
    // All material expression classes live in /Script/Engine
    const FString ClassPath = FString::Printf(TEXT("/Script/Engine.%s"), *ClassName);
    UClass* Class = LoadObject<UClass>(nullptr, *ClassPath);
    if (Class && Class->IsChildOf(UMaterialExpression::StaticClass()))
    {
        return Class;
    }
    return nullptr;
}

// Map a string property name to EMaterialProperty enum.
// Accepts "world_position_offset", "WorldPositionOffset", "MP_WorldPositionOffset", "wpo", etc.
static EMaterialProperty StringToMaterialProperty(const FString& Name)
{
    FString N = Name;
    N = N.Replace(TEXT("_"), TEXT(""));
    N = N.Replace(TEXT("MP"), TEXT(""), ESearchCase::CaseSensitive);
    N = N.ToLower();

    if (N == TEXT("basecolor")) return MP_BaseColor;
    if (N == TEXT("metallic")) return MP_Metallic;
    if (N == TEXT("specular")) return MP_Specular;
    if (N == TEXT("roughness")) return MP_Roughness;
    if (N == TEXT("anisotropy")) return MP_Anisotropy;
    if (N == TEXT("emissivecolor") || N == TEXT("emissive")) return MP_EmissiveColor;
    if (N == TEXT("opacity")) return MP_Opacity;
    if (N == TEXT("opacitymask")) return MP_OpacityMask;
    if (N == TEXT("normal")) return MP_Normal;
    if (N == TEXT("tangent")) return MP_Tangent;
    if (N == TEXT("worldpositionoffset") || N == TEXT("wpo")) return MP_WorldPositionOffset;
    if (N == TEXT("subsurfacecolor") || N == TEXT("subsurface")) return MP_SubsurfaceColor;
    if (N == TEXT("ambientocclusion") || N == TEXT("ao")) return MP_AmbientOcclusion;
    if (N == TEXT("refraction")) return MP_Refraction;
    if (N == TEXT("pixeldepthoffset") || N == TEXT("pdo")) return MP_PixelDepthOffset;
    if (N == TEXT("clearcoat")) return MP_CustomData0;
    if (N == TEXT("clearcoatroughness")) return MP_CustomData1;
    return MP_MAX;
}

// Find an expression in a material by index (the same indexing used by get_material_graph)
static UMaterialExpression* FindMaterialExpressionByIndex(UMaterial* Material, int32 Index)
{
    if (!Material) return nullptr;
    UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
    if (!EditorData) return nullptr;
    const TArray<TObjectPtr<UMaterialExpression>>& Exprs = EditorData->ExpressionCollection.Expressions;
    if (Index < 0 || Index >= Exprs.Num()) return nullptr;
    return Exprs[Index].Get();
}

// =============================================================================
// Material editing handlers
// =============================================================================

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleSetMaterialInstanceParent(const TSharedPtr<FJsonObject>& Params)
{
    FString InstancePath, ParentPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), InstancePath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }
    if (!Params->TryGetStringField(TEXT("parent_path"), ParentPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parent_path' parameter"));
    }

    UMaterialInstanceConstant* Instance = LoadObject<UMaterialInstanceConstant>(nullptr, *InstancePath);
    if (!Instance)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Material instance not found (must be MaterialInstanceConstant): %s"), *InstancePath));
    }

    UMaterialInterface* NewParent = LoadObject<UMaterialInterface>(nullptr, *ParentPath);
    if (!NewParent)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Parent material not found: %s"), *ParentPath));
    }

    const FString OldParent = Instance->Parent ? Instance->Parent->GetPathName() : TEXT("None");

    UMaterialEditingLibrary::SetMaterialInstanceParent(Instance, NewParent);
    Instance->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("instance"), Instance->GetPathName());
    Result->SetStringField(TEXT("old_parent"), OldParent);
    Result->SetStringField(TEXT("new_parent"), NewParent->GetPathName());
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleAddMaterialExpression(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath, ExpressionType;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }
    if (!Params->TryGetStringField(TEXT("expression_type"), ExpressionType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'expression_type' parameter"));
    }

    int32 PosX = 0, PosY = 0;
    Params->TryGetNumberField(TEXT("pos_x"), PosX);
    Params->TryGetNumberField(TEXT("pos_y"), PosY);

    UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
    if (!Material)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
    }

    UClass* ExprClass = ResolveMaterialExpressionClass(ExpressionType);
    if (!ExprClass)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Material expression class not found: %s"), *ExpressionType));
    }

    UMaterialExpression* NewExpr = UMaterialEditingLibrary::CreateMaterialExpression(Material, ExprClass, PosX, PosY);
    if (!NewExpr)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("CreateMaterialExpression returned null"));
    }

    Material->MarkPackageDirty();

    // Find the new expression's index in the collection
    int32 NewIndex = INDEX_NONE;
    const TArray<TObjectPtr<UMaterialExpression>>& Exprs = Material->GetEditorOnlyData()->ExpressionCollection.Expressions;
    for (int32 i = 0; i < Exprs.Num(); i++)
    {
        if (Exprs[i] == NewExpr)
        {
            NewIndex = i;
            break;
        }
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("material"), Material->GetPathName());
    Result->SetNumberField(TEXT("expression_id"), NewIndex);
    FString TypeName = NewExpr->GetClass()->GetName();
    if (TypeName.StartsWith(TEXT("MaterialExpression")))
    {
        TypeName = TypeName.Mid(18);
    }
    Result->SetStringField(TEXT("type"), TypeName);
    Result->SetNumberField(TEXT("pos_x"), NewExpr->MaterialExpressionEditorX);
    Result->SetNumberField(TEXT("pos_y"), NewExpr->MaterialExpressionEditorY);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleDeleteMaterialExpression(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }
    int32 ExpressionId = -1;
    if (!Params->TryGetNumberField(TEXT("expression_id"), ExpressionId))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'expression_id' parameter"));
    }

    UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
    if (!Material)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
    }

    UMaterialExpression* Expr = FindMaterialExpressionByIndex(Material, ExpressionId);
    if (!Expr)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Expression id %d not found in %s"), ExpressionId, *MaterialPath));
    }

    UMaterialEditingLibrary::DeleteMaterialExpression(Material, Expr);
    Material->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("material"), Material->GetPathName());
    Result->SetNumberField(TEXT("deleted_expression_id"), ExpressionId);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("note"), TEXT("Indices of remaining expressions may shift after deletion."));
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleConnectMaterialExpressions(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }
    int32 FromId = -1, ToId = -1;
    if (!Params->TryGetNumberField(TEXT("from_id"), FromId))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'from_id' parameter"));
    }
    if (!Params->TryGetNumberField(TEXT("to_id"), ToId))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'to_id' parameter"));
    }
    FString FromOutputName, ToInputName;
    Params->TryGetStringField(TEXT("from_output_name"), FromOutputName);
    Params->TryGetStringField(TEXT("to_input_name"), ToInputName);

    UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
    if (!Material)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
    }

    UMaterialExpression* FromExpr = FindMaterialExpressionByIndex(Material, FromId);
    UMaterialExpression* ToExpr = FindMaterialExpressionByIndex(Material, ToId);
    if (!FromExpr || !ToExpr)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("from_id or to_id is invalid"));
    }

    const bool bConnected = UMaterialEditingLibrary::ConnectMaterialExpressions(FromExpr, FromOutputName, ToExpr, ToInputName);
    if (bConnected)
    {
        Material->MarkPackageDirty();
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("material"), Material->GetPathName());
    Result->SetBoolField(TEXT("connected"), bConnected);
    Result->SetNumberField(TEXT("from_id"), FromId);
    Result->SetNumberField(TEXT("to_id"), ToId);
    Result->SetStringField(TEXT("from_output_name"), FromOutputName);
    Result->SetStringField(TEXT("to_input_name"), ToInputName);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleConnectMaterialProperty(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath, PropertyName;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }
    int32 FromId = -1;
    if (!Params->TryGetNumberField(TEXT("from_id"), FromId))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'from_id' parameter"));
    }
    FString FromOutputName;
    Params->TryGetStringField(TEXT("from_output_name"), FromOutputName);

    UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
    if (!Material)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
    }
    UMaterialExpression* FromExpr = FindMaterialExpressionByIndex(Material, FromId);
    if (!FromExpr)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Expression id %d not found"), FromId));
    }
    EMaterialProperty Prop = StringToMaterialProperty(PropertyName);
    if (Prop == MP_MAX)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unknown material property: %s"), *PropertyName));
    }

    const bool bConnected = UMaterialEditingLibrary::ConnectMaterialProperty(FromExpr, FromOutputName, Prop);
    if (bConnected)
    {
        Material->MarkPackageDirty();
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("material"), Material->GetPathName());
    Result->SetBoolField(TEXT("connected"), bConnected);
    Result->SetStringField(TEXT("property"), PropertyName);
    Result->SetNumberField(TEXT("from_id"), FromId);
    Result->SetStringField(TEXT("from_output_name"), FromOutputName);
    return Result;
}

// Manual disconnect helper — UMaterialEditingLibrary doesn't expose a DisconnectMaterialProperty
// in UE 5.7, so we clear the FExpressionInput on the editor-only data ourselves.
static FExpressionInput* GetMaterialInputForProperty(UMaterialEditorOnlyData* EditorData, EMaterialProperty Prop)
{
    if (!EditorData) return nullptr;
    switch (Prop)
    {
        case MP_BaseColor:           return &EditorData->BaseColor;
        case MP_Metallic:            return &EditorData->Metallic;
        case MP_Specular:            return &EditorData->Specular;
        case MP_Roughness:           return &EditorData->Roughness;
        case MP_Anisotropy:          return &EditorData->Anisotropy;
        case MP_EmissiveColor:       return &EditorData->EmissiveColor;
        case MP_Opacity:             return &EditorData->Opacity;
        case MP_OpacityMask:         return &EditorData->OpacityMask;
        case MP_Normal:              return &EditorData->Normal;
        case MP_Tangent:             return &EditorData->Tangent;
        case MP_WorldPositionOffset: return &EditorData->WorldPositionOffset;
        case MP_SubsurfaceColor:     return &EditorData->SubsurfaceColor;
        case MP_AmbientOcclusion:    return &EditorData->AmbientOcclusion;
        case MP_Refraction:          return &EditorData->Refraction;
        case MP_PixelDepthOffset:    return &EditorData->PixelDepthOffset;
        case MP_CustomData0:         return &EditorData->ClearCoat;
        case MP_CustomData1:         return &EditorData->ClearCoatRoughness;
        default:                     return nullptr;
    }
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleDisconnectMaterialProperty(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath, PropertyName;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
    if (!Material)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
    }
    EMaterialProperty Prop = StringToMaterialProperty(PropertyName);
    if (Prop == MP_MAX)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unknown material property: %s"), *PropertyName));
    }

    UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
    FExpressionInput* Input = GetMaterialInputForProperty(EditorData, Prop);
    if (!Input)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Property '%s' is not a routable top-level material input"), *PropertyName));
    }

    const bool bWasConnected = (Input->Expression != nullptr);
    Material->Modify();
    Input->Expression = nullptr;
    Input->OutputIndex = 0;
    Material->PostEditChange();
    Material->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("material"), Material->GetPathName());
    Result->SetBoolField(TEXT("disconnected"), bWasConnected);
    Result->SetStringField(TEXT("property"), PropertyName);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleSetMaterialExpressionProperty(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath, PropertyName;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }
    int32 ExpressionId = -1;
    if (!Params->TryGetNumberField(TEXT("expression_id"), ExpressionId))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'expression_id' parameter"));
    }
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }
    // property_value is required as a string (callers serialize their value)
    FString PropertyValue;
    if (!Params->TryGetStringField(TEXT("property_value"), PropertyValue))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter (string-encoded value)"));
    }

    UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
    if (!Material)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
    }
    UMaterialExpression* Expr = FindMaterialExpressionByIndex(Material, ExpressionId);
    if (!Expr)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Expression id %d not found"), ExpressionId));
    }

    FProperty* Property = Expr->GetClass()->FindPropertyByName(*PropertyName);
    if (!Property)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Property '%s' not found on %s"), *PropertyName, *Expr->GetClass()->GetName()));
    }

    void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Expr);

    // Special-case object properties: ImportText for object refs needs Object'/Path' format,
    // but we want to accept raw paths too. Convert if needed.
    FString InputText = PropertyValue;
    if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
    {
        if (!InputText.Contains(TEXT("'")) && (InputText.StartsWith(TEXT("/Game")) || InputText.StartsWith(TEXT("/Engine"))))
        {
            // Try to load as the property's class to validate, then format as Object'path'
            UObject* LoadedObj = LoadObject<UObject>(nullptr, *InputText);
            if (LoadedObj)
            {
                InputText = FString::Printf(TEXT("%s'%s'"), *LoadedObj->GetClass()->GetName(), *LoadedObj->GetPathName());
            }
        }
    }

    Expr->Modify();
    const TCHAR* ImportResult = Property->ImportText_Direct(*InputText, ValuePtr, Expr, PPF_None);
    if (!ImportResult)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to import value '%s' into property '%s'"), *PropertyValue, *PropertyName));
    }

    // Special handling: if a MaterialFunctionCall's MaterialFunction was changed, refresh inputs/outputs
    if (UMaterialExpressionMaterialFunctionCall* FuncCall = Cast<UMaterialExpressionMaterialFunctionCall>(Expr))
    {
        if (PropertyName == TEXT("MaterialFunction") && FuncCall->MaterialFunction)
        {
            FuncCall->UpdateFromFunctionResource();
        }
    }

    // Trigger PostEditChangeProperty so the material editor refreshes
    FPropertyChangedEvent ChangeEvent(Property, EPropertyChangeType::ValueSet);
    Expr->PostEditChangeProperty(ChangeEvent);
    Material->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("material"), Material->GetPathName());
    Result->SetNumberField(TEXT("expression_id"), ExpressionId);
    Result->SetStringField(TEXT("property"), PropertyName);
    Result->SetStringField(TEXT("value"), PropertyValue);
    Result->SetBoolField(TEXT("success"), true);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleAddCustomNodeInput(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath, InputName;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }
    int32 ExpressionId = -1;
    if (!Params->TryGetNumberField(TEXT("expression_id"), ExpressionId))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'expression_id' parameter"));
    }
    if (!Params->TryGetStringField(TEXT("input_name"), InputName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'input_name' parameter"));
    }

    UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
    if (!Material)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
    }

    UMaterialExpression* Expr = FindMaterialExpressionByIndex(Material, ExpressionId);
    UMaterialExpressionCustom* Custom = Cast<UMaterialExpressionCustom>(Expr);
    if (!Custom)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Expression id %d is not a Custom node"), ExpressionId));
    }

    // Don't duplicate an existing input name.
    for (const FCustomInput& In : Custom->Inputs)
    {
        if (In.InputName == FName(*InputName))
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Input '%s' already exists on the Custom node"), *InputName));
        }
    }

    // Append preserves existing inputs and their connections (the FExpressionInput in each entry).
    Custom->Modify();
    FCustomInput NewInput;
    NewInput.InputName = FName(*InputName);
    Custom->Inputs.Add(NewInput);
    Custom->PostEditChange();
    Material->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("material"), Material->GetPathName());
    Result->SetNumberField(TEXT("expression_id"), ExpressionId);
    Result->SetStringField(TEXT("input_name"), InputName);
    Result->SetNumberField(TEXT("input_index"), Custom->Inputs.Num() - 1);
    Result->SetBoolField(TEXT("success"), true);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleCompileMaterial(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
    if (!Material)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
    }

    UMaterialEditingLibrary::RecompileMaterial(Material);
    Material->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("material"), Material->GetPathName());
    Result->SetBoolField(TEXT("recompiled"), true);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleGetSkeletonBones(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    int32 MaxBones = 500;
    Params->TryGetNumberField(TEXT("max_bones"), MaxBones);

    USkeleton* Skeleton = nullptr;
    UObject* Asset = LoadObject<UObject>(nullptr, *AssetPath);
    if (!Asset)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
    }

    if (USkeleton* Skel = Cast<USkeleton>(Asset))
    {
        Skeleton = Skel;
    }
    else if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(Asset))
    {
        Skeleton = Mesh->GetSkeleton();
    }
    else if (UAnimSequenceBase* Anim = Cast<UAnimSequenceBase>(Asset))
    {
        Skeleton = Anim->GetSkeleton();
    }

    if (!Skeleton)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("No skeleton found on asset: %s (class: %s)"), *AssetPath, *Asset->GetClass()->GetName()));
    }

    const FReferenceSkeleton& RefSkel = Skeleton->GetReferenceSkeleton();
    int32 NumBones = RefSkel.GetNum();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("skeleton_name"), Skeleton->GetName());
    Result->SetStringField(TEXT("skeleton_path"), Skeleton->GetPathName());
    Result->SetNumberField(TEXT("bone_count"), NumBones);

    TArray<TSharedPtr<FJsonValue>> BonesArray;
    int32 Limit = FMath::Min(NumBones, MaxBones);
    for (int32 i = 0; i < Limit; i++)
    {
        TSharedPtr<FJsonObject> BoneObj = MakeShared<FJsonObject>();
        BoneObj->SetNumberField(TEXT("index"), i);
        BoneObj->SetStringField(TEXT("name"), RefSkel.GetBoneName(i).ToString());
        int32 ParentIdx = RefSkel.GetParentIndex(i);
        if (ParentIdx >= 0)
        {
            BoneObj->SetStringField(TEXT("parent"), RefSkel.GetBoneName(ParentIdx).ToString());
            BoneObj->SetNumberField(TEXT("parent_index"), ParentIdx);
        }
        const FTransform& BonePose = RefSkel.GetRefBonePose()[i];
        TSharedPtr<FJsonObject> PoseObj = MakeShared<FJsonObject>();
        PoseObj->SetNumberField(TEXT("x"), BonePose.GetLocation().X);
        PoseObj->SetNumberField(TEXT("y"), BonePose.GetLocation().Y);
        PoseObj->SetNumberField(TEXT("z"), BonePose.GetLocation().Z);
        BoneObj->SetObjectField(TEXT("ref_pose"), PoseObj);

        BonesArray.Add(MakeShared<FJsonValueObject>(BoneObj));
    }
    Result->SetArrayField(TEXT("bones"), BonesArray);

    return Result;
}

// --- Helper: serialize decorators from an array ---
static TArray<TSharedPtr<FJsonValue>> SerializeBTDecorators(const TArray<UBTDecorator*>& Decs)
{
    TArray<TSharedPtr<FJsonValue>> Arr;
    for (int32 d = 0; d < Decs.Num(); d++)
    {
        if (!Decs[d]) continue;
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("name"), Decs[d]->GetNodeName());
        Obj->SetStringField(TEXT("class"), Decs[d]->GetClass()->GetName());
        Arr.Add(MakeShared<FJsonValueObject>(Obj));
    }
    return Arr;
}

// --- Helper: recursively serialize a BT composite node and its children ---
static TSharedPtr<FJsonObject> SerializeBTNode(UBTCompositeNode* Composite, int32 Depth = 0)
{
    if (!Composite || Depth > 50) return nullptr;

    TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
    NodeObj->SetStringField(TEXT("name"), Composite->GetNodeName());
    NodeObj->SetStringField(TEXT("class"), Composite->GetClass()->GetName());
    NodeObj->SetNumberField(TEXT("index"), Composite->GetExecutionIndex());

    // Children
    TArray<TSharedPtr<FJsonValue>> ChildrenArray;
    for (int32 i = 0; i < Composite->GetChildrenNum(); i++)
    {
        const FBTCompositeChild& Child = Composite->Children[i];
        TSharedPtr<FJsonObject> ChildObj;

        if (Child.ChildComposite)
        {
            ChildObj = SerializeBTNode(Child.ChildComposite, Depth + 1);
        }
        else if (Child.ChildTask)
        {
            ChildObj = MakeShared<FJsonObject>();
            ChildObj->SetStringField(TEXT("name"), Child.ChildTask->GetNodeName());
            ChildObj->SetStringField(TEXT("class"), Child.ChildTask->GetClass()->GetName());
            ChildObj->SetNumberField(TEXT("index"), Child.ChildTask->GetExecutionIndex());
        }

        if (ChildObj.IsValid())
        {
            TArray<TSharedPtr<FJsonValue>> Decs = SerializeBTDecorators(Child.Decorators);
            if (Decs.Num() > 0)
                ChildObj->SetArrayField(TEXT("decorators"), Decs);
            ChildrenArray.Add(MakeShared<FJsonValueObject>(ChildObj));
        }
    }
    if (ChildrenArray.Num() > 0)
        NodeObj->SetArrayField(TEXT("children"), ChildrenArray);

    return NodeObj;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleGetBehaviorTreeInfo(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    UBehaviorTree* BT = LoadObject<UBehaviorTree>(nullptr, *AssetPath);
    if (!BT)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("BehaviorTree not found: %s"), *AssetPath));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("name"), BT->GetName());
    Result->SetStringField(TEXT("path"), BT->GetPathName());

    // Blackboard asset
    UBlackboardData* BB = BT->BlackboardAsset;
    if (BB)
    {
        Result->SetStringField(TEXT("blackboard"), BB->GetPathName());

        TArray<TSharedPtr<FJsonValue>> KeysArray;
        for (int32 ki = 0; ki < BB->Keys.Num(); ki++)
        {
            const FBlackboardEntry& Entry = BB->Keys[ki];
            TSharedPtr<FJsonObject> KeyObj = MakeShared<FJsonObject>();
            KeyObj->SetStringField(TEXT("name"), Entry.EntryName.ToString());
            if (Entry.KeyType)
                KeyObj->SetStringField(TEXT("type"), Entry.KeyType->GetClass()->GetName());
            KeysArray.Add(MakeShared<FJsonValueObject>(KeyObj));
        }
        for (int32 ki = 0; ki < BB->ParentKeys.Num(); ki++)
        {
            const FBlackboardEntry& Entry = BB->ParentKeys[ki];
            TSharedPtr<FJsonObject> KeyObj = MakeShared<FJsonObject>();
            KeyObj->SetStringField(TEXT("name"), Entry.EntryName.ToString());
            if (Entry.KeyType)
                KeyObj->SetStringField(TEXT("type"), Entry.KeyType->GetClass()->GetName());
            KeyObj->SetBoolField(TEXT("inherited"), true);
            KeysArray.Add(MakeShared<FJsonValueObject>(KeyObj));
        }
        Result->SetArrayField(TEXT("blackboard_keys"), KeysArray);
    }

    // Tree structure
    if (BT->RootNode)
    {
        TSharedPtr<FJsonObject> TreeObj = SerializeBTNode(BT->RootNode);
        if (TreeObj.IsValid())
        {
            Result->SetObjectField(TEXT("tree"), TreeObj);
        }
    }

    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPInspectionCommands::HandleGetMetaSoundGraph(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path'"));

    UObject* Asset = LoadObject<UObject>(nullptr, *AssetPath);
    if (!Asset)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Asset not found: %s"), *AssetPath));

    UMetaSoundSource* MSSource = Cast<UMetaSoundSource>(Asset);
    if (!MSSource)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Asset is not a MetaSoundSource (got %s)"), *Asset->GetClass()->GetName()));

    const FMetasoundFrontendDocument& Doc = MSSource->GetConstDocument();
    const FMetasoundFrontendGraphClass& RootGraph = Doc.RootGraph;

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("name"), MSSource->GetName());
    Result->SetStringField(TEXT("path"), MSSource->GetPathName());

    // Interfaces (inputs/outputs of the root graph)
    const FMetasoundFrontendClassInterface& Interface = RootGraph.Interface;
    TArray<TSharedPtr<FJsonValue>> InputsArr;
    for (const FMetasoundFrontendClassInput& Input : Interface.Inputs)
    {
        TSharedPtr<FJsonObject> IObj = MakeShared<FJsonObject>();
        IObj->SetStringField(TEXT("name"), Input.Name.ToString());
        IObj->SetStringField(TEXT("type"), Input.TypeName.ToString());
        IObj->SetStringField(TEXT("id"), Input.VertexID.ToString());
        InputsArr.Add(MakeShared<FJsonValueObject>(IObj));
    }
    Result->SetArrayField(TEXT("inputs"), InputsArr);

    TArray<TSharedPtr<FJsonValue>> OutputsArr;
    for (const FMetasoundFrontendClassOutput& Output : Interface.Outputs)
    {
        TSharedPtr<FJsonObject> OObj = MakeShared<FJsonObject>();
        OObj->SetStringField(TEXT("name"), Output.Name.ToString());
        OObj->SetStringField(TEXT("type"), Output.TypeName.ToString());
        OObj->SetStringField(TEXT("id"), Output.VertexID.ToString());
        OutputsArr.Add(MakeShared<FJsonValueObject>(OObj));
    }
    Result->SetArrayField(TEXT("outputs"), OutputsArr);

    // Nodes
    const FMetasoundFrontendGraph& Graph = RootGraph.Graph;
    TArray<TSharedPtr<FJsonValue>> NodesArr;
    for (const FMetasoundFrontendNode& Node : Graph.Nodes)
    {
        TSharedPtr<FJsonObject> NObj = MakeShared<FJsonObject>();
        NObj->SetStringField(TEXT("id"), Node.GetID().ToString());
        NObj->SetStringField(TEXT("class_id"), Node.ClassID.ToString());

        if (!Node.Name.IsNone())
            NObj->SetStringField(TEXT("name"), Node.Name.ToString());

        // Node inputs
        TArray<TSharedPtr<FJsonValue>> NodeInputsArr;
        for (const FMetasoundFrontendVertex& V : Node.Interface.Inputs)
        {
            TSharedPtr<FJsonObject> VObj = MakeShared<FJsonObject>();
            VObj->SetStringField(TEXT("name"), V.Name.ToString());
            VObj->SetStringField(TEXT("type"), V.TypeName.ToString());
            NodeInputsArr.Add(MakeShared<FJsonValueObject>(VObj));
        }
        NObj->SetArrayField(TEXT("inputs"), NodeInputsArr);

        // Node outputs
        TArray<TSharedPtr<FJsonValue>> NodeOutputsArr;
        for (const FMetasoundFrontendVertex& V : Node.Interface.Outputs)
        {
            TSharedPtr<FJsonObject> VObj = MakeShared<FJsonObject>();
            VObj->SetStringField(TEXT("name"), V.Name.ToString());
            VObj->SetStringField(TEXT("type"), V.TypeName.ToString());
            NodeOutputsArr.Add(MakeShared<FJsonValueObject>(VObj));
        }
        NObj->SetArrayField(TEXT("outputs"), NodeOutputsArr);

        NodesArr.Add(MakeShared<FJsonValueObject>(NObj));
    }
    Result->SetArrayField(TEXT("nodes"), NodesArr);
    Result->SetNumberField(TEXT("node_count"), Graph.Nodes.Num());

    // Edges (connections)
    TArray<TSharedPtr<FJsonValue>> EdgesArr;
    for (const FMetasoundFrontendEdge& Edge : Graph.Edges)
    {
        TSharedPtr<FJsonObject> EObj = MakeShared<FJsonObject>();
        EObj->SetStringField(TEXT("from_node"), Edge.FromNodeID.ToString());
        EObj->SetStringField(TEXT("from_vertex"), Edge.FromVertexID.ToString());
        EObj->SetStringField(TEXT("to_node"), Edge.ToNodeID.ToString());
        EObj->SetStringField(TEXT("to_vertex"), Edge.ToVertexID.ToString());
        EdgesArr.Add(MakeShared<FJsonValueObject>(EObj));
    }
    Result->SetArrayField(TEXT("edges"), EdgesArr);
    Result->SetNumberField(TEXT("edge_count"), Graph.Edges.Num());

    return Result;
}
