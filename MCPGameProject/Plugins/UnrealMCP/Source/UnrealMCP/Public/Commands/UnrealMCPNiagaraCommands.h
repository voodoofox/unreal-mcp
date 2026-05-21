#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Niagara system inspection and editing MCP commands.
 * Provides complete programmatic control over Niagara systems, emitters,
 * renderers, modules, and parameters.
 */
class UNREALMCP_API FUnrealMCPNiagaraCommands
{
public:
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // System-level
    TSharedPtr<FJsonObject> HandleCreateNiagaraSystem(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetNiagaraOverview(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraSystemProperty(const TSharedPtr<FJsonObject>& Params);

    // Emitter management
    TSharedPtr<FJsonObject> HandleSetNiagaraEmitterEnabled(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleRemoveNiagaraEmitter(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDuplicateNiagaraEmitter(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraEmitterProperty(const TSharedPtr<FJsonObject>& Params);

    // Renderer management
    TSharedPtr<FJsonObject> HandleAddNiagaraRenderer(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleRemoveNiagaraRenderer(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraRendererProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetNiagaraRendererProperties(const TSharedPtr<FJsonObject>& Params);

    // Module management
    TSharedPtr<FJsonObject> HandleListNiagaraModules(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddNiagaraModule(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleRemoveNiagaraModule(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraModuleEnabled(const TSharedPtr<FJsonObject>& Params);

    // Module inputs (rapid iteration parameters)
    TSharedPtr<FJsonObject> HandleGetNiagaraModuleInputs(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraModuleInput(const TSharedPtr<FJsonObject>& Params);

    // Module pins (static switches + all pin values on a module node)
    TSharedPtr<FJsonObject> HandleGetNiagaraModulePins(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraModulePinValue(const TSharedPtr<FJsonObject>& Params);

    // User parameters (system-level, runtime-accessible)
    TSharedPtr<FJsonObject> HandleGetNiagaraUserParameters(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraUserParameter(const TSharedPtr<FJsonObject>& Params);

    // Module search
    TSharedPtr<FJsonObject> HandleSearchNiagaraModules(const TSharedPtr<FJsonObject>& Params);

    // Parameter assignment (Set New or Existing Parameter Directly)
    TSharedPtr<FJsonObject> HandleAddNiagaraSetParameter(const TSharedPtr<FJsonObject>& Params);

    // System utilities
    TSharedPtr<FJsonObject> HandleCompileNiagaraSystem(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraFixedBounds(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleRenameNiagaraEmitter(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddNiagaraUserParameter(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetNiagaraEmitterProperties(const TSharedPtr<FJsonObject>& Params);

    // Renderer bindings
    TSharedPtr<FJsonObject> HandleGetNiagaraRendererBindings(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraRendererBinding(const TSharedPtr<FJsonObject>& Params);

    // Generic operations
    TSharedPtr<FJsonObject> HandleSetNiagaraModuleOverride(const TSharedPtr<FJsonObject>& Params);

    // Ribbon custom vertices
    TSharedPtr<FJsonObject> HandleSetNiagaraCustomVertices(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetNiagaraCustomVertices(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetNiagaraDeepInspect(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetNiagaraCurveData(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraCurveData(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetNiagaraDIProperties(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraDIProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddNiagaraSimulationStage(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleRemoveNiagaraSimulationStage(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetNiagaraSimStageProperties(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraSimStageProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddNiagaraEventHandler(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraEmitterSimTarget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleRemoveNiagaraEventHandler(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddNiagaraEmitter(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraEventHandlerProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleMoveNiagaraSimulationStage(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetNiagaraScalabilitySettings(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraScalabilityOverride(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetNiagaraDataChannelInfo(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetNiagaraScratchPadScripts(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCreateNiagaraHlslModule(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleExportNiagaraSystemSpec(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetNiagaraStatelessEmitterInfo(const TSharedPtr<FJsonObject>& Params);
};
