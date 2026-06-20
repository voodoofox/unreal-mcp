#include "Commands/UnrealMCPNiagaraViewModelCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"

#pragma warning(push)
#pragma warning(disable: 4267)
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraTypes.h"
#pragma warning(pop)

#include "ViewModels/NiagaraSystemViewModel.h"
#include "ViewModels/NiagaraEmitterHandleViewModel.h"
#include "ViewModels/Stack/NiagaraStackViewModel.h"
#include "ViewModels/Stack/NiagaraStackEntry.h"
#include "ViewModels/Stack/NiagaraStackModuleItem.h"
#include "ViewModels/Stack/NiagaraStackItem.h"
#include "ViewModels/Stack/NiagaraStackFunctionInput.h"
#include "ViewModels/Stack/NiagaraStackRoot.h"

#include "NiagaraNodeFunctionCall.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "EdGraphSchema_Niagara.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/StructOnScope.h"

// ─── Helpers ──────────────────────────────────────────────────────────────────

static UNiagaraSystem* LoadNiagaraSystemVM(const FString& SystemPath)
{
    UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, *SystemPath);
    if (System) return System;

    if (SystemPath.Len() > 0 && SystemPath[0] != TEXT('/'))
    {
        System = LoadObject<UNiagaraSystem>(nullptr, *(TEXT("/Game/") + SystemPath));
        if (System) return System;
    }

    FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> Assets;
    ARM.Get().GetAssetsByClass(UNiagaraSystem::StaticClass()->GetClassPathName(), Assets);
    for (const FAssetData& Asset : Assets)
    {
        if (Asset.AssetName.ToString().Find(SystemPath) != INDEX_NONE)
            return Cast<UNiagaraSystem>(Asset.GetAsset());
    }
    return nullptr;
}

TSharedPtr<FNiagaraSystemViewModel> FUnrealMCPNiagaraViewModelCommands::GetOrCreateViewModel(UNiagaraSystem* System)
{
    if (!System) return nullptr;

    // Disable traversal cache so ViewModel reads live graph data (not stale cache)
    static IConsoleVariable* TraversalCacheCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("fx.Niagara.EnableTraversalCache"));
    int32 OldValue = 1;
    if (TraversalCacheCVar)
    {
        OldValue = TraversalCacheCVar->GetInt();
        TraversalCacheCVar->Set(0);
    }

    TSharedPtr<FNiagaraSystemViewModel> VM;
    try
    {
        VM = MakeShared<FNiagaraSystemViewModel>();
        FNiagaraSystemViewModelOptions Options;
        Options.bIsForDataProcessingOnly = false;
        Options.bCanAutoCompile = false;
        Options.bCanSimulate = false;
        Options.bCanModifyEmittersFromTimeline = false;
        Options.MessageLogGuid = FGuid::NewGuid();
        VM->Initialize(*System, Options);
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("MCP: FNiagaraSystemViewModel creation/init crashed"));
        if (TraversalCacheCVar) TraversalCacheCVar->Set(OldValue);
        return nullptr;
    }

    if (TraversalCacheCVar) TraversalCacheCVar->Set(OldValue);
    return VM;
}

static void SafeReleaseViewModel(TSharedPtr<FNiagaraSystemViewModel>& VM)
{
    if (!VM) return;
    try
    {
        VM->ResetStack();
    }
    catch (...) {}
    VM.Reset();
}

// Helper: normalize module name for matching ("SpawnRate" matches "Spawn Rate")
static FString NormalizeModuleName(const FString& Name)
{
    return Name.Replace(TEXT(" "), TEXT("")).ToLower();
}

void FUnrealMCPNiagaraViewModelCommands::CollectInputs(
    UNiagaraStackEntry* Entry, TArray<UNiagaraStackFunctionInput*>& OutInputs)
{
    if (!Entry) return;

    if (UNiagaraStackFunctionInput* Input = Cast<UNiagaraStackFunctionInput>(Entry))
    {
        OutInputs.Add(Input);
    }

    TArray<UNiagaraStackEntry*> Children;
    Entry->GetUnfilteredChildren(Children);
    for (UNiagaraStackEntry* Child : Children)
    {
        CollectInputs(Child, OutInputs);
    }
}

