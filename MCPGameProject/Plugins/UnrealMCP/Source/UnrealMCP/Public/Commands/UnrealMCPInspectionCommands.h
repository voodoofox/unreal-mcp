#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Blueprint inspection and asset browsing MCP commands
 */
class UNREALMCP_API FUnrealMCPInspectionCommands
{
public:
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    TSharedPtr<FJsonObject> HandleGetBlueprintComponents(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetBlueprintDefaults(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleListAssets(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetAssetDetails(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetMaterialInfo(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetStaticMeshInfo(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetComponentProperties(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetBlueprintGraph(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetNodePins(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleListNaniteStatus(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetStaticMeshNaniteEnabled(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetMaterialGraph(const TSharedPtr<FJsonObject>& Params);

    // Material editing (write operations)
    TSharedPtr<FJsonObject> HandleSetMaterialInstanceParent(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddMaterialExpression(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDeleteMaterialExpression(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleConnectMaterialExpressions(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleConnectMaterialProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDisconnectMaterialProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetMaterialExpressionProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCompileMaterial(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetSkeletonBones(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetBehaviorTreeInfo(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetMetaSoundGraph(const TSharedPtr<FJsonObject>& Params);
};
