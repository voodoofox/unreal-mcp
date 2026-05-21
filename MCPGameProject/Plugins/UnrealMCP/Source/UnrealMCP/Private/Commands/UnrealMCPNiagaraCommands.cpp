#include "Commands/UnrealMCPNiagaraCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"

#pragma warning(push)
#pragma warning(disable: 4267)  // 'size_t' to smaller type conversion
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraLightRendererProperties.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraEditorModule.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraScriptSource.h"
#include "NiagaraNodeAssignment.h"
#include "NiagaraNodeInput.h"
#include "NiagaraSimulationStageBase.h"
#include "NiagaraScript.h"
#include "NiagaraDataInterface.h"
#include "NiagaraDataInterfaceCurve.h"
#include "NiagaraDataInterfaceVectorCurve.h"
#include "NiagaraDataInterfaceColorCurve.h"
#include "NiagaraDataInterfaceVector2DCurve.h"
#include "NiagaraDataInterfaceVector4Curve.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "NiagaraTypes.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraScriptFactoryNew.h"
#include "NiagaraNodeCustomHlsl.h"
#include "NiagaraEffectType.h"
#include "NiagaraDataChannelAsset.h"
#include "NiagaraDataChannel.h"
#include "NiagaraScratchPadContainer.h"
#pragma warning(pop)

#include "AssetToolsModule.h"
#include "EditorAssetLibrary.h"
#include "Factories/Factory.h"
#include "UObject/SavePackage.h"

// ─── Helpers ──────────────────────────────────────────────────────────────────

static UNiagaraSystem* LoadNiagaraSystem(const FString& SystemPath)
{
    // Try direct load
    UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, *SystemPath);
    if (System) return System;

    // Try with /Game/ prefix
    if (SystemPath.Len() > 0 && SystemPath[0] != TEXT('/'))
    {
        System = LoadObject<UNiagaraSystem>(nullptr, *(TEXT("/Game/") + SystemPath));
        if (System) return System;
    }

    // Try asset registry search by name
    FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> Assets;
    ARM.Get().GetAssetsByClass(UNiagaraSystem::StaticClass()->GetClassPathName(), Assets);
    for (const FAssetData& Asset : Assets)
    {
        if (Asset.AssetName.ToString() == SystemPath ||
            Asset.GetObjectPathString().Find(SystemPath) != INDEX_NONE)
        {
            return Cast<UNiagaraSystem>(Asset.GetAsset());
        }
    }
    return nullptr;
}

static FString RendererTypeToString(UNiagaraRendererProperties* Renderer)
{
    if (!Renderer) return TEXT("null");
    if (Renderer->IsA<UNiagaraRibbonRendererProperties>()) return TEXT("Ribbon");
    if (Renderer->IsA<UNiagaraSpriteRendererProperties>()) return TEXT("Sprite");
    if (Renderer->IsA<UNiagaraMeshRendererProperties>()) return TEXT("Mesh");
    if (Renderer->IsA<UNiagaraLightRendererProperties>()) return TEXT("Light");
    return Renderer->GetClass()->GetName();
}

// ─── Command Router ───────────────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleCommand(
    const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // System-level
    if (CommandType == TEXT("create_niagara_system"))
        return HandleCreateNiagaraSystem(Params);
    if (CommandType == TEXT("get_niagara_overview"))
        return HandleGetNiagaraOverview(Params);
    if (CommandType == TEXT("set_niagara_system_property"))
        return HandleSetNiagaraSystemProperty(Params);
    // Emitter management
    if (CommandType == TEXT("set_niagara_emitter_enabled"))
        return HandleSetNiagaraEmitterEnabled(Params);
    if (CommandType == TEXT("remove_niagara_emitter"))
        return HandleRemoveNiagaraEmitter(Params);
    if (CommandType == TEXT("duplicate_niagara_emitter"))
        return HandleDuplicateNiagaraEmitter(Params);
    // Renderer
    if (CommandType == TEXT("add_niagara_renderer"))
        return HandleAddNiagaraRenderer(Params);
    if (CommandType == TEXT("remove_niagara_renderer"))
        return HandleRemoveNiagaraRenderer(Params);
    if (CommandType == TEXT("set_niagara_renderer_property"))
        return HandleSetNiagaraRendererProperty(Params);
    if (CommandType == TEXT("get_niagara_renderer_properties"))
        return HandleGetNiagaraRendererProperties(Params);
    // Emitter properties
    if (CommandType == TEXT("set_niagara_emitter_property"))
        return HandleSetNiagaraEmitterProperty(Params);
    // Module management
    if (CommandType == TEXT("list_niagara_modules"))
        return HandleListNiagaraModules(Params);
    if (CommandType == TEXT("add_niagara_module"))
        return HandleAddNiagaraModule(Params);
    if (CommandType == TEXT("remove_niagara_module"))
        return HandleRemoveNiagaraModule(Params);
    if (CommandType == TEXT("set_niagara_module_enabled"))
        return HandleSetNiagaraModuleEnabled(Params);
    // Module inputs
    if (CommandType == TEXT("get_niagara_module_inputs"))
        return HandleGetNiagaraModuleInputs(Params);
    if (CommandType == TEXT("set_niagara_module_input"))
        return HandleSetNiagaraModuleInput(Params);
    // Module pins (static switches + direct pin values)
    if (CommandType == TEXT("get_niagara_module_pins"))
        return HandleGetNiagaraModulePins(Params);
    if (CommandType == TEXT("set_niagara_module_pin_value"))
        return HandleSetNiagaraModulePinValue(Params);
    // User parameters
    if (CommandType == TEXT("get_niagara_user_parameters"))
        return HandleGetNiagaraUserParameters(Params);
    if (CommandType == TEXT("set_niagara_user_parameter"))
        return HandleSetNiagaraUserParameter(Params);
    // Module search
    if (CommandType == TEXT("search_niagara_modules"))
        return HandleSearchNiagaraModules(Params);
    // Parameter assignment
    if (CommandType == TEXT("add_niagara_set_parameter"))
        return HandleAddNiagaraSetParameter(Params);
    // System utilities
    if (CommandType == TEXT("compile_niagara_system"))
        return HandleCompileNiagaraSystem(Params);
    if (CommandType == TEXT("set_niagara_fixed_bounds"))
        return HandleSetNiagaraFixedBounds(Params);
    if (CommandType == TEXT("rename_niagara_emitter"))
        return HandleRenameNiagaraEmitter(Params);
    if (CommandType == TEXT("add_niagara_user_parameter"))
        return HandleAddNiagaraUserParameter(Params);
    if (CommandType == TEXT("get_niagara_emitter_properties"))
        return HandleGetNiagaraEmitterProperties(Params);
    // Renderer bindings
    if (CommandType == TEXT("get_niagara_renderer_bindings"))
        return HandleGetNiagaraRendererBindings(Params);
    if (CommandType == TEXT("set_niagara_renderer_binding"))
        return HandleSetNiagaraRendererBinding(Params);
    // Generic override
    if (CommandType == TEXT("set_niagara_module_override"))
        return HandleSetNiagaraModuleOverride(Params);
    // Ribbon custom vertices
    if (CommandType == TEXT("set_niagara_custom_vertices"))
        return HandleSetNiagaraCustomVertices(Params);
    if (CommandType == TEXT("get_niagara_custom_vertices"))
        return HandleGetNiagaraCustomVertices(Params);
    if (CommandType == TEXT("get_niagara_deep_inspect"))
        return HandleGetNiagaraDeepInspect(Params);
    if (CommandType == TEXT("get_niagara_curve_data"))
        return HandleGetNiagaraCurveData(Params);
    if (CommandType == TEXT("set_niagara_curve_data"))
        return HandleSetNiagaraCurveData(Params);
    if (CommandType == TEXT("get_niagara_di_properties"))
        return HandleGetNiagaraDIProperties(Params);
    if (CommandType == TEXT("set_niagara_di_property"))
        return HandleSetNiagaraDIProperty(Params);
    if (CommandType == TEXT("add_niagara_simulation_stage"))
        return HandleAddNiagaraSimulationStage(Params);
    if (CommandType == TEXT("remove_niagara_simulation_stage"))
        return HandleRemoveNiagaraSimulationStage(Params);
    if (CommandType == TEXT("get_niagara_sim_stage_properties"))
        return HandleGetNiagaraSimStageProperties(Params);
    if (CommandType == TEXT("set_niagara_sim_stage_property"))
        return HandleSetNiagaraSimStageProperty(Params);
    if (CommandType == TEXT("add_niagara_event_handler"))
        return HandleAddNiagaraEventHandler(Params);
    if (CommandType == TEXT("set_niagara_emitter_sim_target"))
        return HandleSetNiagaraEmitterSimTarget(Params);
    if (CommandType == TEXT("remove_niagara_event_handler"))
        return HandleRemoveNiagaraEventHandler(Params);
    if (CommandType == TEXT("add_niagara_emitter"))
        return HandleAddNiagaraEmitter(Params);
    if (CommandType == TEXT("set_niagara_event_handler_property"))
        return HandleSetNiagaraEventHandlerProperty(Params);
    if (CommandType == TEXT("move_niagara_simulation_stage"))
        return HandleMoveNiagaraSimulationStage(Params);
    if (CommandType == TEXT("get_niagara_scalability"))
        return HandleGetNiagaraScalabilitySettings(Params);
    if (CommandType == TEXT("set_niagara_scalability_override"))
        return HandleSetNiagaraScalabilityOverride(Params);
    if (CommandType == TEXT("get_niagara_data_channel_info"))
        return HandleGetNiagaraDataChannelInfo(Params);
    if (CommandType == TEXT("get_niagara_scratch_pad_scripts"))
        return HandleGetNiagaraScratchPadScripts(Params);
    if (CommandType == TEXT("create_niagara_hlsl_module"))
        return HandleCreateNiagaraHlslModule(Params);
    if (CommandType == TEXT("export_niagara_system_spec"))
        return HandleExportNiagaraSystemSpec(Params);

    return FUnrealMCPCommonUtils::CreateErrorResponse(
        FString::Printf(TEXT("Unknown Niagara command: %s"), *CommandType));
}

// ─── get_niagara_overview ─────────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetNiagaraOverview(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Niagara system not found: %s"), *SystemPath));

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("system_path"), System->GetPathName());
    Result->SetNumberField(TEXT("warmup_time"), System->GetWarmupTime());

    TArray<TSharedPtr<FJsonValue>> EmitterArray;

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    for (int32 i = 0; i < Handles.Num(); i++)
    {
        const FNiagaraEmitterHandle& Handle = Handles[i];
        TSharedPtr<FJsonObject> EmitterObj = MakeShared<FJsonObject>();
        EmitterObj->SetStringField(TEXT("name"), Handle.GetName().ToString());
        EmitterObj->SetNumberField(TEXT("index"), i);
        EmitterObj->SetBoolField(TEXT("enabled"), Handle.GetIsEnabled());

        // Get versioned emitter data for renderers
        if (FVersionedNiagaraEmitterData* EmitterData = Handle.GetInstance().GetEmitterData())
        {
            TArray<TSharedPtr<FJsonValue>> RendererArray;
            const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();

            for (int32 ri = 0; ri < Renderers.Num(); ri++)
            {
                UNiagaraRendererProperties* Renderer = Renderers[ri];
                TSharedPtr<FJsonObject> RendObj = MakeShared<FJsonObject>();
                RendObj->SetNumberField(TEXT("index"), ri);
                RendObj->SetStringField(TEXT("type"), RendererTypeToString(Renderer));
                RendObj->SetBoolField(TEXT("enabled"), Renderer->GetIsEnabled());

                // Type-specific properties
                if (UNiagaraRibbonRendererProperties* Ribbon = Cast<UNiagaraRibbonRendererProperties>(Renderer))
                {
                    RendObj->SetNumberField(TEXT("multi_plane_count"), Ribbon->MultiPlaneCount);
                    RendObj->SetNumberField(TEXT("width_segmentation_count"), Ribbon->WidthSegmentationCount);
                    if (Ribbon->Material)
                        RendObj->SetStringField(TEXT("material"), Ribbon->Material->GetPathName());
                }
                else if (UNiagaraSpriteRendererProperties* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer))
                {
                    if (Sprite->Material)
                        RendObj->SetStringField(TEXT("material"), Sprite->Material->GetPathName());
                }

                RendererArray.Add(MakeShared<FJsonValueObject>(RendObj));
            }
            EmitterObj->SetArrayField(TEXT("renderers"), RendererArray);

            // Simulation target
            EmitterObj->SetStringField(TEXT("sim_target"),
                EmitterData->SimTarget == ENiagaraSimTarget::CPUSim ? TEXT("CPU") : TEXT("GPU"));
        }

        EmitterArray.Add(MakeShared<FJsonValueObject>(EmitterObj));
    }

    Result->SetArrayField(TEXT("emitters"), EmitterArray);
    return Result;
}

// ─── set_niagara_renderer_property ────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraRendererProperty(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    int32 RendererIndex = 0;
    Params->TryGetNumberField(TEXT("renderer_index"), RendererIndex);

    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name'"));

    FString PropertyValue;
    if (!Params->TryGetStringField(TEXT("property_value"), PropertyValue))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* Data = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    const TArray<UNiagaraRendererProperties*>& Renderers = Data->GetRenderers();
    if (!Renderers.IsValidIndex(RendererIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid renderer_index"));

    UNiagaraRendererProperties* Renderer = Renderers[RendererIndex];

    // Use UE reflection to set the property
    FProperty* Prop = Renderer->GetClass()->FindPropertyByName(FName(*PropertyName));
    if (!Prop)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Property '%s' not found on renderer"), *PropertyName));

    Renderer->Modify();
    bool bSuccess = false;

    if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
    {
        IntProp->SetPropertyValue_InContainer(Renderer, FCString::Atoi(*PropertyValue));
        bSuccess = true;
    }
    else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
    {
        FloatProp->SetPropertyValue_InContainer(Renderer, FCString::Atof(*PropertyValue));
        bSuccess = true;
    }
    else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop))
    {
        DoubleProp->SetPropertyValue_InContainer(Renderer, FCString::Atod(*PropertyValue));
        bSuccess = true;
    }
    else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
    {
        BoolProp->SetPropertyValue_InContainer(Renderer, PropertyValue.ToBool());
        bSuccess = true;
    }
    else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
    {
        // Accept integer value or enum name string
        FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
        int64 EnumValue = FCString::Atoi64(*PropertyValue);
        // Try name lookup first
        if (UEnum* Enum = EnumProp->GetEnum())
        {
            int64 NamedVal = Enum->GetValueByNameString(PropertyValue);
            if (NamedVal != INDEX_NONE) EnumValue = NamedVal;
        }
        UnderlyingProp->SetIntPropertyValue(EnumProp->ContainerPtrToValuePtr<void>(Renderer), EnumValue);
        bSuccess = true;
    }
    else if (FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
    {
        if (ByteProp->Enum)
        {
            int64 EnumValue = FCString::Atoi64(*PropertyValue);
            int64 NamedVal = ByteProp->Enum->GetValueByNameString(PropertyValue);
            if (NamedVal != INDEX_NONE) EnumValue = NamedVal;
            ByteProp->SetPropertyValue_InContainer(Renderer, (uint8)EnumValue);
        }
        else
        {
            ByteProp->SetPropertyValue_InContainer(Renderer, (uint8)FCString::Atoi(*PropertyValue));
        }
        bSuccess = true;
    }
    else if (FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(Prop))
    {
        UObject* Obj = LoadObject<UObject>(nullptr, *PropertyValue);
        if (Obj)
        {
            ObjProp->SetObjectPropertyValue_InContainer(Renderer, Obj);
            bSuccess = true;
        }
        else
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Could not load object: %s"), *PropertyValue));
        }
    }

    if (!bSuccess)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unsupported property type for '%s'"), *PropertyName));

    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("property_name"), PropertyName);
    Result->SetStringField(TEXT("property_value"), PropertyValue);
    return Result;
}

