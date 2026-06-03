#include "Commands/UnrealMCPCommonUtils.h"
#include "GameFramework/Actor.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/UObjectIterator.h"
#include "Engine/Selection.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabase.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

// JSON Utilities
TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::CreateErrorResponse(const FString& Message)
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
    ResponseObject->SetBoolField(TEXT("success"), false);
    ResponseObject->SetStringField(TEXT("error"), Message);
    return ResponseObject;
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data)
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
    ResponseObject->SetBoolField(TEXT("success"), true);
    
    if (Data.IsValid())
    {
        ResponseObject->SetObjectField(TEXT("data"), Data);
    }
    
    return ResponseObject;
}

void FUnrealMCPCommonUtils::GetIntArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<int32>& OutArray)
{
    OutArray.Reset();
    
    if (!JsonObject->HasField(FieldName))
    {
        return;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
        {
            OutArray.Add((int32)Value->AsNumber());
        }
    }
}

void FUnrealMCPCommonUtils::GetFloatArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<float>& OutArray)
{
    OutArray.Reset();
    
    if (!JsonObject->HasField(FieldName))
    {
        return;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
        {
            OutArray.Add((float)Value->AsNumber());
        }
    }
}

FVector2D FUnrealMCPCommonUtils::GetVector2DFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FVector2D Result(0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 2)
    {
        Result.X = (float)(*JsonArray)[0]->AsNumber();
        Result.Y = (float)(*JsonArray)[1]->AsNumber();
    }
    
    return Result;
}

FVector FUnrealMCPCommonUtils::GetVectorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FVector Result(0.0f, 0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
    {
        Result.X = (float)(*JsonArray)[0]->AsNumber();
        Result.Y = (float)(*JsonArray)[1]->AsNumber();
        Result.Z = (float)(*JsonArray)[2]->AsNumber();
    }
    
    return Result;
}

FRotator FUnrealMCPCommonUtils::GetRotatorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FRotator Result(0.0f, 0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
    {
        Result.Pitch = (float)(*JsonArray)[0]->AsNumber();
        Result.Yaw = (float)(*JsonArray)[1]->AsNumber();
        Result.Roll = (float)(*JsonArray)[2]->AsNumber();
    }
    
    return Result;
}

// Blueprint Utilities
UBlueprint* FUnrealMCPCommonUtils::FindBlueprint(const FString& BlueprintName)
{
    return FindBlueprintByName(BlueprintName);
}

UBlueprint* FUnrealMCPCommonUtils::FindBlueprintByName(const FString& BlueprintName)
{
    UBlueprint* BP = nullptr;

    // CRITICAL: a rooted content path (starts with '/') must be loaded DIRECTLY. Never prepend
    // "/Game/Blueprints/" to it — that produces a "//" in the package name, and
    // LoadObject -> LoadPackage -> CreatePackage hits a FATAL assert ("name containing double
    // slashes", UObjectGlobals.cpp) which hard-CRASHES the editor instead of failing gracefully.
    // Only bare asset names get the /Game/Blueprints/ convenience prefix.
    if (BlueprintName.StartsWith(TEXT("/")))
    {
        BP = LoadObject<UBlueprint>(nullptr, *BlueprintName);
        if (BP) return BP;
    }
    else
    {
        const FString AssetPath = TEXT("/Game/Blueprints/") + BlueprintName;
        BP = LoadObject<UBlueprint>(nullptr, *AssetPath);
        if (BP) return BP;
    }

    // Search AssetRegistry for any Blueprint matching this name anywhere under /Game.
    // Match on the LEAF name so a full path, a "Name.Name" object path, or a "Name_C"
    // class name all still resolve here.
    FString LeafName = BlueprintName;
    {
        int32 SlashIdx;
        if (LeafName.FindLastChar(TEXT('/'), SlashIdx)) { LeafName = LeafName.Mid(SlashIdx + 1); }
        int32 DotIdx;
        if (LeafName.FindChar(TEXT('.'), DotIdx)) { LeafName = LeafName.Left(DotIdx); }
        LeafName.RemoveFromEnd(TEXT("_C"));
    }

    IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
    TArray<FAssetData> Assets;
    AssetRegistry.GetAssetsByPath(FName(TEXT("/Game")), Assets, true);

    for (const FAssetData& Asset : Assets)
    {
        if (Asset.AssetName.ToString() == LeafName)
        {
            FString AssetClassName = Asset.AssetClassPath.GetAssetName().ToString();
            if (AssetClassName == TEXT("Blueprint") || AssetClassName == TEXT("WidgetBlueprint"))
            {
                BP = LoadObject<UBlueprint>(nullptr, *Asset.GetObjectPathString());
                if (BP) return BP;
            }
        }
    }

    return nullptr;
}

