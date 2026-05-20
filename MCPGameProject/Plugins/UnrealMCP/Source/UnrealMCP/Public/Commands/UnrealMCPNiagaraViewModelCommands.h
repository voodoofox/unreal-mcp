#pragma once

#include "CoreMinimal.h"
#include "Json.h"

class UNiagaraSystem;
class FNiagaraSystemViewModel;
class UNiagaraStackFunctionInput;
class UNiagaraStackEntry;

/**
 * ViewModel-based Niagara commands.
 * Uses UNiagaraSystemViewModel → UNiagaraStackFunctionInput::SetLocalValue()
 * for proper static switch, dynamic input, and module input handling.
 * This is the same code path the Niagara editor UI uses.
 */
class UNREALMCP_API FUnrealMCPNiagaraViewModelCommands
{
public:
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Set any module input to a static value (handles static switches, unoverridden defaults, everything)
    TSharedPtr<FJsonObject> HandleNiagaraVMSetInput(const TSharedPtr<FJsonObject>& Params);

    // List all inputs on a module with their current values and modes
    TSharedPtr<FJsonObject> HandleNiagaraVMGetInputs(const TSharedPtr<FJsonObject>& Params);

    // Set a dynamic input expression on a module input
    TSharedPtr<FJsonObject> HandleNiagaraVMSetDynamicInput(const TSharedPtr<FJsonObject>& Params);

    // Helpers
    TSharedPtr<FNiagaraSystemViewModel> GetOrCreateViewModel(UNiagaraSystem* System);
    UNiagaraStackFunctionInput* FindInput(TSharedPtr<FNiagaraSystemViewModel> VM, int32 EmitterIndex,
        const FString& ScriptType, const FString& ModuleName, const FString& InputName);
    void CollectInputs(UNiagaraStackEntry* Entry, TArray<UNiagaraStackFunctionInput*>& OutInputs);

    // Cache ViewModels to avoid re-creating them
    TMap<TWeakObjectPtr<UNiagaraSystem>, TSharedPtr<FNiagaraSystemViewModel>> ViewModelCache;
};