// ─── set_niagara_custom_vertices ─────────────────────────────────────────────
// Sets CustomVertices on a Ribbon renderer from a JSON array of {x, y, nx, ny, tv}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraCustomVertices(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    int32 RendererIndex = 0;
    Params->TryGetNumberField(TEXT("renderer_index"), RendererIndex);

    const TArray<TSharedPtr<FJsonValue>>* VerticesArray = nullptr;
    if (!Params->TryGetArrayField(TEXT("vertices"), VerticesArray) || !VerticesArray)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'vertices' array"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* Data = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    const TArray<UNiagaraRendererProperties*>& Renderers = Data->GetRenderers();
    if (!Renderers.IsValidIndex(RendererIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid renderer_index"));

    UNiagaraRibbonRendererProperties* RibbonRenderer = Cast<UNiagaraRibbonRendererProperties>(Renderers[RendererIndex]);
    if (!RibbonRenderer)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Renderer is not a Ribbon renderer"));

    RibbonRenderer->Modify();

    // Parse vertices from JSON array
    TArray<FNiagaraRibbonShapeCustomVertex> NewVertices;
    for (const TSharedPtr<FJsonValue>& VertVal : *VerticesArray)
    {
        const TSharedPtr<FJsonObject>* VertObj = nullptr;
        if (!VertVal->TryGetObject(VertObj) || !VertObj->IsValid())
            continue;

        double X = 0, Y = 0, NX = 0, NY = 0, TV = 0;
        (*VertObj)->TryGetNumberField(TEXT("x"), X);
        (*VertObj)->TryGetNumberField(TEXT("y"), Y);
        (*VertObj)->TryGetNumberField(TEXT("nx"), NX);
        (*VertObj)->TryGetNumberField(TEXT("ny"), NY);
        (*VertObj)->TryGetNumberField(TEXT("tv"), TV);
        int32 Idx = NewVertices.AddUninitialized(1);
        FMemory::Memzero(&NewVertices[Idx], sizeof(FNiagaraRibbonShapeCustomVertex));
        NewVertices[Idx].Position = FVector2f((float)X, (float)Y);
        NewVertices[Idx].Normal = FVector2f((float)NX, (float)NY);
        NewVertices[Idx].TextureV = (float)TV;
    }

    RibbonRenderer->CustomVertices = NewVertices;
    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetNumberField(TEXT("vertex_count"), NewVertices.Num());
    return Result;
}

// ─── get_niagara_custom_vertices ─────────────────────────────────────────────
// Returns CustomVertices on a Ribbon renderer as a JSON array of {x, y, nx, ny, tv}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetNiagaraCustomVertices(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    int32 RendererIndex = 0;
    Params->TryGetNumberField(TEXT("renderer_index"), RendererIndex);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* Data = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    const TArray<UNiagaraRendererProperties*>& Renderers = Data->GetRenderers();
    if (!Renderers.IsValidIndex(RendererIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid renderer_index"));

    UNiagaraRibbonRendererProperties* RibbonRenderer = Cast<UNiagaraRibbonRendererProperties>(Renderers[RendererIndex]);
    if (!RibbonRenderer)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Renderer is not a Ribbon renderer"));

    TArray<TSharedPtr<FJsonValue>> VerticesJson;
    for (const FNiagaraRibbonShapeCustomVertex& V : RibbonRenderer->CustomVertices)
    {
        TSharedPtr<FJsonObject> VObj = MakeShared<FJsonObject>();
        VObj->SetNumberField(TEXT("x"), (double)V.Position.X);
        VObj->SetNumberField(TEXT("y"), (double)V.Position.Y);
        VObj->SetNumberField(TEXT("nx"), (double)V.Normal.X);
        VObj->SetNumberField(TEXT("ny"), (double)V.Normal.Y);
        VObj->SetNumberField(TEXT("tv"), (double)V.TextureV);
        VerticesJson.Add(MakeShared<FJsonValueObject>(VObj));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetNumberField(TEXT("vertex_count"), VerticesJson.Num());
    Result->SetArrayField(TEXT("vertices"), VerticesJson);
    return Result;
}

// ─── add_niagara_renderer ─────────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddNiagaraRenderer(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString RendererType;
    if (!Params->TryGetStringField(TEXT("renderer_type"), RendererType))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'renderer_type' (Ribbon, Sprite)"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitter VerEmitter = Handles[EmitterIndex].GetInstance();
    UNiagaraEmitter* Emitter = VerEmitter.Emitter;
    FVersionedNiagaraEmitterData* Data = VerEmitter.GetEmitterData();
    if (!Emitter || !Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    Emitter->Modify();

    UNiagaraRendererProperties* NewRenderer = nullptr;
    if (RendererType.Equals(TEXT("Ribbon"), ESearchCase::IgnoreCase))
    {
        UNiagaraRibbonRendererProperties* Ribbon = NewObject<UNiagaraRibbonRendererProperties>(Emitter);
        Ribbon->MultiPlaneCount = 2;  // default flat ribbon
        NewRenderer = Ribbon;
    }
    else if (RendererType.Equals(TEXT("Sprite"), ESearchCase::IgnoreCase))
    {
        NewRenderer = NewObject<UNiagaraSpriteRendererProperties>(Emitter);
    }
    else if (RendererType.Equals(TEXT("Mesh"), ESearchCase::IgnoreCase))
    {
        NewRenderer = NewObject<UNiagaraMeshRendererProperties>(Emitter);
    }
    else if (RendererType.Equals(TEXT("Light"), ESearchCase::IgnoreCase))
    {
        NewRenderer = NewObject<UNiagaraLightRendererProperties>(Emitter);
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unsupported renderer type: %s (use Ribbon, Sprite, Mesh, or Light)"), *RendererType));
    }

    if (NewRenderer)
    {
        Emitter->AddRenderer(NewRenderer, VerEmitter.Version);
        System->MarkPackageDirty();
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("renderer_type"), RendererType);
    Result->SetNumberField(TEXT("renderer_index"), Data->GetRenderers().Num() - 1);
    return Result;
}

// ─── remove_niagara_renderer ──────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleRemoveNiagaraRenderer(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    int32 RendererIndex = 0;
    Params->TryGetNumberField(TEXT("renderer_index"), RendererIndex);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitter VerEmitter = Handles[EmitterIndex].GetInstance();
    UNiagaraEmitter* Emitter = VerEmitter.Emitter;
    FVersionedNiagaraEmitterData* Data = VerEmitter.GetEmitterData();
    if (!Emitter || !Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    const TArray<UNiagaraRendererProperties*>& Renderers = Data->GetRenderers();
    if (!Renderers.IsValidIndex(RendererIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid renderer_index"));

    Emitter->Modify();
    UNiagaraRendererProperties* ToRemove = Renderers[RendererIndex];
    Emitter->RemoveRenderer(ToRemove, VerEmitter.Version);
    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetNumberField(TEXT("removed_index"), RendererIndex);
    return Result;
}

// ─── set_niagara_system_property ──────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraSystemProperty(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name'"));

    FString PropertyValue;
    if (!Params->TryGetStringField(TEXT("property_value"), PropertyValue))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    FProperty* Prop = System->GetClass()->FindPropertyByName(FName(*PropertyName));
    if (!Prop)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Property '%s' not found on NiagaraSystem"), *PropertyName));

    System->Modify();
    bool bSuccess = false;

    if (FFloatProperty* FP = CastField<FFloatProperty>(Prop))
    { FP->SetPropertyValue_InContainer(System, FCString::Atof(*PropertyValue)); bSuccess = true; }
    else if (FDoubleProperty* DP = CastField<FDoubleProperty>(Prop))
    { DP->SetPropertyValue_InContainer(System, FCString::Atod(*PropertyValue)); bSuccess = true; }
    else if (FIntProperty* IP = CastField<FIntProperty>(Prop))
    { IP->SetPropertyValue_InContainer(System, FCString::Atoi(*PropertyValue)); bSuccess = true; }
    else if (FBoolProperty* BP = CastField<FBoolProperty>(Prop))
    { BP->SetPropertyValue_InContainer(System, PropertyValue.ToBool()); bSuccess = true; }

    if (!bSuccess)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Unsupported property type"));

    System->MarkPackageDirty();
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    return Result;
}

// ─── set_niagara_emitter_enabled ──────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraEmitterEnabled(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    bool bEnabled = true;
    Params->TryGetBoolField(TEXT("enabled"), bEnabled);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    TArray<FNiagaraEmitterHandle>& Handles = const_cast<TArray<FNiagaraEmitterHandle>&>(System->GetEmitterHandles());
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    Handles[EmitterIndex].SetIsEnabled(bEnabled, *System, true);
    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("emitter"), Handles[EmitterIndex].GetName().ToString());
    Result->SetBoolField(TEXT("enabled"), bEnabled);
    return Result;
}

// ─── remove_niagara_emitter ───────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleRemoveNiagaraEmitter(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FString RemovedName = Handles[EmitterIndex].GetName().ToString();
    System->Modify();
    System->RemoveEmitterHandle(Handles[EmitterIndex]);
    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("removed_emitter"), RemovedName);
    return Result;
}

// ─── duplicate_niagara_emitter ────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleDuplicateNiagaraEmitter(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString NewName = TEXT("DuplicatedEmitter");
    Params->TryGetStringField(TEXT("new_name"), NewName);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    System->Modify();
    FNiagaraEmitterHandle NewHandle = System->DuplicateEmitterHandle(Handles[EmitterIndex], FName(*NewName));
    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("new_emitter"), NewHandle.GetName().ToString());
    Result->SetNumberField(TEXT("new_index"), System->GetEmitterHandles().Num() - 1);
    return Result;
}

// ─── get_niagara_renderer_properties (all properties via reflection) ──────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetNiagaraRendererProperties(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    int32 RendererIndex = 0;
    Params->TryGetNumberField(TEXT("renderer_index"), RendererIndex);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* Data = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    const TArray<UNiagaraRendererProperties*>& Renderers = Data->GetRenderers();
    if (!Renderers.IsValidIndex(RendererIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid renderer_index"));

    UNiagaraRendererProperties* Renderer = Renderers[RendererIndex];

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("type"), RendererTypeToString(Renderer));
    Result->SetStringField(TEXT("class"), Renderer->GetClass()->GetName());

    TArray<TSharedPtr<FJsonValue>> PropArray;
    for (TFieldIterator<FProperty> It(Renderer->GetClass()); It; ++It)
    {
        FProperty* Prop = *It;
        if (!Prop->HasAnyPropertyFlags(CPF_Edit)) continue;

        TSharedPtr<FJsonObject> PropObj = MakeShared<FJsonObject>();
        PropObj->SetStringField(TEXT("name"), Prop->GetName());
        PropObj->SetStringField(TEXT("type"), Prop->GetCPPType());
        const FString* CategoryMeta = Prop->FindMetaData(TEXT("Category"));
        PropObj->SetStringField(TEXT("category"), CategoryMeta ? *CategoryMeta : TEXT(""));

        // Read value
        FString ValueStr;
        if (FIntProperty* IP = CastField<FIntProperty>(Prop))
            ValueStr = FString::FromInt(IP->GetPropertyValue_InContainer(Renderer));
        else if (FFloatProperty* FP = CastField<FFloatProperty>(Prop))
            ValueStr = FString::SanitizeFloat(FP->GetPropertyValue_InContainer(Renderer));
        else if (FDoubleProperty* DP = CastField<FDoubleProperty>(Prop))
            ValueStr = FString::SanitizeFloat(DP->GetPropertyValue_InContainer(Renderer));
        else if (FBoolProperty* BP = CastField<FBoolProperty>(Prop))
            ValueStr = BP->GetPropertyValue_InContainer(Renderer) ? TEXT("true") : TEXT("false");
        else if (FEnumProperty* EP = CastField<FEnumProperty>(Prop))
        {
            FNumericProperty* Under = EP->GetUnderlyingProperty();
            int64 Val = Under->GetSignedIntPropertyValue(EP->ContainerPtrToValuePtr<void>(Renderer));
            if (UEnum* Enum = EP->GetEnum())
                ValueStr = Enum->GetNameStringByValue(Val);
            else
                ValueStr = FString::Printf(TEXT("%lld"), Val);
        }
        else if (FByteProperty* ByteP = CastField<FByteProperty>(Prop))
        {
            uint8 Val = ByteP->GetPropertyValue_InContainer(Renderer);
            if (ByteP->Enum)
                ValueStr = ByteP->Enum->GetNameStringByValue(Val);
            else
                ValueStr = FString::FromInt(Val);
        }
        else if (FObjectPropertyBase* OP = CastField<FObjectPropertyBase>(Prop))
        {
            UObject* Obj = OP->GetObjectPropertyValue_InContainer(Renderer);
            ValueStr = Obj ? Obj->GetPathName() : TEXT("None");
        }
        else
            ValueStr = TEXT("<complex>");

        PropObj->SetStringField(TEXT("value"), ValueStr);
        PropArray.Add(MakeShared<FJsonValueObject>(PropObj));
    }

    Result->SetArrayField(TEXT("properties"), PropArray);
    return Result;
}