UEdGraph* FUnrealMCPCommonUtils::FindOrCreateEventGraph(UBlueprint* Blueprint)
{
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // Try to find the event graph
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph->GetName().Contains(TEXT("EventGraph")))
        {
            return Graph;
        }
    }
    
    // Create a new event graph if none exists
    UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, FName(TEXT("EventGraph")), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(Blueprint, NewGraph);
    return NewGraph;
}

// Blueprint node utilities
UK2Node_Event* FUnrealMCPCommonUtils::CreateEventNode(UEdGraph* Graph, const FString& EventName, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // Check for existing event node with this exact name
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
        {
            UE_LOG(LogTemp, Display, TEXT("Using existing event node with name %s (ID: %s)"), 
                *EventName, *EventNode->NodeGuid.ToString());
            return EventNode;
        }
    }

    // No existing node found, create a new one
    UK2Node_Event* EventNode = nullptr;
    
    // Find the function to create the event
    UClass* BlueprintClass = Blueprint->GeneratedClass;
    UFunction* EventFunction = BlueprintClass->FindFunctionByName(FName(*EventName));
    
    if (EventFunction)
    {
        EventNode = NewObject<UK2Node_Event>(Graph);
        EventNode->EventReference.SetExternalMember(FName(*EventName), BlueprintClass);
        EventNode->NodePosX = Position.X;
        EventNode->NodePosY = Position.Y;
        Graph->AddNode(EventNode, true);
        EventNode->PostPlacedNewNode();
        EventNode->AllocateDefaultPins();
        UE_LOG(LogTemp, Display, TEXT("Created new event node with name %s (ID: %s)"), 
            *EventName, *EventNode->NodeGuid.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find function for event name: %s"), *EventName);
    }
    
    return EventNode;
}

UK2Node_CallFunction* FUnrealMCPCommonUtils::CreateFunctionCallNode(UEdGraph* Graph, UFunction* Function, const FVector2D& Position)
{
    if (!Graph || !Function)
    {
        return nullptr;
    }
    
    UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(Graph);
    FunctionNode->SetFromFunction(Function);
    FunctionNode->NodePosX = Position.X;
    FunctionNode->NodePosY = Position.Y;
    Graph->AddNode(FunctionNode, true);
    FunctionNode->CreateNewGuid();
    FunctionNode->PostPlacedNewNode();
    FunctionNode->AllocateDefaultPins();
    
    return FunctionNode;
}