UNiagaraStackFunctionInput* FUnrealMCPNiagaraViewModelCommands::FindInput(
    TSharedPtr<FNiagaraSystemViewModel> VM, int32 EmitterIndex,
    const FString& ScriptType, const FString& ModuleName, const FString& InputName)
{
    if (!VM) return nullptr;

    const auto& EmitterVMs = VM->GetEmitterHandleViewModels();
    if (!EmitterVMs.IsValidIndex(EmitterIndex)) return nullptr;

    UNiagaraStackViewModel* StackVM = EmitterVMs[EmitterIndex]->GetEmitterStackViewModel();
    if (!StackVM) return nullptr;

    UNiagaraStackEntry* Root = StackVM->GetRootEntry();
    if (!Root) return nullptr;

    // Walk the tree to find all UNiagaraStackModuleItem entries
    TArray<UNiagaraStackEntry*> AllEntries;
    TQueue<UNiagaraStackEntry*> Queue;
    Queue.Enqueue(Root);
    while (!Queue.IsEmpty())
    {
        UNiagaraStackEntry* Entry;
        Queue.Dequeue(Entry);
        if (!Entry) continue;
        AllEntries.Add(Entry);

        TArray<UNiagaraStackEntry*> Children;
        Entry->GetUnfilteredChildren(Children);
        for (UNiagaraStackEntry* Child : Children)
            Queue.Enqueue(Child);
    }

    // Find the module by name (supports both "SpawnRate" and "Spawn Rate")
    FString NormalizedSearch = NormalizeModuleName(ModuleName);
    UNiagaraStackModuleItem* FoundModule = nullptr;
    for (UNiagaraStackEntry* Entry : AllEntries)
    {
        UNiagaraStackModuleItem* Module = Cast<UNiagaraStackModuleItem>(Entry);
        if (!Module) continue;

        FString DisplayName = Module->GetDisplayName().ToString();
        FString NormalizedDisplay = NormalizeModuleName(DisplayName);

        // Exact normalized match first
        if (NormalizedDisplay == NormalizedSearch)
        {
            FoundModule = Module;
            break;
        }
        // Substring match fallback
        if (NormalizedDisplay.Find(NormalizedSearch) != INDEX_NONE)
        {
            FoundModule = Module;
            break;
        }
    }

    if (!FoundModule) return nullptr;

    // Refresh module to pick up static switch changes
    FoundModule->RefreshChildren();

    // Get all parameter inputs via the proper ViewModel API
    TArray<UNiagaraStackFunctionInput*> Inputs;
    FoundModule->GetParameterInputs(Inputs);

    // Also collect from tree (for nested inputs inside categories)
    TArray<UNiagaraStackFunctionInput*> AllInputs;
    CollectInputs(FoundModule, AllInputs);
    for (UNiagaraStackFunctionInput* Input : AllInputs)
    {
        if (!Inputs.Contains(Input))
            Inputs.Add(Input);
    }

    // Find the input by name — exact match only, no substring
    for (UNiagaraStackFunctionInput* Input : Inputs)
    {
        FString DisplayName = Input->GetDisplayName().ToString();
        if (DisplayName == InputName)
            return Input;
    }

    // Fallback: case-insensitive exact match
    for (UNiagaraStackFunctionInput* Input : Inputs)
    {
        FString DisplayName = Input->GetDisplayName().ToString();
        if (DisplayName.Equals(InputName, ESearchCase::IgnoreCase))
            return Input;
    }

    return nullptr;
}

// ─── FindModule: locate a stack module item by name ───────────────────────────