// ─── create_niagara_system ────────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleCreateNiagaraSystem(
    const TSharedPtr<FJsonObject>& Params)
{
    FString AssetName;
    if (!Params->TryGetStringField(TEXT("asset_name"), AssetName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_name'"));

    FString AssetPath = TEXT("/Game/Effects");
    Params->TryGetStringField(TEXT("asset_path"), AssetPath);

    // Check if it already exists
    FString FullPath = AssetPath / AssetName;
    if (UEditorAssetLibrary::DoesAssetExist(FullPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Asset already exists: %s"), *FullPath));

    // Optionally duplicate from a template
    FString TemplatePath;
    if (Params->TryGetStringField(TEXT("template_path"), TemplatePath))
    {
        UObject* Dup = UEditorAssetLibrary::DuplicateAsset(TemplatePath, FullPath);
        if (!Dup)
            return FUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Failed to duplicate template: %s"), *TemplatePath));

        UEditorAssetLibrary::SaveAsset(FullPath);

        TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetBoolField(TEXT("success"), true);
        Result->SetStringField(TEXT("asset_path"), FullPath);
        Result->SetStringField(TEXT("source"), TEXT("template"));
        return Result;
    }

    // Create via factory (empty system with default emitters)
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    UNiagaraSystemFactoryNew* Factory = NewObject<UNiagaraSystemFactoryNew>();
    UObject* NewAsset = AssetTools.CreateAsset(AssetName, AssetPath, UNiagaraSystem::StaticClass(), Factory);

    if (!NewAsset)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Niagara system"));

    UEditorAssetLibrary::SaveAsset(FullPath);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("asset_path"), FullPath);
    Result->SetStringField(TEXT("source"), TEXT("factory"));
    return Result;
}

// ─── Helper: get script by usage name ─────────────────────────────────────────

static UNiagaraScript* GetEmitterScript(FVersionedNiagaraEmitterData* Data, const FString& ScriptType)
{
    if (ScriptType.Equals(TEXT("Spawn"), ESearchCase::IgnoreCase) ||
        ScriptType.Equals(TEXT("ParticleSpawn"), ESearchCase::IgnoreCase))
        return Data->SpawnScriptProps.Script;
    if (ScriptType.Equals(TEXT("Update"), ESearchCase::IgnoreCase) ||
        ScriptType.Equals(TEXT("ParticleUpdate"), ESearchCase::IgnoreCase))
        return Data->UpdateScriptProps.Script;
    if (ScriptType.Equals(TEXT("EmitterSpawn"), ESearchCase::IgnoreCase))
        return Data->EmitterSpawnScriptProps.Script;
    if (ScriptType.Equals(TEXT("EmitterUpdate"), ESearchCase::IgnoreCase))
        return Data->EmitterUpdateScriptProps.Script;
    return nullptr;
}

// ─── get_niagara_module_inputs ────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetNiagaraModuleInputs(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitter VerEmitter = Handles[EmitterIndex].GetInstance();
    FVersionedNiagaraEmitterData* Data = VerEmitter.GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("emitter"), Handles[EmitterIndex].GetName().ToString());

    // Iterate all script types and list their rapid iteration parameters
    TArray<TPair<FString, UNiagaraScript*>> Scripts;
    Scripts.Add({TEXT("ParticleSpawn"), Data->SpawnScriptProps.Script});
    Scripts.Add({TEXT("ParticleUpdate"), Data->UpdateScriptProps.Script});
    Scripts.Add({TEXT("EmitterSpawn"), Data->EmitterSpawnScriptProps.Script});
    Scripts.Add({TEXT("EmitterUpdate"), Data->EmitterUpdateScriptProps.Script});

    TArray<TSharedPtr<FJsonValue>> ScriptArray;
    for (auto& Pair : Scripts)
    {
        UNiagaraScript* Script = Pair.Value;
        if (!Script) continue;

        TSharedPtr<FJsonObject> ScriptObj = MakeShared<FJsonObject>();
        ScriptObj->SetStringField(TEXT("script_type"), Pair.Key);

        TArray<TSharedPtr<FJsonValue>> ParamArray;
        const FNiagaraParameterStore& Store = Script->RapidIterationParameters;
        TArrayView<const FNiagaraVariableWithOffset> Variables = Store.ReadParameterVariables();

        for (const FNiagaraVariableWithOffset& Var : Variables)
        {
            TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
            ParamObj->SetStringField(TEXT("name"), Var.GetName().ToString());
            ParamObj->SetStringField(TEXT("type"), Var.GetType().GetName());

            // Read value based on type
            const uint8* ValuePtr = Store.GetParameterData(Var);
            if (ValuePtr)
            {
                if (Var.GetType() == FNiagaraTypeDefinition::GetFloatDef())
                {
                    float Val = *(const float*)ValuePtr;
                    ParamObj->SetNumberField(TEXT("value"), Val);
                }
                else if (Var.GetType() == FNiagaraTypeDefinition::GetIntDef() ||
                         Var.GetType() == FNiagaraTypeDefinition::GetBoolDef())
                {
                    int32 Val = *(const int32*)ValuePtr;
                    ParamObj->SetNumberField(TEXT("value"), Val);
                }
                else if (Var.GetType() == FNiagaraTypeDefinition::GetVec3Def() ||
                         Var.GetType() == FNiagaraTypeDefinition::GetPositionDef())
                {
                    const FVector3f* Vec = (const FVector3f*)ValuePtr;
                    ParamObj->SetStringField(TEXT("value"),
                        FString::Printf(TEXT("(%f, %f, %f)"), Vec->X, Vec->Y, Vec->Z));
                }
                else if (Var.GetType() == FNiagaraTypeDefinition::GetVec2Def())
                {
                    const FVector2f* Vec = (const FVector2f*)ValuePtr;
                    ParamObj->SetStringField(TEXT("value"),
                        FString::Printf(TEXT("(%f, %f)"), Vec->X, Vec->Y));
                }
                else
                {
                    ParamObj->SetStringField(TEXT("value"), TEXT("<complex>"));
                }
            }

            ParamArray.Add(MakeShared<FJsonValueObject>(ParamObj));
        }
        ScriptObj->SetArrayField(TEXT("parameters"), ParamArray);
        ScriptArray.Add(MakeShared<FJsonValueObject>(ScriptObj));
    }

    Result->SetArrayField(TEXT("scripts"), ScriptArray);
    return Result;
}

// ─── set_niagara_module_input ─────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraModuleInput(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString ScriptType;
    if (!Params->TryGetStringField(TEXT("script_type"), ScriptType))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'script_type' (ParticleSpawn, ParticleUpdate, EmitterSpawn, EmitterUpdate)"));

    FString ParamName;
    if (!Params->TryGetStringField(TEXT("param_name"), ParamName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_name'"));

    FString ParamValue;
    if (!Params->TryGetStringField(TEXT("param_value"), ParamValue))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_value'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitter VerEmitter = Handles[EmitterIndex].GetInstance();
    FVersionedNiagaraEmitterData* Data = VerEmitter.GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    UNiagaraScript* Script = GetEmitterScript(Data, ScriptType);
    if (!Script)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Script type '%s' not found"), *ScriptType));

    FNiagaraParameterStore& Store = Script->RapidIterationParameters;
    TArrayView<const FNiagaraVariableWithOffset> Variables = Store.ReadParameterVariables();

    // Find parameter by name (partial match allowed)
    const FNiagaraVariableWithOffset* Found = nullptr;
    for (const FNiagaraVariableWithOffset& Var : Variables)
    {
        FString VarName = Var.GetName().ToString();
        if (VarName == ParamName || VarName.Find(ParamName) != INDEX_NONE)
        {
            Found = &Var;
            break;
        }
    }

    if (!Found)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Parameter '%s' not found in %s script"), *ParamName, *ScriptType));

    Script->Modify();
    bool bSuccess = false;
    FNiagaraVariable SetVar(Found->GetType(), Found->GetName());

    if (Found->GetType() == FNiagaraTypeDefinition::GetFloatDef())
    {
        float Val = FCString::Atof(*ParamValue);
        bSuccess = Store.SetParameterValue<float>(Val, SetVar, true);
    }
    else if (Found->GetType() == FNiagaraTypeDefinition::GetIntDef())
    {
        int32 Val = FCString::Atoi(*ParamValue);
        bSuccess = Store.SetParameterValue<int32>(Val, SetVar, true);
    }
    else if (Found->GetType() == FNiagaraTypeDefinition::GetBoolDef())
    {
        int32 Val = ParamValue.ToBool() ? 1 : 0;
        bSuccess = Store.SetParameterValue<int32>(Val, SetVar, true);
    }
    else if (Found->GetType() == FNiagaraTypeDefinition::GetVec3Def() ||
             Found->GetType() == FNiagaraTypeDefinition::GetPositionDef())
    {
        FString Clean = ParamValue.Replace(TEXT("("), TEXT("")).Replace(TEXT(")"), TEXT("")).TrimStartAndEnd();
        TArray<FString> Parts;
        Clean.ParseIntoArray(Parts, TEXT(","));
        if (Parts.Num() >= 3)
        {
            FVector3f Vec(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2]));
            bSuccess = Store.SetParameterValue<FVector3f>(Vec, SetVar, true);
        }
    }
    else if (Found->GetType() == FNiagaraTypeDefinition::GetVec2Def())
    {
        FString Clean = ParamValue.Replace(TEXT("("), TEXT("")).Replace(TEXT(")"), TEXT("")).TrimStartAndEnd();
        TArray<FString> Parts;
        Clean.ParseIntoArray(Parts, TEXT(","));
        if (Parts.Num() >= 2)
        {
            FVector2f Vec(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]));
            bSuccess = Store.SetParameterValue<FVector2f>(Vec, SetVar, true);
        }
    }

    if (!bSuccess)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not set parameter '%s' (type: %s) - unsupported type or bad value format"),
                *ParamName, *Found->GetType().GetName()));

    // Trigger notifications so the editor picks up the change
    Store.TriggerOnLayoutChanged();
    System->RequestCompile(false);
    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("param_name"), Found->GetName().ToString());
    Result->SetStringField(TEXT("param_value"), ParamValue);
    return Result;
}

// ─── Helper: get script source and output node for a usage ────────────────────

static UNiagaraNodeOutput* FindOutputNodeForUsage(FVersionedNiagaraEmitterData* Data, const FString& ScriptType)
{
    UNiagaraScript* Script = GetEmitterScript(Data, ScriptType);
    if (!Script) return nullptr;

    UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Script->GetLatestSource());
    if (!Source) return nullptr;

    UNiagaraGraph* Graph = Source->NodeGraph;
    if (!Graph) return nullptr;

    ENiagaraScriptUsage Usage;
    if (ScriptType.Equals(TEXT("ParticleSpawn"), ESearchCase::IgnoreCase) || ScriptType.Equals(TEXT("Spawn"), ESearchCase::IgnoreCase))
        Usage = ENiagaraScriptUsage::ParticleSpawnScript;
    else if (ScriptType.Equals(TEXT("ParticleUpdate"), ESearchCase::IgnoreCase) || ScriptType.Equals(TEXT("Update"), ESearchCase::IgnoreCase))
        Usage = ENiagaraScriptUsage::ParticleUpdateScript;
    else if (ScriptType.Equals(TEXT("EmitterSpawn"), ESearchCase::IgnoreCase))
        Usage = ENiagaraScriptUsage::EmitterSpawnScript;
    else if (ScriptType.Equals(TEXT("EmitterUpdate"), ESearchCase::IgnoreCase))
        Usage = ENiagaraScriptUsage::EmitterUpdateScript;
    else
        return nullptr;

    return Graph->FindEquivalentOutputNode(Usage);
}

// ─── set_niagara_emitter_property ─────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraEmitterProperty(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name'"));

    FString PropertyValue;
    if (!Params->TryGetStringField(TEXT("property_value"), PropertyValue))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* Data = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    // Set known emitter-level properties
    bool bSuccess = false;
    if (PropertyName.Equals(TEXT("SimTarget"), ESearchCase::IgnoreCase))
    {
        if (PropertyValue.Equals(TEXT("CPU"), ESearchCase::IgnoreCase))
        { Data->SimTarget = ENiagaraSimTarget::CPUSim; bSuccess = true; }
        else if (PropertyValue.Equals(TEXT("GPU"), ESearchCase::IgnoreCase))
        { Data->SimTarget = ENiagaraSimTarget::GPUComputeSim; bSuccess = true; }
    }
    else if (PropertyName.Equals(TEXT("LocalSpace"), ESearchCase::IgnoreCase))
    { Data->bLocalSpace = PropertyValue.ToBool(); bSuccess = true; }
    else if (PropertyName.Equals(TEXT("Determinism"), ESearchCase::IgnoreCase))
    { Data->bDeterminism = PropertyValue.ToBool(); bSuccess = true; }
    else if (PropertyName.Equals(TEXT("InterpolatedSpawning"), ESearchCase::IgnoreCase))
    {
        Data->InterpolatedSpawnMode = PropertyValue.ToBool()
            ? ENiagaraInterpolatedSpawnMode::Interpolation
            : ENiagaraInterpolatedSpawnMode::NoInterpolation;
        bSuccess = true;
    }

    if (!bSuccess)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unknown or unsupported emitter property: %s (supported: SimTarget, LocalSpace, Determinism, InterpolatedSpawning)"), *PropertyName));

    System->RequestCompile(false);
    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("property_name"), PropertyName);
    Result->SetStringField(TEXT("property_value"), PropertyValue);
    return Result;
}

// ─── list_niagara_modules ─────────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleListNiagaraModules(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString ScriptType = TEXT("ParticleSpawn");
    Params->TryGetStringField(TEXT("script_type"), ScriptType);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* Data = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    UNiagaraNodeOutput* OutputNode = FindOutputNodeForUsage(Data, ScriptType);
    if (!OutputNode)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find output node for script type: %s"), *ScriptType));

    // Walk the linked input chain from the output node to find all module calls
    TArray<TSharedPtr<FJsonValue>> ModuleArray;
    TArray<UNiagaraNodeFunctionCall*> FunctionNodes;

    // Traverse the graph from the output node upstream
    UEdGraphPin* InputExecPin = OutputNode->Pins.Num() > 0 ? OutputNode->Pins[0] : nullptr;
    if (InputExecPin)
    {
        TArray<UEdGraphNode*> Visited;
        TQueue<UEdGraphNode*> Queue;
        for (UEdGraphPin* LinkedPin : InputExecPin->LinkedTo)
        {
            if (LinkedPin && LinkedPin->GetOwningNode())
                Queue.Enqueue(LinkedPin->GetOwningNode());
        }

        while (!Queue.IsEmpty())
        {
            UEdGraphNode* Node = nullptr;
            Queue.Dequeue(Node);
            if (!Node || Visited.Contains(Node)) continue;
            Visited.Add(Node);

            if (UNiagaraNodeFunctionCall* FuncNode = Cast<UNiagaraNodeFunctionCall>(Node))
            {
                FunctionNodes.Add(FuncNode);
            }

            // Continue upstream
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (Pin && Pin->Direction == EGPD_Input)
                {
                    for (UEdGraphPin* Linked : Pin->LinkedTo)
                    {
                        if (Linked && Linked->GetOwningNode())
                            Queue.Enqueue(Linked->GetOwningNode());
                    }
                }
            }
        }
    }

    // Build result (reverse order to show top-to-bottom stack order)
    for (int32 i = FunctionNodes.Num() - 1; i >= 0; --i)
    {
        UNiagaraNodeFunctionCall* FuncNode = FunctionNodes[i];
        TSharedPtr<FJsonObject> ModObj = MakeShared<FJsonObject>();
        ModObj->SetStringField(TEXT("name"), FuncNode->GetNodeTitle(ENodeTitleType::ListView).ToString());
        ModObj->SetStringField(TEXT("node_name"), FuncNode->GetName());
        ModObj->SetNumberField(TEXT("index"), FunctionNodes.Num() - 1 - i);

        ModObj->SetBoolField(TEXT("enabled"), FuncNode->IsNodeEnabled());

        if (FuncNode->FunctionScript)
            ModObj->SetStringField(TEXT("script_path"), FuncNode->FunctionScript->GetPathName());

        ModuleArray.Add(MakeShared<FJsonValueObject>(ModObj));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("emitter"), Handles[EmitterIndex].GetName().ToString());
    Result->SetStringField(TEXT("script_type"), ScriptType);
    Result->SetArrayField(TEXT("modules"), ModuleArray);
    return Result;
}

// ─── add_niagara_module ───────────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddNiagaraModule(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString ScriptType = TEXT("ParticleSpawn");
    Params->TryGetStringField(TEXT("script_type"), ScriptType);

    FString ModulePath;
    if (!Params->TryGetStringField(TEXT("module_path"), ModulePath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_path' (asset path to the Niagara module script)"));

    int32 TargetIndex = INDEX_NONE;
    Params->TryGetNumberField(TEXT("target_index"), TargetIndex);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* Data = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    UNiagaraNodeOutput* OutputNode = FindOutputNodeForUsage(Data, ScriptType);
    if (!OutputNode)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find output node for script type: %s"), *ScriptType));

    // Load the module script
    UNiagaraScript* ModuleScript = LoadObject<UNiagaraScript>(nullptr, *ModulePath);
    if (!ModuleScript)
    {
        // Try asset registry search
        FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        TArray<FAssetData> Assets;
        ARM.Get().GetAssetsByClass(UNiagaraScript::StaticClass()->GetClassPathName(), Assets);
        for (const FAssetData& Asset : Assets)
        {
            if (Asset.AssetName.ToString().Find(ModulePath) != INDEX_NONE)
            {
                ModuleScript = Cast<UNiagaraScript>(Asset.GetAsset());
                if (ModuleScript) break;
            }
        }
    }

    if (!ModuleScript)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Module script not found: %s"), *ModulePath));

    System->Modify();
    UNiagaraNodeFunctionCall* NewNode = FNiagaraStackGraphUtilities::AddScriptModuleToStack(
        ModuleScript, *OutputNode, TargetIndex);

    if (!NewNode)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to add module to stack"));

    System->RequestCompile(false);
    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("module_name"), NewNode->GetNodeTitle(ENodeTitleType::ListView).ToString());
    Result->SetStringField(TEXT("node_name"), NewNode->GetName());
    return Result;
}

// ─── remove_niagara_module ────────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleRemoveNiagaraModule(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString ScriptType = TEXT("ParticleSpawn");
    Params->TryGetStringField(TEXT("script_type"), ScriptType);

    FString ModuleName;
    if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* Data = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    UNiagaraNodeOutput* OutputNode = FindOutputNodeForUsage(Data, ScriptType);
    if (!OutputNode)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Output node not found"));

    // Find the module node by name
    UNiagaraNodeFunctionCall* FoundNode = nullptr;
    UNiagaraGraph* Graph = Cast<UNiagaraGraph>(OutputNode->GetGraph());
    if (Graph)
    {
        TArray<UNiagaraNodeFunctionCall*> FuncNodes;
        Graph->GetNodesOfClass<UNiagaraNodeFunctionCall>(FuncNodes);
        for (UNiagaraNodeFunctionCall* Node : FuncNodes)
        {
            FString Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
            if (Title.Find(ModuleName) != INDEX_NONE || Node->GetName().Find(ModuleName) != INDEX_NONE)
            {
                FoundNode = Node;
                break;
            }
        }
    }

    if (!FoundNode)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Module '%s' not found in %s"), *ModuleName, *ScriptType));

    System->Modify();

    // Break all pin connections and remove the node from the graph
    UEdGraph* OwningGraph = FoundNode->GetGraph();
    if (!OwningGraph)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Could not find graph for module node"));

    FoundNode->BreakAllNodeLinks();
    OwningGraph->RemoveNode(FoundNode);

    System->RequestCompile(false);
    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("removed_module"), ModuleName);
    return Result;
}

// ─── set_niagara_module_enabled ───────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraModuleEnabled(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString ScriptType = TEXT("ParticleSpawn");
    Params->TryGetStringField(TEXT("script_type"), ScriptType);

    FString ModuleName;
    if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name'"));

    bool bEnabled = true;
    Params->TryGetBoolField(TEXT("enabled"), bEnabled);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* Data = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    UNiagaraNodeOutput* OutputNode = FindOutputNodeForUsage(Data, ScriptType);
    if (!OutputNode)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Output node not found"));

    UNiagaraNodeFunctionCall* FoundNode = nullptr;
    UNiagaraGraph* Graph = Cast<UNiagaraGraph>(OutputNode->GetGraph());
    if (Graph)
    {
        TArray<UNiagaraNodeFunctionCall*> FuncNodes;
        Graph->GetNodesOfClass<UNiagaraNodeFunctionCall>(FuncNodes);
        for (UNiagaraNodeFunctionCall* Node : FuncNodes)
        {
            FString Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
            if (Title.Find(ModuleName) != INDEX_NONE || Node->GetName().Find(ModuleName) != INDEX_NONE)
            {
                FoundNode = Node;
                break;
            }
        }
    }

    if (!FoundNode)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Module '%s' not found in %s"), *ModuleName, *ScriptType));

    FNiagaraStackGraphUtilities::SetModuleIsEnabled(*FoundNode, bEnabled);
    System->RequestCompile(false);
    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("module_name"), ModuleName);
    Result->SetBoolField(TEXT("enabled"), bEnabled);
    return Result;
}

// ─── Helper: find a module node by name in a script stage ─────────────────────

static UNiagaraNodeFunctionCall* FindModuleNodeByName(FVersionedNiagaraEmitterData* Data, const FString& ScriptType, const FString& ModuleName)
{
    UNiagaraNodeOutput* OutputNode = FindOutputNodeForUsage(Data, ScriptType);
    if (!OutputNode) return nullptr;

    UNiagaraGraph* Graph = Cast<UNiagaraGraph>(OutputNode->GetGraph());
    if (!Graph) return nullptr;

    TArray<UNiagaraNodeFunctionCall*> FuncNodes;
    Graph->GetNodesOfClass<UNiagaraNodeFunctionCall>(FuncNodes);
    for (UNiagaraNodeFunctionCall* Node : FuncNodes)
    {
        FString Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
        if (Title.Find(ModuleName) != INDEX_NONE || Node->GetName().Find(ModuleName) != INDEX_NONE)
            return Node;
    }
    return nullptr;
}