UK2Node_VariableGet* FUnrealMCPCommonUtils::CreateVariableGetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position)
{
    if (!Graph || !Blueprint)
    {
        return nullptr;
    }
    
    UK2Node_VariableGet* VariableGetNode = NewObject<UK2Node_VariableGet>(Graph);
    
    FName VarName(*VariableName);
    FProperty* Property = FindFProperty<FProperty>(Blueprint->GeneratedClass, VarName);
    
    if (Property)
    {
        // SetSelfMember (not SetFromField with bSelfContext=false): tells the variable
        // system "this variable belongs to self (the BP instance)", which leaves the
        // node's hidden 'self' input auto-resolved. SetFromField(..., false) leaves self
        // dangling and compile errors with "uses an invalid target".
        VariableGetNode->VariableReference.SetSelfMember(VarName);
        VariableGetNode->NodePosX = Position.X;
        VariableGetNode->NodePosY = Position.Y;
        Graph->AddNode(VariableGetNode, true);
        VariableGetNode->CreateNewGuid();
        VariableGetNode->PostPlacedNewNode();
        VariableGetNode->AllocateDefaultPins();

        return VariableGetNode;
    }

    return nullptr;
}

UK2Node_VariableSet* FUnrealMCPCommonUtils::CreateVariableSetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position)
{
    if (!Graph || !Blueprint)
    {
        return nullptr;
    }

    UK2Node_VariableSet* VariableSetNode = NewObject<UK2Node_VariableSet>(Graph);

    FName VarName(*VariableName);
    FProperty* Property = FindFProperty<FProperty>(Blueprint->GeneratedClass, VarName);

    if (Property)
    {
        // See note on CreateVariableGetNode — SetSelfMember keeps the hidden 'self' pin
        // auto-resolved to the BP instance. SetFromField(..., false) would leave the
        // node with a dangling self target.
        VariableSetNode->VariableReference.SetSelfMember(VarName);
        VariableSetNode->NodePosX = Position.X;
        VariableSetNode->NodePosY = Position.Y;
        Graph->AddNode(VariableSetNode, true);
        VariableSetNode->CreateNewGuid();
        VariableSetNode->PostPlacedNewNode();
        VariableSetNode->AllocateDefaultPins();

        return VariableSetNode;
    }

    return nullptr;
}

UK2Node_InputAction* FUnrealMCPCommonUtils::CreateInputActionNode(UEdGraph* Graph, const FString& ActionName, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UK2Node_InputAction* InputActionNode = NewObject<UK2Node_InputAction>(Graph);
    InputActionNode->InputActionName = FName(*ActionName);
    InputActionNode->NodePosX = Position.X;
    InputActionNode->NodePosY = Position.Y;
    Graph->AddNode(InputActionNode, true);
    InputActionNode->CreateNewGuid();
    InputActionNode->PostPlacedNewNode();
    InputActionNode->AllocateDefaultPins();
    
    return InputActionNode;
}

UK2Node_Self* FUnrealMCPCommonUtils::CreateSelfReferenceNode(UEdGraph* Graph, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(Graph);
    SelfNode->NodePosX = Position.X;
    SelfNode->NodePosY = Position.Y;
    Graph->AddNode(SelfNode, true);
    SelfNode->CreateNewGuid();
    SelfNode->PostPlacedNewNode();
    SelfNode->AllocateDefaultPins();
    
    return SelfNode;
}

bool FUnrealMCPCommonUtils::ConnectGraphNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName, 
                                           UEdGraphNode* TargetNode, const FString& TargetPinName)
{
    if (!Graph || !SourceNode || !TargetNode)
    {
        return false;
    }
    
    UEdGraphPin* SourcePin = FindPin(SourceNode, SourcePinName, EGPD_Output);
    UEdGraphPin* TargetPin = FindPin(TargetNode, TargetPinName, EGPD_Input);
    
    if (SourcePin && TargetPin)
    {
        // Exec output pins can only have one connection — break existing before adding new
        if (SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && SourcePin->Direction == EGPD_Output)
        {
            SourcePin->BreakAllPinLinks();
        }
        // Input pins can only have one source — break existing before adding new.
        // (Applies to both exec AND data input pins. Without this, repeated connections
        // to the same data input produce a multi-sourced pin: compile succeeds but
        // code-gen picks one arbitrarily, leaving the other ignored.)
        if (TargetPin->Direction == EGPD_Input)
        {
            TargetPin->BreakAllPinLinks();
        }

        // Use the schema's TryCreateConnection so wildcard-typed nodes (Select, cast
        // results, etc.) get their types resolved from the connected pin's type.
        // Lower-level MakeLinkTo bypasses NotifyPinConnectionListChanged and leaves
        // wildcards unresolved — compile then fails with "Unsupported type Wildcard".
        const UEdGraphSchema* Schema = Graph->GetSchema();
        if (Schema && Schema->TryCreateConnection(SourcePin, TargetPin))
        {
            // Belt-and-suspenders: notify both nodes so K2Node overrides (e.g.
            // UK2Node_Select::NotifyPinConnectionListChanged) run their retyping logic.
            SourceNode->NodeConnectionListChanged();
            TargetNode->NodeConnectionListChanged();
            return true;
        }
        // Fallback if schema refuses (shouldn't normally happen): raw link
        SourcePin->MakeLinkTo(TargetPin);
        SourceNode->NodeConnectionListChanged();
        TargetNode->NodeConnectionListChanged();
        return true;
    }
    
    return false;
}