UNiagaraStackModuleItem* FUnrealMCPNiagaraViewModelCommands::FindModule(
    TSharedPtr<FNiagaraSystemViewModel> VM, int32 EmitterIndex, const FString& ModuleName)
{
    if (!VM) return nullptr;

    const auto& EmitterVMs = VM->GetEmitterHandleViewModels();
    if (!EmitterVMs.IsValidIndex(EmitterIndex)) return nullptr;

    UNiagaraStackViewModel* StackVM = EmitterVMs[EmitterIndex]->GetEmitterStackViewModel();
    if (!StackVM) return nullptr;

    UNiagaraStackEntry* Root = StackVM->GetRootEntry();
    if (!Root) return nullptr;

    const FString NormalizedSearch = NormalizeModuleName(ModuleName);
    UNiagaraStackModuleItem* Exact = nullptr;
    UNiagaraStackModuleItem* Fuzzy = nullptr;

    TQueue<UNiagaraStackEntry*> Queue;
    Queue.Enqueue(Root);
    while (!Queue.IsEmpty())
    {
        UNiagaraStackEntry* Entry = nullptr;
        Queue.Dequeue(Entry);
        if (!Entry) continue;

        if (UNiagaraStackModuleItem* Module = Cast<UNiagaraStackModuleItem>(Entry))
        {
            const FString NormalizedDisplay = NormalizeModuleName(Module->GetDisplayName().ToString());
            if (NormalizedDisplay == NormalizedSearch) { Exact = Module; break; }
            if (!Fuzzy && NormalizedDisplay.Find(NormalizedSearch) != INDEX_NONE) Fuzzy = Module;
        }

        TArray<UNiagaraStackEntry*> Children;
        Entry->GetUnfilteredChildren(Children);
        for (UNiagaraStackEntry* Child : Children) Queue.Enqueue(Child);
    }

    return Exact ? Exact : Fuzzy;
}

// ─── niagara_vm_set_module_enabled ─────────────────────────────────────────────
// Enable/disable a module through the stack (UNiagaraStackItem::SetIsEnabled) — the editor's own path,
// which keeps the parameter-map chain valid and persists. Replaces the data-layer command that crashed.

TSharedPtr<FJsonObject> FUnrealMCPNiagaraViewModelCommands::HandleNiagaraVMSetModuleEnabled(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString ModuleName;
    if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name'"));

    bool bEnabled = true;
    Params->TryGetBoolField(TEXT("enabled"), bEnabled);

    UNiagaraSystem* System = LoadNiagaraSystemVM(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    TSharedPtr<FNiagaraSystemViewModel> VM = GetOrCreateViewModel(System);
    if (!VM)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create ViewModel"));

    UNiagaraStackModuleItem* Module = FindModule(VM, EmitterIndex, ModuleName);
    if (!Module)
    {
        SafeReleaseViewModel(VM);
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Module '%s' not found"), *ModuleName));
    }

    const FString DisplayName = Module->GetDisplayName().ToString();
    Module->SetIsEnabled(bEnabled);
    const bool bEnabledAfter = Module->GetIsEnabled();

    System->MarkPackageDirty();
    UEditorAssetLibrary::SaveAsset(System->GetPathName());

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("module"), DisplayName);
    Result->SetBoolField(TEXT("enabled"), bEnabledAfter);
    SafeReleaseViewModel(VM);
    return Result;
}

// ─── niagara_vm_delete_module ──────────────────────────────────────────────────
// Delete a module through the stack (UNiagaraStackModuleItem::Delete) — reconnects the chain + persists.

TSharedPtr<FJsonObject> FUnrealMCPNiagaraViewModelCommands::HandleNiagaraVMDeleteModule(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString ModuleName;
    if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name'"));

    UNiagaraSystem* System = LoadNiagaraSystemVM(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    TSharedPtr<FNiagaraSystemViewModel> VM = GetOrCreateViewModel(System);
    if (!VM)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create ViewModel"));

    UNiagaraStackModuleItem* Module = FindModule(VM, EmitterIndex, ModuleName);
    if (!Module)
    {
        SafeReleaseViewModel(VM);
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Module '%s' not found"), *ModuleName));
    }

    const FString DisplayName = Module->GetDisplayName().ToString();
    if (!Module->CanMoveAndDelete())
    {
        SafeReleaseViewModel(VM);
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Module '%s' cannot be deleted"), *DisplayName));
    }

    Module->Delete();  // Module pointer is invalid after this — DisplayName captured above

    System->MarkPackageDirty();
    UEditorAssetLibrary::SaveAsset(System->GetPathName());

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("deleted_module"), DisplayName);
    SafeReleaseViewModel(VM);
    return Result;
}