// ─── get_niagara_module_pins ──────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetNiagaraModulePins(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString ScriptType = TEXT("ParticleSpawn");
    Params->TryGetStringField(TEXT("script_type"), ScriptType);

    FString ModuleName;
    if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* Data = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    UNiagaraNodeFunctionCall* FuncNode = FindModuleNodeByName(Data, ScriptType, ModuleName);
    if (!FuncNode)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Module '%s' not found in %s"), *ModuleName, *ScriptType));

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("module"), FuncNode->GetNodeTitle(ENodeTitleType::ListView).ToString());

    TArray<TSharedPtr<FJsonValue>> PinArray;
    for (UEdGraphPin* Pin : FuncNode->Pins)
    {
        if (!Pin) continue;

        TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
        PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
        PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));
        PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
        PinObj->SetStringField(TEXT("sub_type"), Pin->PinType.PinSubCategory.ToString());
        PinObj->SetStringField(TEXT("default_value"), Pin->DefaultValue);
        PinObj->SetBoolField(TEXT("is_connected"), Pin->LinkedTo.Num() > 0);
        PinObj->SetBoolField(TEXT("hidden"), Pin->bHidden);

        if (Pin->PinType.PinSubCategoryObject.IsValid())
            PinObj->SetStringField(TEXT("sub_type_object"), Pin->PinType.PinSubCategoryObject->GetName());

        PinArray.Add(MakeShared<FJsonValueObject>(PinObj));
    }

    Result->SetArrayField(TEXT("pins"), PinArray);
    return Result;
}

// ─── set_niagara_module_pin_value ─────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraModulePinValue(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString ScriptType = TEXT("ParticleSpawn");
    Params->TryGetStringField(TEXT("script_type"), ScriptType);

    FString ModuleName;
    if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name'"));

    FString PinName;
    if (!Params->TryGetStringField(TEXT("pin_name"), PinName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pin_name'"));

    FString PinValue;
    if (!Params->TryGetStringField(TEXT("pin_value"), PinValue))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pin_value'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* Data = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    UNiagaraNodeFunctionCall* FuncNode = FindModuleNodeByName(Data, ScriptType, ModuleName);
    if (!FuncNode)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Module '%s' not found in %s"), *ModuleName, *ScriptType));

    // Find the pin by name (partial match)
    UEdGraphPin* FoundPin = nullptr;
    for (UEdGraphPin* Pin : FuncNode->Pins)
    {
        if (!Pin || Pin->Direction != EGPD_Input) continue;
        FString Name = Pin->PinName.ToString();
        if (Name == PinName || Name.Find(PinName) != INDEX_NONE)
        {
            FoundPin = Pin;
            break;
        }
    }

    if (!FoundPin)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Pin '%s' not found on module '%s'"), *PinName, *ModuleName));

    FuncNode->Modify();
    FoundPin->DefaultValue = PinValue;

    // Refresh the node to process static switch changes (shows/hides dependent inputs)
    FuncNode->RefreshFromExternalChanges();

    // Notify graph of change
    UNiagaraGraph* Graph = Cast<UNiagaraGraph>(FuncNode->GetGraph());
    if (Graph)
    {
        Graph->NotifyGraphChanged();
    }

    System->RequestCompile(false);
    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("pin_name"), FoundPin->PinName.ToString());
    Result->SetStringField(TEXT("pin_value"), PinValue);
    return Result;
}

// ─── get_niagara_user_parameters ──────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetNiagaraUserParameters(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const FNiagaraUserRedirectionParameterStore& UserParams = System->GetExposedParameters();
    TArrayView<const FNiagaraVariableWithOffset> Variables = UserParams.ReadParameterVariables();

    TArray<TSharedPtr<FJsonValue>> ParamArray;
    for (const FNiagaraVariableWithOffset& Var : Variables)
    {
        TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
        ParamObj->SetStringField(TEXT("name"), Var.GetName().ToString());
        ParamObj->SetStringField(TEXT("type"), Var.GetType().GetName());

        const uint8* ValuePtr = UserParams.GetParameterData(Var);
        if (ValuePtr)
        {
            if (Var.GetType() == FNiagaraTypeDefinition::GetFloatDef())
                ParamObj->SetNumberField(TEXT("value"), *(const float*)ValuePtr);
            else if (Var.GetType() == FNiagaraTypeDefinition::GetIntDef())
                ParamObj->SetNumberField(TEXT("value"), *(const int32*)ValuePtr);
            else if (Var.GetType() == FNiagaraTypeDefinition::GetBoolDef())
                ParamObj->SetNumberField(TEXT("value"), *(const int32*)ValuePtr);
            else
                ParamObj->SetStringField(TEXT("value"), TEXT("<complex>"));
        }

        ParamArray.Add(MakeShared<FJsonValueObject>(ParamObj));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetArrayField(TEXT("parameters"), ParamArray);
    return Result;
}

// ─── set_niagara_user_parameter ───────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraUserParameter(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    FString ParamName;
    if (!Params->TryGetStringField(TEXT("param_name"), ParamName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_name'"));

    FString ParamValue;
    if (!Params->TryGetStringField(TEXT("param_value"), ParamValue))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_value'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    FNiagaraUserRedirectionParameterStore& UserParams = System->GetExposedParameters();
    TArrayView<const FNiagaraVariableWithOffset> Variables = UserParams.ReadParameterVariables();

    const FNiagaraVariableWithOffset* Found = nullptr;
    for (const FNiagaraVariableWithOffset& Var : Variables)
    {
        if (Var.GetName().ToString().Find(ParamName) != INDEX_NONE)
        {
            Found = &Var;
            break;
        }
    }

    if (!Found)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("User parameter '%s' not found"), *ParamName));

    FNiagaraVariable SetVar(Found->GetType(), Found->GetName());
    bool bSuccess = false;

    if (Found->GetType() == FNiagaraTypeDefinition::GetFloatDef())
    {
        float Val = FCString::Atof(*ParamValue);
        bSuccess = UserParams.SetParameterValue<float>(Val, SetVar, true);
    }
    else if (Found->GetType() == FNiagaraTypeDefinition::GetIntDef())
    {
        int32 Val = FCString::Atoi(*ParamValue);
        bSuccess = UserParams.SetParameterValue<int32>(Val, SetVar, true);
    }
    else if (Found->GetType() == FNiagaraTypeDefinition::GetBoolDef())
    {
        int32 Val = ParamValue.ToBool() ? 1 : 0;
        bSuccess = UserParams.SetParameterValue<int32>(Val, SetVar, true);
    }

    if (!bSuccess)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to set user parameter"));

    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("param_name"), Found->GetName().ToString());
    return Result;
}

// ─── search_niagara_modules ──────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSearchNiagaraModules(
    const TSharedPtr<FJsonObject>& Params)
{
    FString Query;
    if (!Params->TryGetStringField(TEXT("query"), Query))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'query'"));

    int32 MaxResults = 20;
    Params->TryGetNumberField(TEXT("max_results"), MaxResults);

    FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> Assets;
    ARM.Get().GetAssetsByClass(UNiagaraScript::StaticClass()->GetClassPathName(), Assets);

    TArray<TSharedPtr<FJsonValue>> ResultArray;
    int32 Count = 0;

    for (const FAssetData& Asset : Assets)
    {
        if (Count >= MaxResults) break;

        FString Name = Asset.AssetName.ToString();
        FString Path = Asset.GetObjectPathString();

        if (Name.Find(Query) != INDEX_NONE || Path.Find(Query) != INDEX_NONE)
        {
            TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
            Entry->SetStringField(TEXT("name"), Name);
            Entry->SetStringField(TEXT("path"), Path);
            Entry->SetStringField(TEXT("package"), Asset.PackageName.ToString());
            ResultArray.Add(MakeShared<FJsonValueObject>(Entry));
            Count++;
        }
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("query"), Query);
    Result->SetNumberField(TEXT("count"), Count);
    Result->SetArrayField(TEXT("modules"), ResultArray);
    return Result;
}