UEdGraphPin* FUnrealMCPCommonUtils::FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction)
{
    if (!Node)
    {
        return nullptr;
    }
    
    // Log all pins for debugging
    UE_LOG(LogTemp, Display, TEXT("FindPin: Looking for pin '%s' (Direction: %d) in node '%s'"), 
           *PinName, (int32)Direction, *Node->GetName());
    
    for (UEdGraphPin* Pin : Node->Pins)
    {
        UE_LOG(LogTemp, Display, TEXT("  - Available pin: '%s', Direction: %d, Category: %s"), 
               *Pin->PinName.ToString(), (int32)Pin->Direction, *Pin->PinType.PinCategory.ToString());
    }
    
    // First try exact match
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinName.ToString() == PinName && (Direction == EGPD_MAX || Pin->Direction == Direction))
        {
            UE_LOG(LogTemp, Display, TEXT("  - Found exact matching pin: '%s'"), *Pin->PinName.ToString());
            return Pin;
        }
    }
    
    // If no exact match and we're looking for a component reference, try case-insensitive match
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinName.ToString().Equals(PinName, ESearchCase::IgnoreCase) && 
            (Direction == EGPD_MAX || Pin->Direction == Direction))
        {
            UE_LOG(LogTemp, Display, TEXT("  - Found case-insensitive matching pin: '%s'"), *Pin->PinName.ToString());
            return Pin;
        }
    }
    
    // If we're looking for a component output and didn't find it by name, try to find the first data output pin
    if (Direction == EGPD_Output && Cast<UK2Node_VariableGet>(Node) != nullptr)
    {
        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
            {
                UE_LOG(LogTemp, Display, TEXT("  - Found fallback data output pin: '%s'"), *Pin->PinName.ToString());
                return Pin;
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("  - No matching pin found for '%s'"), *PinName);
    return nullptr;
}

// Actor utilities
TSharedPtr<FJsonValue> FUnrealMCPCommonUtils::ActorToJson(AActor* Actor)
{
    if (!Actor)
    {
        return MakeShared<FJsonValueNull>();
    }
    
    TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
    ActorObject->SetStringField(TEXT("name"), Actor->GetName());
    ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
    
    FVector Location = Actor->GetActorLocation();
    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
    ActorObject->SetArrayField(TEXT("location"), LocationArray);
    
    FRotator Rotation = Actor->GetActorRotation();
    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
    ActorObject->SetArrayField(TEXT("rotation"), RotationArray);
    
    FVector Scale = Actor->GetActorScale3D();
    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
    ActorObject->SetArrayField(TEXT("scale"), ScaleArray);
    
    return MakeShared<FJsonValueObject>(ActorObject);
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::ActorToJsonObject(AActor* Actor, bool bDetailed)
{
    if (!Actor)
    {
        return nullptr;
    }
    
    TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
    ActorObject->SetStringField(TEXT("name"), Actor->GetName());
    ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
    
    FVector Location = Actor->GetActorLocation();
    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
    ActorObject->SetArrayField(TEXT("location"), LocationArray);
    
    FRotator Rotation = Actor->GetActorRotation();
    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
    ActorObject->SetArrayField(TEXT("rotation"), RotationArray);
    
    FVector Scale = Actor->GetActorScale3D();
    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
    ActorObject->SetArrayField(TEXT("scale"), ScaleArray);
    
    return ActorObject;
}

UK2Node_Event* FUnrealMCPCommonUtils::FindExistingEventNode(UEdGraph* Graph, const FString& EventName)
{
    if (!Graph)
    {
        return nullptr;
    }

    // Look for existing event nodes
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
        {
            UE_LOG(LogTemp, Display, TEXT("Found existing event node with name: %s"), *EventName);
            return EventNode;
        }
    }

    return nullptr;
}