// ─── Command Router ───────────────────────────────────────────────────────────

TSharedPtr<FJsonObject> FUnrealMCPNiagaraViewModelCommands::HandleCommand(
    const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("niagara_vm_set_input"))
        return HandleNiagaraVMSetInput(Params);
    if (CommandType == TEXT("niagara_vm_get_inputs"))
        return HandleNiagaraVMGetInputs(Params);
    if (CommandType == TEXT("niagara_vm_set_dynamic_input"))
        return HandleNiagaraVMSetDynamicInput(Params);
    if (CommandType == TEXT("niagara_vm_set_module_enabled"))
        return HandleNiagaraVMSetModuleEnabled(Params);
    if (CommandType == TEXT("niagara_vm_delete_module"))
        return HandleNiagaraVMDeleteModule(Params);

    return FUnrealMCPCommonUtils::CreateErrorResponse(
        FString::Printf(TEXT("Unknown Niagara VM command: %s"), *CommandType));
}

// ─── niagara_vm_set_input ─────────────────────────────────────────────────────
// The holy grail: sets ANY module input to a static value using the ViewModel.
// Handles static switches, unoverridden defaults, rapid iteration params — everything.

TSharedPtr<FJsonObject> FUnrealMCPNiagaraViewModelCommands::HandleNiagaraVMSetInput(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString ModuleName;
    if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name'"));

    FString InputName;
    if (!Params->TryGetStringField(TEXT("input_name"), InputName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'input_name'"));

    FString Value;
    if (!Params->TryGetStringField(TEXT("value"), Value))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value'"));

    UNiagaraSystem* System = LoadNiagaraSystemVM(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    TSharedPtr<FNiagaraSystemViewModel> VM = GetOrCreateViewModel(System);
    if (!VM)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create ViewModel"));

    UNiagaraStackFunctionInput* Input = FindInput(VM, EmitterIndex, TEXT(""), ModuleName, InputName);
    if (!Input)
    {
        // Pin fallback: find the FunctionCall node directly and set pin default
        SafeReleaseViewModel(VM);

        const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
        if (!Handles.IsValidIndex(EmitterIndex))
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

        FVersionedNiagaraEmitterData* EmData = Handles[EmitterIndex].GetInstance().GetEmitterData();
        if (!EmData)
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));

        // Search emitter scripts for our module's FunctionCall
        UNiagaraNodeFunctionCall* FoundFunc = nullptr;
        FString NormSearch = NormalizeModuleName(ModuleName);
        TArray<ENiagaraScriptUsage> Usages = {
            ENiagaraScriptUsage::ParticleSpawnScript,
            ENiagaraScriptUsage::ParticleUpdateScript,
            ENiagaraScriptUsage::EmitterSpawnScript,
            ENiagaraScriptUsage::EmitterUpdateScript
        };
        for (ENiagaraScriptUsage Usage : Usages)
        {
            if (FoundFunc) break;
            UNiagaraScript* Script = EmData->GetScript(Usage, FGuid());
            if (!Script) continue;
            UNiagaraScriptSource* ScriptSrc = Cast<UNiagaraScriptSource>(Script->GetLatestSource());
            if (!ScriptSrc || !ScriptSrc->NodeGraph) continue;

            TArray<UNiagaraNodeFunctionCall*> FuncNodes;
            ScriptSrc->NodeGraph->GetNodesOfClass(FuncNodes);
            for (UNiagaraNodeFunctionCall* FN : FuncNodes)
            {
                FString Title = FN->GetNodeTitle(ENodeTitleType::ListView).ToString();
                if (NormalizeModuleName(Title) == NormSearch || NormalizeModuleName(Title).Find(NormSearch) != INDEX_NONE)
                {
                    FoundFunc = FN;
                    break;
                }
            }
        }

        if (!FoundFunc)
            return FUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Input '%s' not found on module '%s' (pin fallback: module not found)"), *InputName, *ModuleName));

        UEdGraphPin* TargetPin = nullptr;
        for (UEdGraphPin* Pin : FoundFunc->Pins)
        {
            if (Pin && Pin->Direction == EGPD_Input && Pin->PinName.ToString() == InputName)
            {
                TargetPin = Pin;
                break;
            }
        }

        if (!TargetPin)
            return FUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Pin '%s' not found on module '%s'"), *InputName, *ModuleName));

        TargetPin->DefaultValue = Value;
        FoundFunc->GetGraph()->NotifyGraphChanged();
        System->RequestCompile(false);
        System->MarkPackageDirty();
        UEditorAssetLibrary::SaveAsset(System->GetPathName());

        TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetBoolField(TEXT("success"), true);
        Result->SetStringField(TEXT("module"), ModuleName);
        Result->SetStringField(TEXT("input"), InputName);
        Result->SetStringField(TEXT("value"), Value);
        Result->SetStringField(TEXT("source"), TEXT("pin_fallback"));
        return Result;
    }

    // Get the input type
    const FNiagaraTypeDefinition& InputType = Input->GetInputType();
    const UScriptStruct* Struct = Cast<UScriptStruct>(InputType.GetStruct());
    if (!Struct)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Input has no valid type struct"));

    // Create FStructOnScope with the value
    TSharedRef<FStructOnScope> LocalValue = MakeShared<FStructOnScope>(Struct);

    // Parse value into the struct based on type
    if (InputType == FNiagaraTypeDefinition::GetFloatDef())
    {
        float* Data = (float*)LocalValue->GetStructMemory();
        *Data = FCString::Atof(*Value);
    }
    else if (InputType == FNiagaraTypeDefinition::GetIntDef())
    {
        int32* Data = (int32*)LocalValue->GetStructMemory();
        *Data = FCString::Atoi(*Value);
    }
    else if (InputType == FNiagaraTypeDefinition::GetBoolDef())
    {
        int32* Data = (int32*)LocalValue->GetStructMemory(); // Niagara bools are int32
        *Data = Value.ToBool() ? 1 : 0;
    }
    else if (InputType == FNiagaraTypeDefinition::GetVec3Def() ||
             InputType == FNiagaraTypeDefinition::GetPositionDef())
    {
        FVector3f* Data = (FVector3f*)LocalValue->GetStructMemory();
        FString Clean = Value.Replace(TEXT("("), TEXT("")).Replace(TEXT(")"), TEXT("")).TrimStartAndEnd();
        TArray<FString> Parts;
        Clean.ParseIntoArray(Parts, TEXT(","));
        if (Parts.Num() >= 3)
            *Data = FVector3f(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2]));
    }
    else if (InputType == FNiagaraTypeDefinition::GetVec2Def())
    {
        FVector2f* Data = (FVector2f*)LocalValue->GetStructMemory();
        FString Clean = Value.Replace(TEXT("("), TEXT("")).Replace(TEXT(")"), TEXT("")).TrimStartAndEnd();
        TArray<FString> Parts;
        Clean.ParseIntoArray(Parts, TEXT(","));
        if (Parts.Num() >= 2)
            *Data = FVector2f(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]));
    }
    else if (InputType == FNiagaraTypeDefinition::GetColorDef())
    {
        FLinearColor* Data = (FLinearColor*)LocalValue->GetStructMemory();
        FString Clean = Value.Replace(TEXT("("), TEXT("")).Replace(TEXT(")"), TEXT("")).TrimStartAndEnd();
        TArray<FString> Parts;
        Clean.ParseIntoArray(Parts, TEXT(","));
        if (Parts.Num() >= 4)
            *Data = FLinearColor(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2]), FCString::Atof(*Parts[3]));
        else if (Parts.Num() >= 3)
            *Data = FLinearColor(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2]), 1.0f);
    }
    else if (Input->IsStaticParameter())
    {
        // Static switches (enums) — stored as int32
        int32* Data = (int32*)LocalValue->GetStructMemory();
        *Data = FCString::Atoi(*Value);
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unsupported input type: %s"), *InputType.GetName()));
    }

    // THE KEY CALL — SetLocalValue handles everything:
    // override pins, rapid iteration params, graph relayout, compilation
    Input->SetLocalValue(LocalValue);

    // Force save BEFORE the ViewModel goes out of scope
    // (ViewModel destructor may revert transient state)
    System->MarkPackageDirty();
    UEditorAssetLibrary::SaveAsset(System->GetPathName());

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("module"), ModuleName);
    Result->SetStringField(TEXT("input"), Input->GetDisplayName().ToString());
    Result->SetStringField(TEXT("value"), Value);
    Result->SetStringField(TEXT("type"), InputType.GetName());
    Result->SetBoolField(TEXT("is_static_parameter"), Input->IsStaticParameter());
    SafeReleaseViewModel(VM);
    return Result;
}