// ─── add_niagara_set_parameter ────────────────────────────────────────────────
// Adds a "Set New or Existing Parameter Directly" node (UNiagaraNodeAssignment).

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddNiagaraSetParameter(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString ScriptType = TEXT("ParticleSpawn");
    Params->TryGetStringField(TEXT("script_type"), ScriptType);

    FString ParamName;
    if (!Params->TryGetStringField(TEXT("param_name"), ParamName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_name' (e.g. 'Particles.RibbonID')"));

    FString ParamType = TEXT("int");
    Params->TryGetStringField(TEXT("param_type"), ParamType);

    FString DefaultValue = TEXT("0");
    Params->TryGetStringField(TEXT("default_value"), DefaultValue);

    int32 TargetIndex = INDEX_NONE;
    Params->TryGetNumberField(TEXT("target_index"), TargetIndex);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* Data = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    UNiagaraNodeOutput* OutputNode = FindOutputNodeForUsage(Data, ScriptType);
    if (!OutputNode)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Output node not found"));

    // Determine Niagara type
    FNiagaraTypeDefinition TypeDef;
    if (ParamType.Equals(TEXT("int"), ESearchCase::IgnoreCase) || ParamType.Equals(TEXT("int32"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetIntDef();
    else if (ParamType.Equals(TEXT("float"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetFloatDef();
    else if (ParamType.Equals(TEXT("bool"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetBoolDef();
    else if (ParamType.Equals(TEXT("vec3"), ESearchCase::IgnoreCase) || ParamType.Equals(TEXT("vector"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetVec3Def();
    else if (ParamType.Equals(TEXT("vec2"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetVec2Def();
    else if (ParamType.Equals(TEXT("vec4"), ESearchCase::IgnoreCase) || ParamType.Equals(TEXT("color"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetVec4Def();
    else if (ParamType.Equals(TEXT("position"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetPositionDef();
    else
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unsupported param_type: %s (use int, float, bool, vec2, vec3, vec4, position, color)"), *ParamType));

    FNiagaraVariable Variable(TypeDef, FName(*ParamName));

    System->Modify();
    TArray<FNiagaraVariable> Variables;
    Variables.Add(Variable);
    TArray<FString> DefaultValues;
    DefaultValues.Add(DefaultValue);

    // Validate graph structure before calling AddParameterModuleToStack
    // The function asserts StackNodeGroups.Num() >= 2 which fails on some emitter templates
    UNiagaraNodeAssignment* AssignNode = nullptr;
    UNiagaraGraph* Graph = Cast<UNiagaraGraph>(OutputNode->GetGraph());

    if (Graph)
    {
        {
            // Fallback: manually create assignment node and wire it
            AssignNode = NewObject<UNiagaraNodeAssignment>(Graph);
            AssignNode->AddAssignmentTarget(Variable, &DefaultValue);
            AssignNode->CreateNewGuid();
            AssignNode->AllocateDefaultPins();
            Graph->AddNode(AssignNode, false, false);

            // Wire: insert before the output node's first input pin
            UEdGraphPin* OutputInputPin = nullptr;
            for (UEdGraphPin* Pin : OutputNode->Pins)
            {
                if (Pin && Pin->Direction == EGPD_Input)
                {
                    OutputInputPin = Pin;
                    break;
                }
            }

            if (OutputInputPin && AssignNode->Pins.Num() >= 2)
            {
                UEdGraphPin* AssignInput = AssignNode->Pins[0];   // parameter map in
                UEdGraphPin* AssignOutput = AssignNode->Pins[1];  // parameter map out

                if (OutputInputPin->LinkedTo.Num() > 0)
                {
                    // Insert between previous module and output
                    UEdGraphPin* PrevOutput = OutputInputPin->LinkedTo[0];
                    OutputInputPin->BreakAllPinLinks();
                    PrevOutput->MakeLinkTo(AssignInput);
                    AssignOutput->MakeLinkTo(OutputInputPin);
                }
                else
                {
                    AssignOutput->MakeLinkTo(OutputInputPin);
                }
            }

            Graph->NotifyGraphChanged();
        }
    }

    if (!AssignNode)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to add parameter assignment node"));

    System->RequestCompile(false);
    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("param_name"), ParamName);
    Result->SetStringField(TEXT("param_type"), ParamType);
    Result->SetStringField(TEXT("default_value"), DefaultValue);
    Result->SetStringField(TEXT("node_name"), AssignNode->GetName());
    return Result;
}

// ─── compile_niagara_system ───────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleCompileNiagaraSystem(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    bool bForce = false;
    Params->TryGetBoolField(TEXT("force"), bForce);

    System->RequestCompile(bForce);
    System->WaitForCompilationComplete(false, false);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("system_path"), System->GetPathName());
    return Result;
}

// ─── set_niagara_fixed_bounds ─────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraFixedBounds(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    double MinX = -100, MinY = -100, MinZ = -100;
    double MaxX = 100, MaxY = 100, MaxZ = 100;
    Params->TryGetNumberField(TEXT("min_x"), MinX);
    Params->TryGetNumberField(TEXT("min_y"), MinY);
    Params->TryGetNumberField(TEXT("min_z"), MinZ);
    Params->TryGetNumberField(TEXT("max_x"), MaxX);
    Params->TryGetNumberField(TEXT("max_y"), MaxY);
    Params->TryGetNumberField(TEXT("max_z"), MaxZ);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* Data = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    Data->FixedBounds = FBox(FVector(MinX, MinY, MinZ), FVector(MaxX, MaxY, MaxZ));
    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    return Result;
}

// ─── rename_niagara_emitter ───────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleRenameNiagaraEmitter(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString NewName;
    if (!Params->TryGetStringField(TEXT("new_name"), NewName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'new_name'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    TArray<FNiagaraEmitterHandle>& Handles = const_cast<TArray<FNiagaraEmitterHandle>&>(System->GetEmitterHandles());
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FString OldName = Handles[EmitterIndex].GetName().ToString();
    Handles[EmitterIndex].SetName(FName(*NewName), *System);
    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("old_name"), OldName);
    Result->SetStringField(TEXT("new_name"), NewName);
    return Result;
}

// ─── add_niagara_user_parameter ───────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddNiagaraUserParameter(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    FString ParamName;
    if (!Params->TryGetStringField(TEXT("param_name"), ParamName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_name'"));

    FString ParamType = TEXT("float");
    Params->TryGetStringField(TEXT("param_type"), ParamType);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    FNiagaraTypeDefinition TypeDef;
    if (ParamType.Equals(TEXT("float"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetFloatDef();
    else if (ParamType.Equals(TEXT("int"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetIntDef();
    else if (ParamType.Equals(TEXT("bool"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetBoolDef();
    else if (ParamType.Equals(TEXT("vec3"), ESearchCase::IgnoreCase) || ParamType.Equals(TEXT("vector"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetVec3Def();
    else if (ParamType.Equals(TEXT("vec4"), ESearchCase::IgnoreCase) || ParamType.Equals(TEXT("color"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetVec4Def();
    else
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unsupported param_type: %s"), *ParamType));

    // Ensure name is in User namespace
    FString FullName = ParamName;
    if (!FullName.StartsWith(TEXT("User.")))
        FullName = TEXT("User.") + FullName;

    FNiagaraVariable NewParam(TypeDef, FName(*FullName));
    FNiagaraUserRedirectionParameterStore& UserParams = System->GetExposedParameters();
    bool bAdded = UserParams.AddParameter(NewParam, true, true);

    if (!bAdded)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to add user parameter (may already exist)"));

    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("param_name"), FullName);
    Result->SetStringField(TEXT("param_type"), ParamType);
    return Result;
}

// ─── get_niagara_emitter_properties ───────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetNiagaraEmitterProperties(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    const FNiagaraEmitterHandle& Handle = Handles[EmitterIndex];
    FVersionedNiagaraEmitterData* Data = Handle.GetInstance().GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("name"), Handle.GetName().ToString());
    Result->SetBoolField(TEXT("enabled"), Handle.GetIsEnabled());
    Result->SetStringField(TEXT("sim_target"),
        Data->SimTarget == ENiagaraSimTarget::CPUSim ? TEXT("CPU") : TEXT("GPU"));
    Result->SetBoolField(TEXT("local_space"), Data->bLocalSpace);
    Result->SetBoolField(TEXT("determinism"), Data->bDeterminism);
    Result->SetStringField(TEXT("interpolated_spawn_mode"),
        Data->InterpolatedSpawnMode == ENiagaraInterpolatedSpawnMode::Interpolation ? TEXT("Interpolation") :
        Data->InterpolatedSpawnMode == ENiagaraInterpolatedSpawnMode::RunUpdateScript ? TEXT("RunUpdateScript") :
        TEXT("NoInterpolation"));

    // Fixed bounds
    const FBox& Bounds = Data->FixedBounds;
    TSharedPtr<FJsonObject> BoundsObj = MakeShared<FJsonObject>();
    BoundsObj->SetNumberField(TEXT("min_x"), Bounds.Min.X);
    BoundsObj->SetNumberField(TEXT("min_y"), Bounds.Min.Y);
    BoundsObj->SetNumberField(TEXT("min_z"), Bounds.Min.Z);
    BoundsObj->SetNumberField(TEXT("max_x"), Bounds.Max.X);
    BoundsObj->SetNumberField(TEXT("max_y"), Bounds.Max.Y);
    BoundsObj->SetNumberField(TEXT("max_z"), Bounds.Max.Z);
    Result->SetObjectField(TEXT("fixed_bounds"), BoundsObj);

    // Renderer count
    Result->SetNumberField(TEXT("renderer_count"), Data->GetRenderers().Num());

    return Result;
}

// ─── get_niagara_renderer_bindings ────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetNiagaraRendererBindings(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    int32 RendererIndex = 0;
    Params->TryGetNumberField(TEXT("renderer_index"), RendererIndex);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* Data = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    const TArray<UNiagaraRendererProperties*>& Renderers = Data->GetRenderers();
    if (!Renderers.IsValidIndex(RendererIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid renderer_index"));

    UNiagaraRendererProperties* Renderer = Renderers[RendererIndex];

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("type"), RendererTypeToString(Renderer));

    // Iterate all FNiagaraVariableAttributeBinding properties via reflection
    TArray<TSharedPtr<FJsonValue>> BindingArray;
    for (TFieldIterator<FStructProperty> It(Renderer->GetClass()); It; ++It)
    {
        FStructProperty* StructProp = *It;
        if (!StructProp->Struct || StructProp->Struct->GetFName() != FName("NiagaraVariableAttributeBinding"))
            continue;

        const FNiagaraVariableAttributeBinding* Binding =
            StructProp->ContainerPtrToValuePtr<FNiagaraVariableAttributeBinding>(Renderer);
        if (!Binding) continue;

        TSharedPtr<FJsonObject> BindObj = MakeShared<FJsonObject>();
        BindObj->SetStringField(TEXT("property"), StructProp->GetName());
#if WITH_EDITORONLY_DATA
        BindObj->SetStringField(TEXT("display_name"), Binding->GetName().ToString());
#endif
        BindObj->SetStringField(TEXT("bound_variable"), Binding->GetParamMapBindableVariable().GetName().ToString());
        BindObj->SetStringField(TEXT("type"), Binding->GetType().GetName());
        BindObj->SetBoolField(TEXT("valid"), Binding->IsValid());

        BindingArray.Add(MakeShared<FJsonValueObject>(BindObj));
    }

    Result->SetArrayField(TEXT("bindings"), BindingArray);
    return Result;
}

// ─── set_niagara_renderer_binding ─────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraRendererBinding(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    int32 RendererIndex = 0;
    Params->TryGetNumberField(TEXT("renderer_index"), RendererIndex);

    FString BindingName;
    if (!Params->TryGetStringField(TEXT("binding_name"), BindingName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'binding_name' (e.g. 'RibbonIdBinding')"));

    FString AttributeName;
    if (!Params->TryGetStringField(TEXT("attribute_name"), AttributeName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'attribute_name' (e.g. 'Particles.RibbonID')"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitter VerEmitter = Handles[EmitterIndex].GetInstance();
    FVersionedNiagaraEmitterData* Data = VerEmitter.GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    const TArray<UNiagaraRendererProperties*>& Renderers = Data->GetRenderers();
    if (!Renderers.IsValidIndex(RendererIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid renderer_index"));

    UNiagaraRendererProperties* Renderer = Renderers[RendererIndex];

    // Find the binding property by name
    FStructProperty* FoundProp = nullptr;
    for (TFieldIterator<FStructProperty> It(Renderer->GetClass()); It; ++It)
    {
        FStructProperty* StructProp = *It;
        if (!StructProp->Struct || StructProp->Struct->GetFName() != FName("NiagaraVariableAttributeBinding"))
            continue;
        if (StructProp->GetName().Find(BindingName) != INDEX_NONE)
        {
            FoundProp = StructProp;
            break;
        }
    }

    if (!FoundProp)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Binding '%s' not found on renderer"), *BindingName));

    FNiagaraVariableAttributeBinding* Binding =
        FoundProp->ContainerPtrToValuePtr<FNiagaraVariableAttributeBinding>(Renderer);
    if (!Binding)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Could not access binding"));

    Renderer->Modify();
    FVersionedNiagaraEmitterBase EmitterBase = VerEmitter.ToBase();
    Binding->SetValue(FName(*AttributeName), EmitterBase, ENiagaraRendererSourceDataMode::Particles);

    System->RequestCompile(false);
    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("binding_name"), FoundProp->GetName());
    Result->SetStringField(TEXT("attribute_name"), AttributeName);
    return Result;
}

// ─── set_niagara_module_override ──────────────────────────────────────────────
// Sets ANY module input to a static value using the proper editor override system.
// This is the correct way to override module defaults that aren't in the rapid-iteration store.

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraModuleOverride(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString ScriptType = TEXT("ParticleSpawn");
    Params->TryGetStringField(TEXT("script_type"), ScriptType);

    FString ModuleName;
    if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name'"));

    FString InputName;
    if (!Params->TryGetStringField(TEXT("input_name"), InputName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'input_name' (e.g. 'Ribbon Width')"));

    FString Value;
    if (!Params->TryGetStringField(TEXT("value"), Value))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* Data = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!Data)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    // Find the module node
    UNiagaraNodeFunctionCall* FuncNode = FindModuleNodeByName(Data, ScriptType, ModuleName);
    if (!FuncNode)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Module '%s' not found in %s"), *ModuleName, *ScriptType));

    // Determine input type from user-provided type string (default: float)
    FString InputType = TEXT("float");
    Params->TryGetStringField(TEXT("input_type"), InputType);

    FNiagaraTypeDefinition TypeDef;
    if (InputType.Equals(TEXT("float"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetFloatDef();
    else if (InputType.Equals(TEXT("int"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetIntDef();
    else if (InputType.Equals(TEXT("bool"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetBoolDef();
    else if (InputType.Equals(TEXT("vec2"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetVec2Def();
    else if (InputType.Equals(TEXT("vec3"), ESearchCase::IgnoreCase) || InputType.Equals(TEXT("vector"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetVec3Def();
    else if (InputType.Equals(TEXT("vec4"), ESearchCase::IgnoreCase) || InputType.Equals(TEXT("color"), ESearchCase::IgnoreCase))
        TypeDef = FNiagaraTypeDefinition::GetVec4Def();
    else
        TypeDef = FNiagaraTypeDefinition::GetFloatDef();

    // Create the parameter handle from the input name
    FNiagaraParameterHandle InputHandle = FNiagaraParameterHandle::CreateModuleParameterHandle(FName(*InputName));
    FNiagaraParameterHandle AliasedHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(InputHandle, FuncNode);

    // Get or create the override pin
    System->Modify();
    UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
        *FuncNode, AliasedHandle, TypeDef, FGuid(), FGuid());

    // Set the pin's default value
    OverridePin.DefaultValue = Value;

    // Notify graph
    UNiagaraGraph* Graph = Cast<UNiagaraGraph>(FuncNode->GetGraph());
    if (Graph)
        Graph->NotifyGraphChanged();

    System->RequestCompile(false);
    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("module"), ModuleName);
    Result->SetStringField(TEXT("input"), InputName);
    Result->SetStringField(TEXT("value"), Value);
    Result->SetStringField(TEXT("override_pin"), OverridePin.PinName.ToString());
    return Result;
}

// --- get_niagara_deep_inspect ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetNiagaraDeepInspect(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Niagara system not found: %s"), *SystemPath));

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("system_name"), System->GetName());
    Result->SetStringField(TEXT("system_path"), System->GetPathName());
    Result->SetNumberField(TEXT("warmup_time"), System->GetWarmupTime());

    // User parameters
    TArray<TSharedPtr<FJsonValue>> UserParamsArray;
    auto ExposedVars = System->GetExposedParameters().ReadParameterVariables();
    for (int32 vi = 0; vi < ExposedVars.Num(); vi++)
    {
        TSharedPtr<FJsonObject> PObj = MakeShared<FJsonObject>();
        PObj->SetStringField(TEXT("name"), ExposedVars[vi].GetName().ToString());
        PObj->SetStringField(TEXT("type"), ExposedVars[vi].GetType().GetName());
        UserParamsArray.Add(MakeShared<FJsonValueObject>(PObj));
    }
    Result->SetArrayField(TEXT("user_parameters"), UserParamsArray);

    // Emitters
    TArray<TSharedPtr<FJsonValue>> EmitterArray;
    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();

    for (int32 i = 0; i < Handles.Num(); i++)
    {
        const FNiagaraEmitterHandle& Handle = Handles[i];
        FVersionedNiagaraEmitterData* EmitterData = Handle.GetInstance().GetEmitterData();
        if (!EmitterData) continue;

        TSharedPtr<FJsonObject> EmObj = MakeShared<FJsonObject>();
        EmObj->SetStringField(TEXT("name"), Handle.GetName().ToString());
        EmObj->SetNumberField(TEXT("index"), i);
        EmObj->SetBoolField(TEXT("enabled"), Handle.GetIsEnabled());
        EmObj->SetStringField(TEXT("sim_target"),
            EmitterData->SimTarget == ENiagaraSimTarget::CPUSim ? TEXT("CPU") : TEXT("GPU"));

        // Renderers
        TArray<TSharedPtr<FJsonValue>> RendArray;
        for (int32 ri = 0; ri < EmitterData->GetRenderers().Num(); ri++)
        {
            UNiagaraRendererProperties* Rend = EmitterData->GetRenderers()[ri];
            TSharedPtr<FJsonObject> RObj = MakeShared<FJsonObject>();
            RObj->SetNumberField(TEXT("index"), ri);
            RObj->SetStringField(TEXT("type"), RendererTypeToString(Rend));
            RObj->SetStringField(TEXT("class"), Rend->GetClass()->GetName());
            RObj->SetBoolField(TEXT("enabled"), Rend->GetIsEnabled());
            RendArray.Add(MakeShared<FJsonValueObject>(RObj));
        }
        EmObj->SetArrayField(TEXT("renderers"), RendArray);

        // Modules (from the graph)
        TArray<TSharedPtr<FJsonValue>> ModulesArray;
        if (UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(EmitterData->GraphSource))
        {
            if (UNiagaraGraph* Graph = Source->NodeGraph)
            {
                TArray<UNiagaraNodeFunctionCall*> FuncNodes;
                Graph->GetNodesOfClass<UNiagaraNodeFunctionCall>(FuncNodes);
                for (UNiagaraNodeFunctionCall* FuncNode : FuncNodes)
                {
                    TSharedPtr<FJsonObject> MObj = MakeShared<FJsonObject>();
                    MObj->SetStringField(TEXT("name"), FuncNode->GetFunctionName());
                    MObj->SetStringField(TEXT("class"), FuncNode->GetClass()->GetName());
                    MObj->SetBoolField(TEXT("enabled"), FuncNode->IsNodeEnabled());
                    if (FuncNode->FunctionScript)
                    {
                        MObj->SetStringField(TEXT("script"), FuncNode->FunctionScript->GetPathName());
                    }
                    ModulesArray.Add(MakeShared<FJsonValueObject>(MObj));
                }
            }
        }
        EmObj->SetArrayField(TEXT("modules"), ModulesArray);

        // Simulation Stages
        TArray<TSharedPtr<FJsonValue>> StagesArray;
        for (UNiagaraSimulationStageBase* Stage : EmitterData->GetSimulationStages())
        {
            if (!Stage) continue;
            TSharedPtr<FJsonObject> SObj = MakeShared<FJsonObject>();
            SObj->SetStringField(TEXT("name"), Stage->SimulationStageName.ToString());
            SObj->SetStringField(TEXT("class"), Stage->GetClass()->GetName());
            SObj->SetBoolField(TEXT("enabled"), Stage->bEnabled);
            if (UNiagaraSimulationStageGeneric* GenStage = Cast<UNiagaraSimulationStageGeneric>(Stage))
            {
                SObj->SetStringField(TEXT("iteration_source"),
                    GenStage->IterationSource == ENiagaraIterationSource::Particles ? TEXT("Particles") : TEXT("DataInterface"));
            }
            StagesArray.Add(MakeShared<FJsonValueObject>(SObj));
        }
        EmObj->SetArrayField(TEXT("simulation_stages"), StagesArray);

        // Event Handlers
        TArray<TSharedPtr<FJsonValue>> EventsArray;
        for (const FNiagaraEventScriptProperties& EventProps : EmitterData->GetEventHandlers())
        {
            TSharedPtr<FJsonObject> EObj = MakeShared<FJsonObject>();
            EObj->SetStringField(TEXT("source_event_name"), EventProps.SourceEventName.ToString());
            EObj->SetStringField(TEXT("source_emitter_id"), EventProps.SourceEmitterID.ToString());
            EObj->SetNumberField(TEXT("max_events_per_frame"), EventProps.MaxEventsPerFrame);
            EObj->SetStringField(TEXT("execution_mode"),
                EventProps.ExecutionMode == EScriptExecutionMode::EveryParticle ? TEXT("EveryParticle") :
                EventProps.ExecutionMode == EScriptExecutionMode::SpawnedParticles ? TEXT("SpawnedParticles") : TEXT("SingleParticle"));
            if (EventProps.Script)
                EObj->SetStringField(TEXT("usage_id"), EventProps.Script->GetUsageId().ToString());
            EventsArray.Add(MakeShared<FJsonValueObject>(EObj));
        }
        EmObj->SetArrayField(TEXT("event_handlers"), EventsArray);

        // Data Interfaces (from all scripts)
        TArray<TSharedPtr<FJsonValue>> DIsArray;
        TSet<FString> SeenDIs;
        TArray<UNiagaraScript*> Scripts;
        EmitterData->GetScripts(Scripts, false, false);
        for (UNiagaraScript* Script : Scripts)
        {
            if (!Script) continue;
            for (const FNiagaraScriptDataInterfaceInfo& DIInfo : Script->GetCachedDefaultDataInterfaces())
            {
                if (!DIInfo.DataInterface) continue;
                FString DIName = DIInfo.Name.ToString();
                if (SeenDIs.Contains(DIName)) continue;
                SeenDIs.Add(DIName);

                TSharedPtr<FJsonObject> DObj = MakeShared<FJsonObject>();
                DObj->SetStringField(TEXT("name"), DIName);
                DObj->SetStringField(TEXT("class"), DIInfo.DataInterface->GetClass()->GetName());
                DIsArray.Add(MakeShared<FJsonValueObject>(DObj));
            }
        }
        EmObj->SetArrayField(TEXT("data_interfaces"), DIsArray);

        EmitterArray.Add(MakeShared<FJsonValueObject>(EmObj));
    }

    Result->SetArrayField(TEXT("emitters"), EmitterArray);
    return Result;
}

// --- Helper: serialize FRichCurve keys to JSON array ---
static TArray<TSharedPtr<FJsonValue>> SerializeRichCurveKeys(const FRichCurve& Curve)
{
    TArray<TSharedPtr<FJsonValue>> KeysArr;
    for (auto It = Curve.GetKeyIterator(); It; ++It)
    {
        TSharedPtr<FJsonObject> KObj = MakeShared<FJsonObject>();
        KObj->SetNumberField(TEXT("time"), It->Time);
        KObj->SetNumberField(TEXT("value"), It->Value);
        FString InterpStr;
        switch (It->InterpMode)
        {
            case RCIM_Linear: InterpStr = TEXT("Linear"); break;
            case RCIM_Constant: InterpStr = TEXT("Constant"); break;
            case RCIM_Cubic: InterpStr = TEXT("Cubic"); break;
            default: InterpStr = TEXT("Unknown"); break;
        }
        KObj->SetStringField(TEXT("interp"), InterpStr);
        KeysArr.Add(MakeShared<FJsonValueObject>(KObj));
    }
    return KeysArr;
}

// --- Helper: find a DI by name from an emitter's scripts ---
static UNiagaraDataInterface* FindDIByName(FVersionedNiagaraEmitterData* EmitterData, const FString& DIName)
{
    if (!EmitterData) return nullptr;
    TArray<UNiagaraScript*> Scripts;
    EmitterData->GetScripts(Scripts, false, false);
    for (UNiagaraScript* Script : Scripts)
    {
        if (!Script) continue;
        for (const FNiagaraScriptDataInterfaceInfo& Info : Script->GetCachedDefaultDataInterfaces())
        {
            if (Info.DataInterface && Info.Name.ToString() == DIName)
                return Info.DataInterface;
        }
    }
    return nullptr;
}

// --- get_niagara_curve_data ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetNiagaraCurveData(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString DIName;
    if (!Params->TryGetStringField(TEXT("di_name"), DIName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'di_name'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* EmitterData = Handles[EmitterIndex].GetInstance().GetEmitterData();
    UNiagaraDataInterface* DI = FindDIByName(EmitterData, DIName);
    if (!DI)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Data interface '%s' not found on emitter %d"), *DIName, EmitterIndex));

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("di_name"), DIName);
    Result->SetStringField(TEXT("di_class"), DI->GetClass()->GetName());

    if (UNiagaraDataInterfaceCurve* CurveDI = Cast<UNiagaraDataInterfaceCurve>(DI))
    {
        Result->SetArrayField(TEXT("curve"), SerializeRichCurveKeys(CurveDI->Curve));
    }
    else if (UNiagaraDataInterfaceVectorCurve* VecDI = Cast<UNiagaraDataInterfaceVectorCurve>(DI))
    {
        Result->SetArrayField(TEXT("x_curve"), SerializeRichCurveKeys(VecDI->XCurve));
        Result->SetArrayField(TEXT("y_curve"), SerializeRichCurveKeys(VecDI->YCurve));
        Result->SetArrayField(TEXT("z_curve"), SerializeRichCurveKeys(VecDI->ZCurve));
    }
    else if (UNiagaraDataInterfaceColorCurve* ColDI = Cast<UNiagaraDataInterfaceColorCurve>(DI))
    {
        Result->SetArrayField(TEXT("red_curve"), SerializeRichCurveKeys(ColDI->RedCurve));
        Result->SetArrayField(TEXT("green_curve"), SerializeRichCurveKeys(ColDI->GreenCurve));
        Result->SetArrayField(TEXT("blue_curve"), SerializeRichCurveKeys(ColDI->BlueCurve));
        Result->SetArrayField(TEXT("alpha_curve"), SerializeRichCurveKeys(ColDI->AlphaCurve));
    }
    else if (UNiagaraDataInterfaceVector2DCurve* Vec2DI = Cast<UNiagaraDataInterfaceVector2DCurve>(DI))
    {
        Result->SetArrayField(TEXT("x_curve"), SerializeRichCurveKeys(Vec2DI->XCurve));
        Result->SetArrayField(TEXT("y_curve"), SerializeRichCurveKeys(Vec2DI->YCurve));
    }
    else
    {
        Result->SetStringField(TEXT("warning"), TEXT("Not a curve-type DI, no key data available"));
    }

    return Result;
}

// --- set_niagara_curve_data ---

static void ParseAndSetCurveKeys(FRichCurve& Curve, const TArray<TSharedPtr<FJsonValue>>& KeysJson)
{
    Curve.Reset();
    for (const TSharedPtr<FJsonValue>& KeyVal : KeysJson)
    {
        const TSharedPtr<FJsonObject>& KeyObj = KeyVal->AsObject();
        if (!KeyObj.IsValid()) continue;
        double Time = 0.0, Value = 0.0;
        KeyObj->TryGetNumberField(TEXT("time"), Time);
        KeyObj->TryGetNumberField(TEXT("value"), Value);
        FKeyHandle Handle = Curve.AddKey((float)Time, (float)Value);

        FString Interp;
        if (KeyObj->TryGetStringField(TEXT("interp"), Interp))
        {
            ERichCurveInterpMode Mode = RCIM_Cubic;
            if (Interp == TEXT("Linear")) Mode = RCIM_Linear;
            else if (Interp == TEXT("Constant")) Mode = RCIM_Constant;
            Curve.SetKeyInterpMode(Handle, Mode);
        }
    }
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraCurveData(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString DIName;
    if (!Params->TryGetStringField(TEXT("di_name"), DIName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'di_name'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* EmitterData = Handles[EmitterIndex].GetInstance().GetEmitterData();
    UNiagaraDataInterface* DI = FindDIByName(EmitterData, DIName);
    if (!DI)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Data interface '%s' not found"), *DIName));

    DI->Modify();
    int32 KeysSet = 0;

    if (UNiagaraDataInterfaceCurve* CurveDI = Cast<UNiagaraDataInterfaceCurve>(DI))
    {
        const TArray<TSharedPtr<FJsonValue>>* Keys = nullptr;
        if (Params->TryGetArrayField(TEXT("curve"), Keys))
        {
            ParseAndSetCurveKeys(CurveDI->Curve, *Keys);
            KeysSet = Keys->Num();
        }
    }
    else if (UNiagaraDataInterfaceVectorCurve* VecDI = Cast<UNiagaraDataInterfaceVectorCurve>(DI))
    {
        const TArray<TSharedPtr<FJsonValue>>* Keys = nullptr;
        if (Params->TryGetArrayField(TEXT("x_curve"), Keys)) { ParseAndSetCurveKeys(VecDI->XCurve, *Keys); KeysSet += Keys->Num(); }
        if (Params->TryGetArrayField(TEXT("y_curve"), Keys)) { ParseAndSetCurveKeys(VecDI->YCurve, *Keys); KeysSet += Keys->Num(); }
        if (Params->TryGetArrayField(TEXT("z_curve"), Keys)) { ParseAndSetCurveKeys(VecDI->ZCurve, *Keys); KeysSet += Keys->Num(); }
    }
    else if (UNiagaraDataInterfaceColorCurve* ColDI = Cast<UNiagaraDataInterfaceColorCurve>(DI))
    {
        const TArray<TSharedPtr<FJsonValue>>* Keys = nullptr;
        if (Params->TryGetArrayField(TEXT("red_curve"), Keys)) { ParseAndSetCurveKeys(ColDI->RedCurve, *Keys); KeysSet += Keys->Num(); }
        if (Params->TryGetArrayField(TEXT("green_curve"), Keys)) { ParseAndSetCurveKeys(ColDI->GreenCurve, *Keys); KeysSet += Keys->Num(); }
        if (Params->TryGetArrayField(TEXT("blue_curve"), Keys)) { ParseAndSetCurveKeys(ColDI->BlueCurve, *Keys); KeysSet += Keys->Num(); }
        if (Params->TryGetArrayField(TEXT("alpha_curve"), Keys)) { ParseAndSetCurveKeys(ColDI->AlphaCurve, *Keys); KeysSet += Keys->Num(); }
    }
    else if (UNiagaraDataInterfaceVector2DCurve* Vec2DI = Cast<UNiagaraDataInterfaceVector2DCurve>(DI))
    {
        const TArray<TSharedPtr<FJsonValue>>* Keys = nullptr;
        if (Params->TryGetArrayField(TEXT("x_curve"), Keys)) { ParseAndSetCurveKeys(Vec2DI->XCurve, *Keys); KeysSet += Keys->Num(); }
        if (Params->TryGetArrayField(TEXT("y_curve"), Keys)) { ParseAndSetCurveKeys(Vec2DI->YCurve, *Keys); KeysSet += Keys->Num(); }
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Not a supported curve-type DI"));
    }

    System->RequestCompile(false);
    UEditorAssetLibrary::SaveAsset(System->GetPathName(), false);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("di_name"), DIName);
    Result->SetNumberField(TEXT("keys_set"), KeysSet);
    return Result;
}

// --- get_niagara_di_properties: generic reflection-based DI property reader ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetNiagaraDIProperties(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString DIName;
    if (!Params->TryGetStringField(TEXT("di_name"), DIName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'di_name'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* EmitterData = Handles[EmitterIndex].GetInstance().GetEmitterData();
    UNiagaraDataInterface* DI = FindDIByName(EmitterData, DIName);
    if (!DI)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Data interface '%s' not found"), *DIName));

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("di_name"), DIName);
    Result->SetStringField(TEXT("di_class"), DI->GetClass()->GetName());

    TArray<TSharedPtr<FJsonValue>> PropsArray;
    for (TFieldIterator<FProperty> PropIt(DI->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Prop = *PropIt;
        if (!Prop->HasAnyPropertyFlags(CPF_Edit)) continue;

        TSharedPtr<FJsonObject> PObj = MakeShared<FJsonObject>();
        PObj->SetStringField(TEXT("name"), Prop->GetName());
        PObj->SetStringField(TEXT("type"), Prop->GetCPPType());
        PObj->SetStringField(TEXT("category"), Prop->GetMetaData(TEXT("Category")));

        FString ValueStr;
        const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(DI);
        Prop->ExportTextItem_Direct(ValueStr, ValuePtr, nullptr, DI, PPF_None);
        PObj->SetStringField(TEXT("value"), ValueStr);

        PropsArray.Add(MakeShared<FJsonValueObject>(PObj));
    }
    Result->SetArrayField(TEXT("properties"), PropsArray);

    return Result;
}

// --- set_niagara_di_property: generic reflection-based DI property writer ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraDIProperty(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString DIName;
    if (!Params->TryGetStringField(TEXT("di_name"), DIName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'di_name'"));

    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name'"));

    FString PropertyValue;
    if (!Params->TryGetStringField(TEXT("property_value"), PropertyValue))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* EmitterData = Handles[EmitterIndex].GetInstance().GetEmitterData();
    UNiagaraDataInterface* DI = FindDIByName(EmitterData, DIName);
    if (!DI)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Data interface '%s' not found"), *DIName));

    FProperty* Prop = DI->GetClass()->FindPropertyByName(*PropertyName);
    if (!Prop)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Property '%s' not found on %s"), *PropertyName, *DI->GetClass()->GetName()));

    DI->Modify();
    void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(DI);

    // Handle object references (material paths etc.)
    FString InputText = PropertyValue;
    if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
    {
        if (!InputText.Contains(TEXT("'")) && (InputText.StartsWith(TEXT("/Game")) || InputText.StartsWith(TEXT("/Engine"))))
        {
            UObject* Loaded = LoadObject<UObject>(nullptr, *InputText);
            if (Loaded)
                InputText = FString::Printf(TEXT("%s'%s'"), *Loaded->GetClass()->GetName(), *Loaded->GetPathName());
        }
    }

    const TCHAR* ImportResult = Prop->ImportText_Direct(*InputText, ValuePtr, DI, PPF_None);
    if (!ImportResult)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to set '%s' to '%s'"), *PropertyName, *PropertyValue));

    System->RequestCompile(false);
    UEditorAssetLibrary::SaveAsset(System->GetPathName(), false);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("di_name"), DIName);
    Result->SetStringField(TEXT("property"), PropertyName);
    Result->SetStringField(TEXT("value"), PropertyValue);
    return Result;
}

// --- Helper: get ScriptSource and Graph from emitter ---
static bool GetEmitterSourceAndGraph(FVersionedNiagaraEmitterData* EmitterData, UNiagaraScriptSource*& OutSource, UNiagaraGraph*& OutGraph)
{
    OutSource = Cast<UNiagaraScriptSource>(EmitterData->GraphSource);
    if (!OutSource) return false;
    OutGraph = OutSource->NodeGraph;
    return OutGraph != nullptr;
}

// --- Helper: create output+input nodes for a new script usage (replicates ResetGraphForOutput) ---
static void SetupGraphForNewScript(UNiagaraGraph& Graph, ENiagaraScriptUsage Usage, FGuid UsageId)
{
    Graph.Modify();

    FGraphNodeCreator<UNiagaraNodeOutput> OutputCreator(Graph);
    UNiagaraNodeOutput* OutputNode = OutputCreator.CreateNode();
    OutputNode->SetUsage(Usage);
    OutputNode->SetUsageId(UsageId);
    OutputNode->Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetParameterMapDef(), TEXT("Out")));
    OutputCreator.Finalize();

    FGraphNodeCreator<UNiagaraNodeInput> InputCreator(Graph);
    UNiagaraNodeInput* InputNode = InputCreator.CreateNode();
    InputNode->Input = FNiagaraVariable(FNiagaraTypeDefinition::GetParameterMapDef(), TEXT("InputMap"));
    InputNode->Usage = ENiagaraInputNodeUsage::Parameter;
    InputCreator.Finalize();

    UEdGraphPin* OutInputPin = OutputNode->Pins.Num() > 0 ? OutputNode->Pins[0] : nullptr;
    UEdGraphPin* InOutputPin = InputNode->Pins.Num() > 0 ? InputNode->Pins[0] : nullptr;
    if (OutInputPin && InOutputPin)
    {
        OutInputPin->BreakAllPinLinks();
        OutInputPin->MakeLinkTo(InOutputPin);
    }
}

// --- add_niagara_simulation_stage ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddNiagaraSimulationStage(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString StageName = TEXT("NewSimStage");
    Params->TryGetStringField(TEXT("stage_name"), StageName);

    FString IterSource = TEXT("Particles");
    Params->TryGetStringField(TEXT("iteration_source"), IterSource);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitter VerEmitter = Handles[EmitterIndex].GetInstance();
    FVersionedNiagaraEmitterData* EmitterData = VerEmitter.GetEmitterData();
    UNiagaraEmitter* Emitter = VerEmitter.Emitter;
    if (!Emitter || !EmitterData)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter"));

    UNiagaraScriptSource* Source = nullptr;
    UNiagaraGraph* Graph = nullptr;
    if (!GetEmitterSourceAndGraph(EmitterData, Source, Graph))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No script source/graph on emitter"));

    Emitter->Modify();

    FGuid StageUsageId = FGuid::NewGuid();

    UNiagaraSimulationStageGeneric* NewStage = NewObject<UNiagaraSimulationStageGeneric>(
        Emitter, NAME_None, RF_Transactional);
    NewStage->SimulationStageName = *StageName;
    NewStage->bEnabled = true;
    if (IterSource == TEXT("DataInterface"))
        NewStage->IterationSource = ENiagaraIterationSource::DataInterface;
    else
        NewStage->IterationSource = ENiagaraIterationSource::Particles;

    NewStage->Script = NewObject<UNiagaraScript>(
        NewStage,
        MakeUniqueObjectName(NewStage, UNiagaraScript::StaticClass(), TEXT("SimulationStage")),
        RF_Transactional);
    NewStage->Script->SetUsage(ENiagaraScriptUsage::ParticleSimulationStageScript);
    NewStage->Script->SetUsageId(StageUsageId);
    NewStage->Script->SetLatestSource(Source);

    Emitter->AddSimulationStage(NewStage, VerEmitter.Version);
    SetupGraphForNewScript(*Graph, ENiagaraScriptUsage::ParticleSimulationStageScript, StageUsageId);

    System->RequestCompile(false);
    UEditorAssetLibrary::SaveAsset(System->GetPathName(), false);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("stage_name"), StageName);
    Result->SetStringField(TEXT("iteration_source"), IterSource);
    Result->SetNumberField(TEXT("emitter_index"), EmitterIndex);
    return Result;
}

// --- remove_niagara_simulation_stage ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleRemoveNiagaraSimulationStage(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString StageName;
    if (!Params->TryGetStringField(TEXT("stage_name"), StageName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'stage_name'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitter VerEmitter = Handles[EmitterIndex].GetInstance();
    FVersionedNiagaraEmitterData* EmitterData = VerEmitter.GetEmitterData();
    UNiagaraEmitter* Emitter = VerEmitter.Emitter;
    if (!Emitter || !EmitterData)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    UNiagaraSimulationStageBase* FoundStage = nullptr;
    for (UNiagaraSimulationStageBase* Stage : EmitterData->GetSimulationStages())
    {
        if (Stage && Stage->SimulationStageName.ToString() == StageName)
        {
            FoundStage = Stage;
            break;
        }
    }
    if (!FoundStage)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Simulation stage '%s' not found"), *StageName));

    Emitter->RemoveSimulationStage(FoundStage, VerEmitter.Version);
    System->RequestCompile(false);
    UEditorAssetLibrary::SaveAsset(System->GetPathName(), false);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("removed_stage"), StageName);
    return Result;
}

// --- get_niagara_sim_stage_properties: reflection-based ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetNiagaraSimStageProperties(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString StageName;
    if (!Params->TryGetStringField(TEXT("stage_name"), StageName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'stage_name'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* EmitterData = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!EmitterData)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    UNiagaraSimulationStageBase* FoundStage = nullptr;
    for (UNiagaraSimulationStageBase* Stage : EmitterData->GetSimulationStages())
    {
        if (Stage && Stage->SimulationStageName.ToString() == StageName)
        {
            FoundStage = Stage;
            break;
        }
    }
    if (!FoundStage)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Simulation stage '%s' not found"), *StageName));

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("stage_name"), StageName);
    Result->SetStringField(TEXT("class"), FoundStage->GetClass()->GetName());

    TArray<TSharedPtr<FJsonValue>> PropsArray;
    for (TFieldIterator<FProperty> PropIt(FoundStage->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Prop = *PropIt;
        if (!Prop->HasAnyPropertyFlags(CPF_Edit)) continue;

        TSharedPtr<FJsonObject> PObj = MakeShared<FJsonObject>();
        PObj->SetStringField(TEXT("name"), Prop->GetName());
        PObj->SetStringField(TEXT("type"), Prop->GetCPPType());

        FString ValueStr;
        const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(FoundStage);
        Prop->ExportTextItem_Direct(ValueStr, ValuePtr, nullptr, FoundStage, PPF_None);
        PObj->SetStringField(TEXT("value"), ValueStr);

        PropsArray.Add(MakeShared<FJsonValueObject>(PObj));
    }
    Result->SetArrayField(TEXT("properties"), PropsArray);
    return Result;
}

// --- set_niagara_sim_stage_property: reflection-based ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraSimStageProperty(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString StageName;
    if (!Params->TryGetStringField(TEXT("stage_name"), StageName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'stage_name'"));

    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name'"));

    FString PropertyValue;
    if (!Params->TryGetStringField(TEXT("property_value"), PropertyValue))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* EmitterData = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!EmitterData)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    UNiagaraSimulationStageBase* FoundStage = nullptr;
    for (UNiagaraSimulationStageBase* Stage : EmitterData->GetSimulationStages())
    {
        if (Stage && Stage->SimulationStageName.ToString() == StageName)
        {
            FoundStage = Stage;
            break;
        }
    }
    if (!FoundStage)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Simulation stage '%s' not found"), *StageName));

    FProperty* Prop = FoundStage->GetClass()->FindPropertyByName(*PropertyName);
    if (!Prop)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Property '%s' not found"), *PropertyName));

    FoundStage->Modify();
    void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(FoundStage);
    const TCHAR* ImportResult = Prop->ImportText_Direct(*PropertyValue, ValuePtr, FoundStage, PPF_None);
    if (!ImportResult)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to set '%s' to '%s'"), *PropertyName, *PropertyValue));

    System->RequestCompile(false);
    UEditorAssetLibrary::SaveAsset(System->GetPathName(), false);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("stage_name"), StageName);
    Result->SetStringField(TEXT("property"), PropertyName);
    Result->SetStringField(TEXT("value"), PropertyValue);
    return Result;
}

// --- add_niagara_event_handler ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddNiagaraEventHandler(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString SourceEventName;
    if (!Params->TryGetStringField(TEXT("source_event_name"), SourceEventName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_event_name'"));

    int32 SourceEmitterIndex = -1;
    Params->TryGetNumberField(TEXT("source_emitter_index"), SourceEmitterIndex);

    FString ExecMode = TEXT("EveryParticle");
    Params->TryGetStringField(TEXT("execution_mode"), ExecMode);

    int32 MaxEvents = 0;
    Params->TryGetNumberField(TEXT("max_events_per_frame"), MaxEvents);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitter VerEmitter = Handles[EmitterIndex].GetInstance();
    FVersionedNiagaraEmitterData* EmitterData = VerEmitter.GetEmitterData();
    UNiagaraEmitter* Emitter = VerEmitter.Emitter;
    if (!Emitter || !EmitterData)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter"));

    UNiagaraScriptSource* Source = nullptr;
    UNiagaraGraph* Graph = nullptr;
    if (!GetEmitterSourceAndGraph(EmitterData, Source, Graph))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No script source/graph on emitter"));

    Emitter->Modify();

    FNiagaraEventScriptProperties EventProps;
    EventProps.Script = NewObject<UNiagaraScript>(
        Emitter,
        MakeUniqueObjectName(Emitter, UNiagaraScript::StaticClass(), TEXT("EventScript")),
        RF_Transactional);
    EventProps.Script->SetUsage(ENiagaraScriptUsage::ParticleEventScript);
    EventProps.Script->SetUsageId(FGuid::NewGuid());
    EventProps.Script->SetLatestSource(Source);

    EventProps.SourceEventName = *SourceEventName;
    EventProps.MaxEventsPerFrame = MaxEvents;

    if (ExecMode == TEXT("SpawnedParticles"))
        EventProps.ExecutionMode = EScriptExecutionMode::SpawnedParticles;
    else if (ExecMode == TEXT("SingleParticle"))
        EventProps.ExecutionMode = EScriptExecutionMode::SingleParticle;
    else
        EventProps.ExecutionMode = EScriptExecutionMode::EveryParticle;

    if (SourceEmitterIndex >= 0 && Handles.IsValidIndex(SourceEmitterIndex))
        EventProps.SourceEmitterID = Handles[SourceEmitterIndex].GetId();

    Emitter->AddEventHandler(EventProps, VerEmitter.Version);
    SetupGraphForNewScript(*Graph, ENiagaraScriptUsage::ParticleEventScript, EventProps.Script->GetUsageId());

    System->RequestCompile(false);
    UEditorAssetLibrary::SaveAsset(System->GetPathName(), false);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("source_event_name"), SourceEventName);
    Result->SetStringField(TEXT("execution_mode"), ExecMode);
    Result->SetNumberField(TEXT("emitter_index"), EmitterIndex);
    return Result;
}

// --- set_niagara_emitter_sim_target ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraEmitterSimTarget(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString Target;
    if (!Params->TryGetStringField(TEXT("sim_target"), Target))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'sim_target' (CPU or GPU)"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* EmitterData = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!EmitterData)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    ENiagaraSimTarget NewTarget;
    if (Target == TEXT("GPU") || Target == TEXT("GPUComputeSim"))
        NewTarget = ENiagaraSimTarget::GPUComputeSim;
    else
        NewTarget = ENiagaraSimTarget::CPUSim;

    EmitterData->SimTarget = NewTarget;

    System->RequestCompile(false);
    UEditorAssetLibrary::SaveAsset(System->GetPathName(), false);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("sim_target"), Target);
    Result->SetNumberField(TEXT("emitter_index"), EmitterIndex);
    return Result;
}

// --- remove_niagara_event_handler ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleRemoveNiagaraEventHandler(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString UsageIdStr;
    if (!Params->TryGetStringField(TEXT("usage_id"), UsageIdStr))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'usage_id' - get it from get_niagara_deep_inspect event_handlers[].usage_id"));

    FGuid UsageId;
    if (!FGuid::Parse(UsageIdStr, UsageId))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid 'usage_id' format"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitter VerEmitter = Handles[EmitterIndex].GetInstance();
    UNiagaraEmitter* Emitter = VerEmitter.Emitter;
    if (!Emitter)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter"));

    Emitter->Modify();
    Emitter->RemoveEventHandlerByUsageId(UsageId, VerEmitter.Version);

    System->RequestCompile(false);
    UEditorAssetLibrary::SaveAsset(System->GetPathName(), false);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("removed_usage_id"), UsageIdStr);
    return Result;
}

// --- add_niagara_emitter: add emitter from template or existing asset ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddNiagaraEmitter(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    FString EmitterPath;
    if (!Params->TryGetStringField(TEXT("emitter_path"), EmitterPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'emitter_path' - path to source emitter asset"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    UNiagaraEmitter* SourceEmitter = LoadObject<UNiagaraEmitter>(nullptr, *EmitterPath);
    if (!SourceEmitter)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Emitter not found: %s"), *EmitterPath));

    FGuid EmitterVersion = SourceEmitter->GetExposedVersion().VersionGuid;
    FGuid NewHandleId = FNiagaraEditorUtilities::AddEmitterToSystem(*System, *SourceEmitter, EmitterVersion);

    if (!NewHandleId.IsValid())
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("AddEmitterToSystem returned invalid handle"));

    System->RequestCompile(false);
    UEditorAssetLibrary::SaveAsset(System->GetPathName(), false);

    // Find the new emitter's name
    FString NewEmitterName = TEXT("Unknown");
    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    for (const FNiagaraEmitterHandle& Handle : Handles)
    {
        if (Handle.GetId() == NewHandleId)
        {
            NewEmitterName = Handle.GetName().ToString();
            break;
        }
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("emitter_name"), NewEmitterName);
    Result->SetStringField(TEXT("source_emitter"), EmitterPath);
    Result->SetNumberField(TEXT("emitter_count"), Handles.Num());
    Result->SetStringField(TEXT("handle_id"), NewHandleId.ToString());
    return Result;
}

// --- set_niagara_event_handler_property: modify existing event handler ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraEventHandlerProperty(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString UsageIdStr;
    if (!Params->TryGetStringField(TEXT("usage_id"), UsageIdStr))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'usage_id'"));

    FGuid UsageId;
    if (!FGuid::Parse(UsageIdStr, UsageId))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid 'usage_id'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitter VerEmitter = Handles[EmitterIndex].GetInstance();
    FVersionedNiagaraEmitterData* EmitterData = VerEmitter.GetEmitterData();
    if (!EmitterData)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    FNiagaraEventScriptProperties* EventProps = EmitterData->GetEventHandlerByIdUnsafe(UsageId);
    if (!EventProps)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Event handler not found"));

    FString Value;
    if (Params->TryGetStringField(TEXT("source_event_name"), Value))
        EventProps->SourceEventName = *Value;

    if (Params->TryGetStringField(TEXT("execution_mode"), Value))
    {
        if (Value == TEXT("SpawnedParticles"))
            EventProps->ExecutionMode = EScriptExecutionMode::SpawnedParticles;
        else if (Value == TEXT("SingleParticle"))
            EventProps->ExecutionMode = EScriptExecutionMode::SingleParticle;
        else
            EventProps->ExecutionMode = EScriptExecutionMode::EveryParticle;
    }

    int32 IntVal;
    if (Params->TryGetNumberField(TEXT("max_events_per_frame"), IntVal))
        EventProps->MaxEventsPerFrame = IntVal;

    if (Params->TryGetNumberField(TEXT("spawn_number"), IntVal))
        EventProps->SpawnNumber = IntVal;

    int32 SourceEmitterIndex = -1;
    if (Params->TryGetNumberField(TEXT("source_emitter_index"), SourceEmitterIndex))
    {
        if (Handles.IsValidIndex(SourceEmitterIndex))
            EventProps->SourceEmitterID = Handles[SourceEmitterIndex].GetId();
    }

    System->RequestCompile(false);
    UEditorAssetLibrary::SaveAsset(System->GetPathName(), false);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("usage_id"), UsageIdStr);
    return Result;
}

// --- move_niagara_simulation_stage: reorder stage ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleMoveNiagaraSimulationStage(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString StageName;
    if (!Params->TryGetStringField(TEXT("stage_name"), StageName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'stage_name'"));

    int32 TargetIndex = 0;
    if (!Params->TryGetNumberField(TEXT("target_index"), TargetIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'target_index'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitter VerEmitter = Handles[EmitterIndex].GetInstance();
    FVersionedNiagaraEmitterData* EmitterData = VerEmitter.GetEmitterData();
    UNiagaraEmitter* Emitter = VerEmitter.Emitter;
    if (!Emitter || !EmitterData)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter"));

    UNiagaraSimulationStageBase* FoundStage = nullptr;
    for (UNiagaraSimulationStageBase* Stage : EmitterData->GetSimulationStages())
    {
        if (Stage && Stage->SimulationStageName.ToString() == StageName)
        {
            FoundStage = Stage;
            break;
        }
    }
    if (!FoundStage)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Simulation stage '%s' not found"), *StageName));

    Emitter->MoveSimulationStageToIndex(FoundStage, TargetIndex, VerEmitter.Version);

    System->RequestCompile(false);
    UEditorAssetLibrary::SaveAsset(System->GetPathName(), false);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("stage_name"), StageName);
    Result->SetNumberField(TEXT("target_index"), TargetIndex);
    return Result;
}

// --- get_niagara_scalability: read emitter scalability settings ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetNiagaraScalabilitySettings(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* EmitterData = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!EmitterData)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    const FNiagaraEmitterScalabilitySettings& Settings = EmitterData->GetScalabilitySettings();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetBoolField(TEXT("scale_spawn_count"), Settings.bScaleSpawnCount);
    Result->SetNumberField(TEXT("spawn_count_scale"), Settings.SpawnCountScale);

    // Overrides
    TArray<TSharedPtr<FJsonValue>> OverridesArr;
    for (const FNiagaraEmitterScalabilityOverride& Override : EmitterData->ScalabilityOverrides.Overrides)
    {
        TSharedPtr<FJsonObject> OObj = MakeShared<FJsonObject>();
        OObj->SetBoolField(TEXT("override_spawn_count_scale"), Override.bOverrideSpawnCountScale);
        OObj->SetBoolField(TEXT("scale_spawn_count"), Override.bScaleSpawnCount);
        OObj->SetNumberField(TEXT("spawn_count_scale"), Override.SpawnCountScale);
        OverridesArr.Add(MakeShared<FJsonValueObject>(OObj));
    }
    Result->SetArrayField(TEXT("overrides"), OverridesArr);

    return Result;
}

// --- set_niagara_scalability_override: set emitter spawn count scale ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraScalabilityOverride(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    double SpawnCountScale = 1.0;
    if (!Params->TryGetNumberField(TEXT("spawn_count_scale"), SpawnCountScale))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'spawn_count_scale'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    if (!Handles.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    FVersionedNiagaraEmitterData* EmitterData = Handles[EmitterIndex].GetInstance().GetEmitterData();
    if (!EmitterData)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

    if (EmitterData->ScalabilityOverrides.Overrides.Num() == 0)
    {
        FNiagaraEmitterScalabilityOverride NewOverride;
        NewOverride.bOverrideSpawnCountScale = true;
        NewOverride.bScaleSpawnCount = true;
        NewOverride.SpawnCountScale = (float)SpawnCountScale;
        EmitterData->ScalabilityOverrides.Overrides.Add(NewOverride);
    }
    else
    {
        FNiagaraEmitterScalabilityOverride& Override = EmitterData->ScalabilityOverrides.Overrides[0];
        Override.bOverrideSpawnCountScale = true;
        Override.bScaleSpawnCount = true;
        Override.SpawnCountScale = (float)SpawnCountScale;
    }

    System->RequestCompile(false);
    UEditorAssetLibrary::SaveAsset(System->GetPathName(), false);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetNumberField(TEXT("spawn_count_scale"), SpawnCountScale);
    return Result;
}

// --- get_niagara_data_channel_info: inspect a data channel asset ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetNiagaraDataChannelInfo(
    const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path'"));

    UObject* Asset = LoadObject<UObject>(nullptr, *AssetPath);
    if (!Asset)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Asset not found"));

    UNiagaraDataChannelAsset* DCAsset = Cast<UNiagaraDataChannelAsset>(Asset);
    if (!DCAsset)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Not a NiagaraDataChannelAsset"));

    UNiagaraDataChannel* DC = DCAsset->Get();
    if (!DC)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("DataChannel is null"));

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("name"), DCAsset->GetName());
    Result->SetStringField(TEXT("path"), DCAsset->GetPathName());
    Result->SetStringField(TEXT("channel_class"), DC->GetClass()->GetName());

    TArray<TSharedPtr<FJsonValue>> VarsArray;
    for (const FNiagaraDataChannelVariable& Var : DC->GetVariables())
    {
        TSharedPtr<FJsonObject> VObj = MakeShared<FJsonObject>();
        VObj->SetStringField(TEXT("name"), Var.GetName().ToString());
        VObj->SetStringField(TEXT("type"), Var.GetType().GetName());
        VarsArray.Add(MakeShared<FJsonValueObject>(VObj));
    }
    Result->SetArrayField(TEXT("variables"), VarsArray);
    Result->SetNumberField(TEXT("variable_count"), DC->GetVariables().Num());

    // Read editable properties via reflection
    TArray<TSharedPtr<FJsonValue>> PropsArray;
    for (TFieldIterator<FProperty> PropIt(DC->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Prop = *PropIt;
        if (!Prop->HasAnyPropertyFlags(CPF_Edit)) continue;
        if (Prop->GetName() == TEXT("ChannelVariables")) continue;

        TSharedPtr<FJsonObject> PObj = MakeShared<FJsonObject>();
        PObj->SetStringField(TEXT("name"), Prop->GetName());
        PObj->SetStringField(TEXT("type"), Prop->GetCPPType());
        FString ValueStr;
        const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(DC);
        Prop->ExportTextItem_Direct(ValueStr, ValuePtr, nullptr, DC, PPF_None);
        PObj->SetStringField(TEXT("value"), ValueStr);
        PropsArray.Add(MakeShared<FJsonValueObject>(PObj));
    }
    Result->SetArrayField(TEXT("properties"), PropsArray);

    return Result;
}

// --- get_niagara_scratch_pad_scripts: list scratch pad scripts in a system ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetNiagaraScratchPadScripts(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));

    TArray<TSharedPtr<FJsonValue>> AllScripts;

    // Helper: read HLSL from a scratch pad script via reflection
    auto ReadHlslFromScript = [](UNiagaraScript* Scr, TSharedPtr<FJsonObject>& Obj)
    {
        UNiagaraScriptSource* Src = Cast<UNiagaraScriptSource>(Scr->GetLatestSource());
        if (!Src || !Src->NodeGraph) return;
        TArray<UNiagaraNodeCustomHlsl*> HlslNodes;
        Src->NodeGraph->GetNodesOfClass<UNiagaraNodeCustomHlsl>(HlslNodes);
        for (UNiagaraNodeCustomHlsl* Node : HlslNodes)
        {
            FProperty* Prop = Node->GetClass()->FindPropertyByName(TEXT("CustomHlsl"));
            if (FStrProperty* SP = CastField<FStrProperty>(Prop))
            {
                Obj->SetStringField(TEXT("hlsl"), SP->GetPropertyValue(SP->ContainerPtrToValuePtr<void>(Node)));
                break;
            }
        }
    };

    // System-level scratch pad scripts
    for (UNiagaraScript* Script : System->ScratchPadScripts)
    {
        if (!Script) continue;
        TSharedPtr<FJsonObject> SObj = MakeShared<FJsonObject>();
        SObj->SetStringField(TEXT("name"), Script->GetName());
        SObj->SetStringField(TEXT("scope"), TEXT("System"));
        SObj->SetStringField(TEXT("usage"), StaticEnum<ENiagaraScriptUsage>()->GetNameStringByValue((int64)Script->GetUsage()));
        SObj->SetStringField(TEXT("path"), Script->GetPathName());
        ReadHlslFromScript(Script, SObj);
        AllScripts.Add(MakeShared<FJsonValueObject>(SObj));
    }

    // Per-emitter scratch pad scripts
    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    for (int32 i = 0; i < Handles.Num(); i++)
    {
        FVersionedNiagaraEmitterData* EmitterData = Handles[i].GetInstance().GetEmitterData();
        if (!EmitterData) continue;

#if WITH_EDITORONLY_DATA
        if (EmitterData->ScratchPads)
        {
            for (UNiagaraScript* Script : EmitterData->ScratchPads->Scripts)
            {
                if (!Script) continue;
                TSharedPtr<FJsonObject> SObj = MakeShared<FJsonObject>();
                SObj->SetStringField(TEXT("name"), Script->GetName());
                SObj->SetStringField(TEXT("scope"), TEXT("Emitter"));
                SObj->SetStringField(TEXT("emitter"), Handles[i].GetName().ToString());
                SObj->SetNumberField(TEXT("emitter_index"), i);
                SObj->SetStringField(TEXT("usage"), StaticEnum<ENiagaraScriptUsage>()->GetNameStringByValue((int64)Script->GetUsage()));
                SObj->SetStringField(TEXT("path"), Script->GetPathName());
                ReadHlslFromScript(Script, SObj);
                AllScripts.Add(MakeShared<FJsonValueObject>(SObj));
            }
        }
#endif
    }

    Result->SetArrayField(TEXT("scratch_pad_scripts"), AllScripts);
    Result->SetNumberField(TEXT("count"), AllScripts.Num());
    return Result;
}

// --- create_niagara_hlsl_module: create a module with custom HLSL ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleCreateNiagaraHlslModule(
    const TSharedPtr<FJsonObject>& Params)
{
    FString Name;
    if (!Params->TryGetStringField(TEXT("name"), Name))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name'"));

    FString Hlsl;
    if (!Params->TryGetStringField(TEXT("hlsl"), Hlsl))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'hlsl'"));

    FString PackagePath = TEXT("/Game/VFX/Modules");
    Params->TryGetStringField(TEXT("package_path"), PackagePath);

    // Create the module script asset via factory
    UNiagaraModuleScriptFactory* Factory = NewObject<UNiagaraModuleScriptFactory>();
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    UObject* Created = AssetTools.CreateAsset(Name, PackagePath, UNiagaraScript::StaticClass(), Factory);
    UNiagaraScript* Script = Cast<UNiagaraScript>(Created);
    if (!Script)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create module script asset"));

    UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Script->GetLatestSource());
    UNiagaraGraph* Graph = Source ? Source->NodeGraph : nullptr;
    if (!Graph)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("New module has no graph"));

    // Create the Custom HLSL node
    FGraphNodeCreator<UNiagaraNodeCustomHlsl> Creator(*Graph);
    UNiagaraNodeCustomHlsl* CustomNode = Creator.CreateNode();
    Creator.Finalize();

    // Set HLSL via reflection (SetCustomHlsl is not exported)
    FProperty* HlslProp = CustomNode->GetClass()->FindPropertyByName(TEXT("CustomHlsl"));
    if (HlslProp)
    {
        FStrProperty* StrProp = CastField<FStrProperty>(HlslProp);
        if (StrProp)
        {
            CustomNode->Modify();
            StrProp->SetPropertyValue(StrProp->ContainerPtrToValuePtr<void>(CustomNode), Hlsl);
        }
    }

    // Reconstruct to parse HLSL and auto-generate pins
    CustomNode->ReconstructNode();
    CustomNode->PostEditChange();
    Graph->NotifyGraphChanged();

    Script->MarkPackageDirty();
    UEditorAssetLibrary::SaveAsset(Script->GetPathName(), false);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetStringField(TEXT("name"), Name);
    Result->SetStringField(TEXT("path"), Script->GetPathName());
    Result->SetNumberField(TEXT("hlsl_length"), Hlsl.Len());
    return Result;
}

// --- Helper: serialize all editable UProperties on a UObject ---
static TArray<TSharedPtr<FJsonValue>> SerializeEditableProperties(UObject* Obj)
{
    TArray<TSharedPtr<FJsonValue>> Arr;
    if (!Obj) return Arr;
    for (TFieldIterator<FProperty> PropIt(Obj->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Prop = *PropIt;
        if (!Prop->HasAnyPropertyFlags(CPF_Edit)) continue;
        TSharedPtr<FJsonObject> PObj = MakeShared<FJsonObject>();
        PObj->SetStringField(TEXT("name"), Prop->GetName());
        PObj->SetStringField(TEXT("type"), Prop->GetCPPType());
        FString Val;
        const void* Ptr = Prop->ContainerPtrToValuePtr<void>(Obj);
        Prop->ExportTextItem_Direct(Val, Ptr, nullptr, Obj, PPF_None);
        PObj->SetStringField(TEXT("value"), Val);
        Arr.Add(MakeShared<FJsonValueObject>(PObj));
    }
    return Arr;
}

// --- export_niagara_system_spec: full system serialization ---

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleExportNiagaraSystemSpec(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    TSharedPtr<FJsonObject> Spec = MakeShared<FJsonObject>();
    Spec->SetStringField(TEXT("system_name"), System->GetName());
    Spec->SetStringField(TEXT("system_path"), System->GetPathName());
    Spec->SetNumberField(TEXT("warmup_time"), System->GetWarmupTime());

    // User parameters
    TArray<TSharedPtr<FJsonValue>> UserParamsArr;
    auto ExposedVars = System->GetExposedParameters().ReadParameterVariables();
    for (int32 vi = 0; vi < ExposedVars.Num(); vi++)
    {
        TSharedPtr<FJsonObject> PObj = MakeShared<FJsonObject>();
        PObj->SetStringField(TEXT("name"), ExposedVars[vi].GetName().ToString());
        PObj->SetStringField(TEXT("type"), ExposedVars[vi].GetType().GetName());
        UserParamsArr.Add(MakeShared<FJsonValueObject>(PObj));
    }
    Spec->SetArrayField(TEXT("user_parameters"), UserParamsArr);

    // Emitters
    TArray<TSharedPtr<FJsonValue>> EmittersArr;
    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();

    for (int32 i = 0; i < Handles.Num(); i++)
    {
        const FNiagaraEmitterHandle& Handle = Handles[i];
        FVersionedNiagaraEmitterData* EmData = Handle.GetInstance().GetEmitterData();
        if (!EmData) continue;

        TSharedPtr<FJsonObject> EmObj = MakeShared<FJsonObject>();
        EmObj->SetStringField(TEXT("name"), Handle.GetName().ToString());
        EmObj->SetNumberField(TEXT("index"), i);
        EmObj->SetBoolField(TEXT("enabled"), Handle.GetIsEnabled());
        EmObj->SetStringField(TEXT("sim_target"),
            EmData->SimTarget == ENiagaraSimTarget::CPUSim ? TEXT("CPU") : TEXT("GPU"));

        // Renderers with full properties
        TArray<TSharedPtr<FJsonValue>> RendArr;
        for (int32 ri = 0; ri < EmData->GetRenderers().Num(); ri++)
        {
            UNiagaraRendererProperties* Rend = EmData->GetRenderers()[ri];
            TSharedPtr<FJsonObject> RObj = MakeShared<FJsonObject>();
            RObj->SetStringField(TEXT("type"), RendererTypeToString(Rend));
            RObj->SetStringField(TEXT("class"), Rend->GetClass()->GetName());
            RObj->SetBoolField(TEXT("enabled"), Rend->GetIsEnabled());
            RObj->SetArrayField(TEXT("properties"), SerializeEditableProperties(Rend));
            RendArr.Add(MakeShared<FJsonValueObject>(RObj));
        }
        EmObj->SetArrayField(TEXT("renderers"), RendArr);

        // Modules with script paths
        TArray<TSharedPtr<FJsonValue>> ModArr;
        if (UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(EmData->GraphSource))
        {
            if (UNiagaraGraph* Graph = Source->NodeGraph)
            {
                TArray<UNiagaraNodeFunctionCall*> FuncNodes;
                Graph->GetNodesOfClass<UNiagaraNodeFunctionCall>(FuncNodes);
                for (UNiagaraNodeFunctionCall* FuncNode : FuncNodes)
                {
                    TSharedPtr<FJsonObject> MObj = MakeShared<FJsonObject>();
                    MObj->SetStringField(TEXT("name"), FuncNode->GetFunctionName());
                    MObj->SetBoolField(TEXT("enabled"), FuncNode->IsNodeEnabled());
                    if (FuncNode->FunctionScript)
                        MObj->SetStringField(TEXT("script"), FuncNode->FunctionScript->GetPathName());
                    ModArr.Add(MakeShared<FJsonValueObject>(MObj));
                }
            }
        }
        EmObj->SetArrayField(TEXT("modules"), ModArr);

        // Rapid iteration parameters (module input overrides)
        TArray<TSharedPtr<FJsonValue>> RapidArr;
        TArray<TPair<FString, UNiagaraScript*>> ScriptPairs;
        ScriptPairs.Add({TEXT("ParticleSpawn"), EmData->SpawnScriptProps.Script});
        ScriptPairs.Add({TEXT("ParticleUpdate"), EmData->UpdateScriptProps.Script});
        ScriptPairs.Add({TEXT("EmitterSpawn"), EmData->EmitterSpawnScriptProps.Script});
        ScriptPairs.Add({TEXT("EmitterUpdate"), EmData->EmitterUpdateScriptProps.Script});
        for (auto& Pair : ScriptPairs)
        {
            if (!Pair.Value) continue;
            auto RapidVars = Pair.Value->RapidIterationParameters.ReadParameterVariables();
            for (int32 vi = 0; vi < RapidVars.Num(); vi++)
            {
                const FNiagaraVariable& Var = RapidVars[vi];
                TSharedPtr<FJsonObject> RIObj = MakeShared<FJsonObject>();
                RIObj->SetStringField(TEXT("stage"), Pair.Key);
                RIObj->SetStringField(TEXT("name"), Var.GetName().ToString());
                RIObj->SetStringField(TEXT("type"), Var.GetType().GetName());
                if (Var.IsDataAllocated())
                {
                    FString ValStr;
                    if (Var.GetType() == FNiagaraTypeDefinition::GetFloatDef())
                        ValStr = FString::SanitizeFloat(Var.GetValue<float>());
                    else if (Var.GetType() == FNiagaraTypeDefinition::GetIntDef())
                        ValStr = FString::FromInt(Var.GetValue<int32>());
                    else if (Var.GetType() == FNiagaraTypeDefinition::GetBoolDef())
                        ValStr = Var.GetValue<FNiagaraBool>().GetValue() ? TEXT("true") : TEXT("false");
                    if (!ValStr.IsEmpty())
                        RIObj->SetStringField(TEXT("value"), ValStr);
                }
                RapidArr.Add(MakeShared<FJsonValueObject>(RIObj));
            }
        }
        EmObj->SetArrayField(TEXT("rapid_iteration_params"), RapidArr);

        // Data interfaces with curve data
        TArray<TSharedPtr<FJsonValue>> DIsArr;
        TSet<FString> SeenDIs;
        TArray<UNiagaraScript*> Scripts;
        EmData->GetScripts(Scripts, false, false);
        for (UNiagaraScript* Script : Scripts)
        {
            if (!Script) continue;
            for (const FNiagaraScriptDataInterfaceInfo& Info : Script->GetCachedDefaultDataInterfaces())
            {
                if (!Info.DataInterface) continue;
                FString DIName = Info.Name.ToString();
                if (SeenDIs.Contains(DIName)) continue;
                SeenDIs.Add(DIName);

                TSharedPtr<FJsonObject> DObj = MakeShared<FJsonObject>();
                DObj->SetStringField(TEXT("name"), DIName);
                DObj->SetStringField(TEXT("class"), Info.DataInterface->GetClass()->GetName());

                // Serialize curve keys for curve DIs
                if (UNiagaraDataInterfaceCurve* C = Cast<UNiagaraDataInterfaceCurve>(Info.DataInterface))
                    DObj->SetArrayField(TEXT("curve"), SerializeRichCurveKeys(C->Curve));
                else if (UNiagaraDataInterfaceVectorCurve* VC = Cast<UNiagaraDataInterfaceVectorCurve>(Info.DataInterface))
                {
                    DObj->SetArrayField(TEXT("x_curve"), SerializeRichCurveKeys(VC->XCurve));
                    DObj->SetArrayField(TEXT("y_curve"), SerializeRichCurveKeys(VC->YCurve));
                    DObj->SetArrayField(TEXT("z_curve"), SerializeRichCurveKeys(VC->ZCurve));
                }
                else if (UNiagaraDataInterfaceColorCurve* CC = Cast<UNiagaraDataInterfaceColorCurve>(Info.DataInterface))
                {
                    DObj->SetArrayField(TEXT("red_curve"), SerializeRichCurveKeys(CC->RedCurve));
                    DObj->SetArrayField(TEXT("green_curve"), SerializeRichCurveKeys(CC->GreenCurve));
                    DObj->SetArrayField(TEXT("blue_curve"), SerializeRichCurveKeys(CC->BlueCurve));
                    DObj->SetArrayField(TEXT("alpha_curve"), SerializeRichCurveKeys(CC->AlphaCurve));
                }

                // Non-curve DIs: serialize editable properties
                DObj->SetArrayField(TEXT("properties"), SerializeEditableProperties(Info.DataInterface));
                DIsArr.Add(MakeShared<FJsonValueObject>(DObj));
            }
        }
        EmObj->SetArrayField(TEXT("data_interfaces"), DIsArr);

        // Simulation stages
        TArray<TSharedPtr<FJsonValue>> StagesArr;
        for (UNiagaraSimulationStageBase* Stage : EmData->GetSimulationStages())
        {
            if (!Stage) continue;
            TSharedPtr<FJsonObject> SObj = MakeShared<FJsonObject>();
            SObj->SetStringField(TEXT("name"), Stage->SimulationStageName.ToString());
            SObj->SetBoolField(TEXT("enabled"), Stage->bEnabled);
            SObj->SetArrayField(TEXT("properties"), SerializeEditableProperties(Stage));
            StagesArr.Add(MakeShared<FJsonValueObject>(SObj));
        }
        EmObj->SetArrayField(TEXT("simulation_stages"), StagesArr);

        // Event handlers
        TArray<TSharedPtr<FJsonValue>> EventsArr;
        for (const FNiagaraEventScriptProperties& Evt : EmData->GetEventHandlers())
        {
            TSharedPtr<FJsonObject> EObj = MakeShared<FJsonObject>();
            EObj->SetStringField(TEXT("source_event_name"), Evt.SourceEventName.ToString());
            EObj->SetStringField(TEXT("source_emitter_id"), Evt.SourceEmitterID.ToString());
            EObj->SetNumberField(TEXT("max_events_per_frame"), Evt.MaxEventsPerFrame);
            EObj->SetStringField(TEXT("execution_mode"),
                Evt.ExecutionMode == EScriptExecutionMode::EveryParticle ? TEXT("EveryParticle") :
                Evt.ExecutionMode == EScriptExecutionMode::SpawnedParticles ? TEXT("SpawnedParticles") : TEXT("SingleParticle"));
            if (Evt.Script)
                EObj->SetStringField(TEXT("usage_id"), Evt.Script->GetUsageId().ToString());
            EventsArr.Add(MakeShared<FJsonValueObject>(EObj));
        }
        EmObj->SetArrayField(TEXT("event_handlers"), EventsArr);

        // Scalability
        TSharedPtr<FJsonObject> ScalObj = MakeShared<FJsonObject>();
        const FNiagaraEmitterScalabilitySettings& Scal = EmData->GetScalabilitySettings();
        ScalObj->SetBoolField(TEXT("scale_spawn_count"), Scal.bScaleSpawnCount);
        ScalObj->SetNumberField(TEXT("spawn_count_scale"), Scal.SpawnCountScale);
        EmObj->SetObjectField(TEXT("scalability"), ScalObj);

        EmittersArr.Add(MakeShared<FJsonValueObject>(EmObj));
    }
    Spec->SetArrayField(TEXT("emitters"), EmittersArr);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("success"));
    Result->SetObjectField(TEXT("spec"), Spec);
    return Result;
}