// Resolves a property path like "Foo", "Foo.Bar", "Stages[1]", or
// "Stages[1].WorldBlueprintClass" against an object. On success, OutProperty and
// OutValuePtr point at the terminal property's FProperty and raw value address.
//
// Supported syntax:
//   name                      top-level property
//   name.subname              nested struct member
//   name[idx]                 array element
//   name[idx].subname         array of structs + member
//   name.subname[idx]         struct containing array + element
//   Combinations of the above.
//
// Supported traversal steps:
//   - FStructProperty    (descends into struct fields)
//   - FObjectProperty    (dereferences the referenced UObject to access its fields)
//   - FArrayProperty     (indexes by integer)
static bool ResolvePropertyPath(
    UObject* RootObject,
    const FString& Path,
    FProperty*& OutProperty,
    void*& OutValuePtr,
    FString& OutError)
{
    if (!RootObject)
    {
        OutError = TEXT("ResolvePropertyPath: null root object");
        return false;
    }

    UStruct* CurrentStruct = RootObject->GetClass();
    void* CurrentContainer = static_cast<void*>(RootObject);
    FProperty* CurrentProperty = nullptr;
    void* CurrentValuePtr = nullptr;

    FString Remaining = Path;
    while (!Remaining.IsEmpty())
    {
        // Extract the next identifier (up to the next '.' or '[')
        int32 DotIdx = INDEX_NONE;
        int32 BracketIdx = INDEX_NONE;
        Remaining.FindChar(TEXT('.'), DotIdx);
        Remaining.FindChar(TEXT('['), BracketIdx);

        int32 NameEnd = Remaining.Len();
        if (DotIdx != INDEX_NONE && (BracketIdx == INDEX_NONE || DotIdx < BracketIdx))
        {
            NameEnd = DotIdx;
        }
        else if (BracketIdx != INDEX_NONE)
        {
            NameEnd = BracketIdx;
        }

        if (NameEnd == 0)
        {
            OutError = FString::Printf(TEXT("ResolvePropertyPath: empty name segment at '%s'"), *Remaining);
            return false;
        }

        const FString Name = Remaining.Left(NameEnd);
        Remaining = Remaining.Mid(NameEnd);

        // Look up the property in the current struct
        if (!CurrentStruct)
        {
            OutError = FString::Printf(TEXT("ResolvePropertyPath: no struct context for '%s'"), *Name);
            return false;
        }
        CurrentProperty = CurrentStruct->FindPropertyByName(*Name);
        if (!CurrentProperty)
        {
            OutError = FString::Printf(TEXT("Property not found: '%s' in %s"), *Name, *CurrentStruct->GetName());
            return false;
        }
        CurrentValuePtr = CurrentProperty->ContainerPtrToValuePtr<void>(CurrentContainer);

        // Handle array indexing (possibly chained for nested arrays: [0][1])
        while (Remaining.StartsWith(TEXT("[")))
        {
            int32 CloseIdx = INDEX_NONE;
            Remaining.FindChar(TEXT(']'), CloseIdx);
            if (CloseIdx == INDEX_NONE)
            {
                OutError = FString::Printf(TEXT("ResolvePropertyPath: unclosed '[' in '%s'"), *Remaining);
                return false;
            }
            const FString IdxStr = Remaining.Mid(1, CloseIdx - 1);
            Remaining = Remaining.Mid(CloseIdx + 1);
            if (!IdxStr.IsNumeric())
            {
                OutError = FString::Printf(TEXT("ResolvePropertyPath: array index must be numeric, got '%s'"), *IdxStr);
                return false;
            }
            const int32 Idx = FCString::Atoi(*IdxStr);

            FArrayProperty* ArrayProp = CastField<FArrayProperty>(CurrentProperty);
            if (!ArrayProp)
            {
                OutError = FString::Printf(TEXT("ResolvePropertyPath: '%s' is not an array (got %s)"),
                    *Name, *CurrentProperty->GetClass()->GetName());
                return false;
            }
            FScriptArrayHelper Helper(ArrayProp, CurrentValuePtr);
            if (Idx < 0 || Idx >= Helper.Num())
            {
                OutError = FString::Printf(TEXT("ResolvePropertyPath: index %d out of bounds [0..%d) for '%s'"),
                    Idx, Helper.Num(), *Name);
                return false;
            }
            CurrentValuePtr = Helper.GetRawPtr(Idx);
            CurrentProperty = ArrayProp->Inner;
        }

        // If there's more path after this segment, consume a dot and descend
        if (Remaining.StartsWith(TEXT(".")))
        {
            Remaining = Remaining.Mid(1);
            if (FStructProperty* StructProp = CastField<FStructProperty>(CurrentProperty))
            {
                CurrentStruct = StructProp->Struct;
                CurrentContainer = CurrentValuePtr;
            }
            else if (FObjectProperty* ObjProp = CastField<FObjectProperty>(CurrentProperty))
            {
                UObject* InnerObject = ObjProp->GetObjectPropertyValue(CurrentValuePtr);
                if (!InnerObject)
                {
                    OutError = FString::Printf(TEXT("ResolvePropertyPath: cannot descend into null object at '%s'"), *Name);
                    return false;
                }
                CurrentStruct = InnerObject->GetClass();
                CurrentContainer = static_cast<void*>(InnerObject);
            }
            else
            {
                OutError = FString::Printf(TEXT("ResolvePropertyPath: '%s' is not a struct or object, cannot descend further"), *Name);
                return false;
            }
        }
        else if (!Remaining.IsEmpty())
        {
            OutError = FString::Printf(TEXT("ResolvePropertyPath: unexpected token at '%s'"), *Remaining);
            return false;
        }
    }

    OutProperty = CurrentProperty;
    OutValuePtr = CurrentValuePtr;
    return true;
}