// ─── niagara_vm_get_inputs ────────────────────────────────────────────────────
// Lists ALL inputs on a module with current values, modes, and types.

TSharedPtr<FJsonObject> FUnrealMCPNiagaraViewModelCommands::HandleNiagaraVMGetInputs(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString ModuleName;
    if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name'"));

    UNiagaraSystem* System = LoadNiagaraSystemVM(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    TSharedPtr<FNiagaraSystemViewModel> VM = GetOrCreateViewModel(System);
    if (!VM)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create ViewModel"));

    const auto& EmitterVMs = VM->GetEmitterHandleViewModels();
    if (!EmitterVMs.IsValidIndex(EmitterIndex))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid emitter_index"));

    UNiagaraStackViewModel* StackVM = EmitterVMs[EmitterIndex]->GetEmitterStackViewModel();
    if (!StackVM)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No stack ViewModel"));

    // Force a full recursive refresh so children are populated
    StackVM->GetRootEntry()->RefreshChildren();
    {
        TArray<UNiagaraStackEntry*> TopChildren;
        StackVM->GetRootEntry()->GetUnfilteredChildren(TopChildren);
        for (UNiagaraStackEntry* C : TopChildren)
        {
            C->RefreshChildren();
            TArray<UNiagaraStackEntry*> Sub;
            C->GetUnfilteredChildren(Sub);
            for (UNiagaraStackEntry* S : Sub)
            {
                S->RefreshChildren();
                TArray<UNiagaraStackEntry*> Sub2;
                S->GetUnfilteredChildren(Sub2);
                for (UNiagaraStackEntry* S2 : Sub2)
                    S2->RefreshChildren();
            }
        }
    }

    // Walk tree to find the module
    UNiagaraStackEntry* RootEntry = StackVM->GetRootEntry();
    TQueue<UNiagaraStackEntry*> Queue;
    Queue.Enqueue(RootEntry);
    UNiagaraStackModuleItem* FoundModule = nullptr;
    TArray<FString> FoundModuleNames;
    int32 TotalEntries = 0;

    while (!Queue.IsEmpty())
    {
        UNiagaraStackEntry* Entry;
        Queue.Dequeue(Entry);
        if (!Entry) continue;
        TotalEntries++;

        if (UNiagaraStackModuleItem* Module = Cast<UNiagaraStackModuleItem>(Entry))
        {
            FString DisplayName = Module->GetDisplayName().ToString();
            FoundModuleNames.Add(DisplayName);
            FString NormalizedDisplay = NormalizeModuleName(DisplayName);
            FString NormalizedSearch = NormalizeModuleName(ModuleName);
            if (NormalizedDisplay == NormalizedSearch || NormalizedDisplay.Find(NormalizedSearch) != INDEX_NONE)
            {
                FoundModule = Module;
                break;
            }
        }

        TArray<UNiagaraStackEntry*> Children;
        Entry->GetUnfilteredChildren(Children);
        for (UNiagaraStackEntry* Child : Children)
            Queue.Enqueue(Child);
    }

    if (!FoundModule)
    {
        FString AvailableModules = FString::Join(FoundModuleNames, TEXT(", "));
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Module '%s' not found. Root=%s, TotalEntries=%d, Modules=[%s]"),
                *ModuleName,
                RootEntry ? *RootEntry->GetDisplayName().ToString() : TEXT("null"),
                TotalEntries,
                *AvailableModules));
    }

    // Get all parameter inputs via the proper ViewModel API
    TArray<UNiagaraStackFunctionInput*> Inputs;
    FoundModule->GetParameterInputs(Inputs);

    TArray<TSharedPtr<FJsonValue>> InputArray;

    // Fallback: if ViewModel returns no inputs, read directly from FunctionCall pins
    if (Inputs.Num() == 0)
    {
        UNiagaraNodeFunctionCall& FuncNode = FoundModule->GetModuleNode();
        const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();
        for (UEdGraphPin* Pin : FuncNode.Pins)
        {
            if (!Pin || Pin->Direction != EGPD_Input) continue;
            FNiagaraTypeDefinition PinType = Schema->PinToTypeDefinition(Pin);
            if (PinType == FNiagaraTypeDefinition::GetParameterMapDef()) continue;

            TSharedPtr<FJsonObject> InputObj = MakeShared<FJsonObject>();
            InputObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
            InputObj->SetStringField(TEXT("type"), PinType.GetName());
            InputObj->SetBoolField(TEXT("is_static_parameter"), false);
            InputObj->SetStringField(TEXT("value_mode"), TEXT("Local"));
            if (!Pin->DefaultValue.IsEmpty())
                InputObj->SetStringField(TEXT("value"), Pin->DefaultValue);
            InputArray.Add(MakeShared<FJsonValueObject>(InputObj));
        }

        if (InputArray.Num() > 0)
        {
            TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
            Result->SetStringField(TEXT("module"), FoundModule->GetDisplayName().ToString());
            Result->SetStringField(TEXT("source"), TEXT("pin_fallback"));
            Result->SetArrayField(TEXT("inputs"), InputArray);
            SafeReleaseViewModel(VM);
            return Result;
        }
    }

    for (UNiagaraStackFunctionInput* Input : Inputs)
    {
        TSharedPtr<FJsonObject> InputObj = MakeShared<FJsonObject>();
        InputObj->SetStringField(TEXT("name"), Input->GetDisplayName().ToString());
        InputObj->SetStringField(TEXT("type"), Input->GetInputType().GetName());
        InputObj->SetBoolField(TEXT("is_static_parameter"), Input->IsStaticParameter());

        UNiagaraStackFunctionInput::EValueMode Mode = Input->GetValueMode();
        FString ModeStr;
        switch (Mode)
        {
        case UNiagaraStackFunctionInput::EValueMode::Local: ModeStr = TEXT("Local"); break;
        case UNiagaraStackFunctionInput::EValueMode::Linked: ModeStr = TEXT("Linked"); break;
        case UNiagaraStackFunctionInput::EValueMode::Dynamic: ModeStr = TEXT("Dynamic"); break;
        case UNiagaraStackFunctionInput::EValueMode::Data: ModeStr = TEXT("Data"); break;
        case UNiagaraStackFunctionInput::EValueMode::Expression: ModeStr = TEXT("Expression"); break;
        case UNiagaraStackFunctionInput::EValueMode::DefaultFunction: ModeStr = TEXT("DefaultFunction"); break;
        case UNiagaraStackFunctionInput::EValueMode::None: ModeStr = TEXT("None"); break;
        default: ModeStr = TEXT("Other"); break;
        }
        InputObj->SetStringField(TEXT("value_mode"), ModeStr);

        // Read current value if Local
        if (Mode == UNiagaraStackFunctionInput::EValueMode::Local)
        {
            TSharedPtr<const FStructOnScope> LocalStruct = Input->GetLocalValueStruct();
            if (LocalStruct.IsValid() && LocalStruct->IsValid())
            {
                const FNiagaraTypeDefinition& Type = Input->GetInputType();
                const uint8* Memory = LocalStruct->GetStructMemory();
                int32 StructSize = LocalStruct->GetStruct()->GetStructureSize();
                if (Type == FNiagaraTypeDefinition::GetFloatDef())
                    InputObj->SetNumberField(TEXT("value"), *(const float*)Memory);
                else if (Type == FNiagaraTypeDefinition::GetIntDef())
                    InputObj->SetNumberField(TEXT("value"), *(const int32*)Memory);
                else if (Type == FNiagaraTypeDefinition::GetBoolDef())
                    InputObj->SetBoolField(TEXT("value"), *(const int32*)Memory != 0);
                else if (Input->IsStaticParameter() || StructSize <= 4)
                {
                    // Enums/static switches — read as int32
                    InputObj->SetNumberField(TEXT("value"), StructSize >= 4 ? *(const int32*)Memory : (int32)(*Memory));
                }
                else if (Type == FNiagaraTypeDefinition::GetVec3Def() || Type == FNiagaraTypeDefinition::GetPositionDef())
                {
                    const FVector3f* V = (const FVector3f*)Memory;
                    InputObj->SetStringField(TEXT("value"), FString::Printf(TEXT("(%f,%f,%f)"), V->X, V->Y, V->Z));
                }
                else
                {
                    InputObj->SetStringField(TEXT("value"), FString::Printf(TEXT("<complex:%d bytes>"), StructSize));
                }
            }
        }

        InputArray.Add(MakeShared<FJsonValueObject>(InputObj));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("module"), FoundModule->GetDisplayName().ToString());
    Result->SetArrayField(TEXT("inputs"), InputArray);
    SafeReleaseViewModel(VM);
    return Result;
}

// ─── niagara_vm_set_dynamic_input ─────────────────────────────────────────────
// Sets a module input to a dynamic expression (e.g., Random Range Float).

TSharedPtr<FJsonObject> FUnrealMCPNiagaraViewModelCommands::HandleNiagaraVMSetDynamicInput(
    const TSharedPtr<FJsonObject>& Params)
{
    FString SystemPath;
    if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path'"));

    int32 EmitterIndex = 0;
    Params->TryGetNumberField(TEXT("emitter_index"), EmitterIndex);

    FString ModuleName;
    if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name'"));

    FString InputName;
    if (!Params->TryGetStringField(TEXT("input_name"), InputName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'input_name'"));

    FString DynamicInputPath;
    if (!Params->TryGetStringField(TEXT("dynamic_input_path"), DynamicInputPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'dynamic_input_path' (asset path to dynamic input script)"));

    UNiagaraSystem* System = LoadNiagaraSystemVM(SystemPath);
    if (!System)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("System not found"));

    TSharedPtr<FNiagaraSystemViewModel> VM = GetOrCreateViewModel(System);
    if (!VM)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create ViewModel"));

    UNiagaraStackFunctionInput* Input = FindInput(VM, EmitterIndex, TEXT(""), ModuleName, InputName);
    if (!Input)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Input '%s' not found on module '%s'"), *InputName, *ModuleName));

    // Load the dynamic input script
    UNiagaraScript* DynScript = LoadObject<UNiagaraScript>(nullptr, *DynamicInputPath);
    if (!DynScript)
    {
        // Try searching by name
        FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        TArray<FAssetData> Assets;
        ARM.Get().GetAssetsByClass(UNiagaraScript::StaticClass()->GetClassPathName(), Assets);
        for (const FAssetData& Asset : Assets)
        {
            if (Asset.AssetName.ToString().Find(DynamicInputPath) != INDEX_NONE)
            {
                DynScript = Cast<UNiagaraScript>(Asset.GetAsset());
                if (DynScript) break;
            }
        }
    }

    if (!DynScript)
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Dynamic input script not found: %s"), *DynamicInputPath));

    Input->SetDynamicInput(DynScript);
    System->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("module"), ModuleName);
    Result->SetStringField(TEXT("input"), Input->GetDisplayName().ToString());
    Result->SetStringField(TEXT("dynamic_input"), DynScript->GetName());
    SafeReleaseViewModel(VM);
    return Result;
}