bool FUnrealMCPCommonUtils::SetObjectProperty(UObject* Object, const FString& PropertyName,
                                     const TSharedPtr<FJsonValue>& Value, FString& OutErrorMessage)
{
    if (!Object)
    {
        OutErrorMessage = TEXT("Invalid object");
        return false;
    }

    // Resolve the (possibly nested) property path. Supports simple names, "Foo.Bar",
    // "Foo[1]", "Foo[1].Bar", and combinations.
    FProperty* Property = nullptr;
    void* PropertyAddr = nullptr;
    if (!ResolvePropertyPath(Object, PropertyName, Property, PropertyAddr, OutErrorMessage))
    {
        return false;
    }
    
    // Handle different property types
    if (Property->IsA<FBoolProperty>())
    {
        ((FBoolProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsBool());
        return true;
    }
    else if (Property->IsA<FIntProperty>())
    {
        int32 IntValue = static_cast<int32>(Value->AsNumber());
        FIntProperty* IntProperty = CastField<FIntProperty>(Property);
        if (IntProperty)
        {
            // Use address-based API, not _InContainer, so it works for nested/array element paths
            IntProperty->SetPropertyValue(PropertyAddr, IntValue);
            return true;
        }
    }
    else if (Property->IsA<FFloatProperty>())
    {
        ((FFloatProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsNumber());
        return true;
    }
    else if (Property->IsA<FStrProperty>())
    {
        ((FStrProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsString());
        return true;
    }
    else if (Property->IsA<FByteProperty>())
    {
        FByteProperty* ByteProp = CastField<FByteProperty>(Property);
        UEnum* EnumDef = ByteProp ? ByteProp->GetIntPropertyEnum() : nullptr;
        
        // If this is a TEnumAsByte property (has associated enum)
        if (EnumDef)
        {
            // Handle numeric value
            if (Value->Type == EJson::Number)
            {
                uint8 ByteValue = static_cast<uint8>(Value->AsNumber());
                ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
                
                UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric value: %d"), 
                      *PropertyName, ByteValue);
                return true;
            }
            // Handle string enum value
            else if (Value->Type == EJson::String)
            {
                FString EnumValueName = Value->AsString();
                
                // Try to convert numeric string to number first
                if (EnumValueName.IsNumeric())
                {
                    uint8 ByteValue = FCString::Atoi(*EnumValueName);
                    ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric string value: %s -> %d"), 
                          *PropertyName, *EnumValueName, ByteValue);
                    return true;
                }
                
                // Handle qualified enum names (e.g., "Player0" or "EAutoReceiveInput::Player0")
                if (EnumValueName.Contains(TEXT("::")))
                {
                    EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName);
                }
                
                int64 EnumValue = EnumDef->GetValueByNameString(EnumValueName);
                if (EnumValue == INDEX_NONE)
                {
                    // Try with full name as fallback
                    EnumValue = EnumDef->GetValueByNameString(Value->AsString());
                }
                
                if (EnumValue != INDEX_NONE)
                {
                    ByteProp->SetPropertyValue(PropertyAddr, static_cast<uint8>(EnumValue));
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to name value: %s -> %lld"), 
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }
                else
                {
                    // Log all possible enum values for debugging
                    UE_LOG(LogTemp, Warning, TEXT("Could not find enum value for '%s'. Available options:"), *EnumValueName);
                    for (int32 i = 0; i < EnumDef->NumEnums(); i++)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("  - %s (value: %d)"), 
                               *EnumDef->GetNameStringByIndex(i), EnumDef->GetValueByIndex(i));
                    }
                    
                    OutErrorMessage = FString::Printf(TEXT("Could not find enum value for '%s'"), *EnumValueName);
                    return false;
                }
            }
        }
        else
        {
            // Regular byte property
            uint8 ByteValue = static_cast<uint8>(Value->AsNumber());
            ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
            return true;
        }
    }
    else if (Property->IsA<FEnumProperty>())
    {
        FEnumProperty* EnumProp = CastField<FEnumProperty>(Property);
        UEnum* EnumDef = EnumProp ? EnumProp->GetEnum() : nullptr;
        FNumericProperty* UnderlyingNumericProp = EnumProp ? EnumProp->GetUnderlyingProperty() : nullptr;
        
        if (EnumDef && UnderlyingNumericProp)
        {
            // Handle numeric value
            if (Value->Type == EJson::Number)
            {
                int64 EnumValue = static_cast<int64>(Value->AsNumber());
                UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
                
                UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric value: %lld"), 
                      *PropertyName, EnumValue);
                return true;
            }
            // Handle string enum value
            else if (Value->Type == EJson::String)
            {
                FString EnumValueName = Value->AsString();
                
                // Try to convert numeric string to number first
                if (EnumValueName.IsNumeric())
                {
                    int64 EnumValue = FCString::Atoi64(*EnumValueName);
                    UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric string value: %s -> %lld"), 
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }
                
                // Handle qualified enum names
                if (EnumValueName.Contains(TEXT("::")))
                {
                    EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName);
                }
                
                int64 EnumValue = EnumDef->GetValueByNameString(EnumValueName);
                if (EnumValue == INDEX_NONE)
                {
                    // Try with full name as fallback
                    EnumValue = EnumDef->GetValueByNameString(Value->AsString());
                }
                
                if (EnumValue != INDEX_NONE)
                {
                    UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to name value: %s -> %lld"), 
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }
                else
                {
                    // Log all possible enum values for debugging
                    UE_LOG(LogTemp, Warning, TEXT("Could not find enum value for '%s'. Available options:"), *EnumValueName);
                    for (int32 i = 0; i < EnumDef->NumEnums(); i++)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("  - %s (value: %d)"), 
                               *EnumDef->GetNameStringByIndex(i), EnumDef->GetValueByIndex(i));
                    }
                    
                    OutErrorMessage = FString::Printf(TEXT("Could not find enum value for '%s'"), *EnumValueName);
                    return false;
                }
            }
        }
    }
    else if (Property->IsA<FStructProperty>())
    {
        // FRotator, FVector, FColor, FLinearColor, FTransform, etc. — anything with a
        // string text representation. Caller must pass the value as a string in Unreal's
        // struct-text format, e.g. "(Pitch=0,Yaw=90,Roll=0)" for FRotator.
        FString StringValue;
        if (Value->Type == EJson::String)
        {
            StringValue = Value->AsString();
        }
        else
        {
            OutErrorMessage = FString::Printf(
                TEXT("StructProperty %s requires a string value in Unreal struct text format (e.g. \"(Pitch=0,Yaw=90,Roll=0)\")"),
                *PropertyName);
            return false;
        }

        const TCHAR* ImportResult = Property->ImportText_Direct(*StringValue, PropertyAddr, Object, PPF_None);
        if (ImportResult == nullptr)
        {
            OutErrorMessage = FString::Printf(
                TEXT("Failed to parse struct value '%s' for property %s (struct: %s)"),
                *StringValue, *PropertyName, *CastField<FStructProperty>(Property)->Struct->GetName());
            return false;
        }
        return true;
    }
    else if (Property->IsA<FObjectProperty>())
    {
        // Object reference (e.g. TSubclassOf<...>, TObjectPtr<UClass*>, asset references).
        // Accept either a raw asset path or the full Object'/Path' format.
        FString StringValue;
        if (Value->Type == EJson::String)
        {
            StringValue = Value->AsString();
        }
        else
        {
            OutErrorMessage = FString::Printf(
                TEXT("ObjectProperty %s requires a string asset path or Object'/Path' value"),
                *PropertyName);
            return false;
        }

        // If user passed a raw path, wrap it in the appropriate Object'...' / Class'...' format
        if (!StringValue.Contains(TEXT("'")) && (StringValue.StartsWith(TEXT("/Game")) || StringValue.StartsWith(TEXT("/Engine")) || StringValue.StartsWith(TEXT("/Script"))))
        {
            UObject* LoadedObj = LoadObject<UObject>(nullptr, *StringValue);
            if (LoadedObj)
            {
                StringValue = FString::Printf(TEXT("%s'%s'"), *LoadedObj->GetClass()->GetName(), *LoadedObj->GetPathName());
            }
        }

        const TCHAR* ImportResult = Property->ImportText_Direct(*StringValue, PropertyAddr, Object, PPF_None);
        if (ImportResult == nullptr)
        {
            OutErrorMessage = FString::Printf(TEXT("Failed to parse object reference '%s' for property %s"), *StringValue, *PropertyName);
            return false;
        }
        return true;
    }

    OutErrorMessage = FString::Printf(TEXT("Unsupported property type: %s for property %s"),
                                    *Property->GetClass()->GetName(), *PropertyName);
    return false;
}