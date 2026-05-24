#include "Commands/UnrealMCPEditorCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "ImageUtils.h"
#include "HighResScreenshot.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "Misc/StringOutputDevice.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "EditorSubsystem.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EditorAssetLibrary.h"

FUnrealMCPEditorCommands::FUnrealMCPEditorCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // Actor manipulation commands
    if (CommandType == TEXT("get_actors_in_level"))
    {
        return HandleGetActorsInLevel(Params);
    }
    else if (CommandType == TEXT("find_actors_by_name"))
    {
        return HandleFindActorsByName(Params);
    }
    else if (CommandType == TEXT("spawn_actor") || CommandType == TEXT("create_actor"))
    {
        if (CommandType == TEXT("create_actor"))
        {
            UE_LOG(LogTemp, Warning, TEXT("'create_actor' command is deprecated and will be removed in a future version. Please use 'spawn_actor' instead."));
        }
        return HandleSpawnActor(Params);
    }
    else if (CommandType == TEXT("delete_actor"))
    {
        return HandleDeleteActor(Params);
    }
    else if (CommandType == TEXT("set_actor_transform"))
    {
        return HandleSetActorTransform(Params);
    }
    else if (CommandType == TEXT("get_actor_properties"))
    {
        return HandleGetActorProperties(Params);
    }
    else if (CommandType == TEXT("set_actor_property"))
    {
        return HandleSetActorProperty(Params);
    }
    // Blueprint actor spawning
    else if (CommandType == TEXT("spawn_blueprint_actor"))
    {
        return HandleSpawnBlueprintActor(Params);
    }
    // Editor viewport commands
    else if (CommandType == TEXT("focus_viewport"))
    {
        return HandleFocusViewport(Params);
    }
    else if (CommandType == TEXT("take_screenshot"))
    {
        return HandleTakeScreenshot(Params);
    }
    // Editor automation commands
    else if (CommandType == TEXT("run_console_command"))
    {
        return HandleRunConsoleCommand(Params);
    }
    else if (CommandType == TEXT("save_asset"))
    {
        return HandleSaveAsset(Params);
    }
    else if (CommandType == TEXT("duplicate_asset"))
    {
        return HandleDuplicateAsset(Params);
    }
    else if (CommandType == TEXT("rename_asset"))
    {
        return HandleRenameAsset(Params);
    }
    else if (CommandType == TEXT("delete_asset"))
    {
        return HandleDeleteAssetCommand(Params);
    }
    else if (CommandType == TEXT("run_python_in_unreal"))
    {
        return HandleRunPythonInUnreal(Params);
    }
    else if (CommandType == TEXT("search_output_log"))
    {
        return HandleSearchOutputLog(Params);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown editor command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorsInLevel(const TSharedPtr<FJsonObject>& Params)
{
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> ActorArray;
    for (AActor* Actor : AllActors)
    {
        if (Actor)
        {
            ActorArray.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), ActorArray);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindActorsByName(const TSharedPtr<FJsonObject>& Params)
{
    FString Pattern;
    if (!Params->TryGetStringField(TEXT("pattern"), Pattern))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pattern' parameter"));
    }
    
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> MatchingActors;
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName().Contains(Pattern))
        {
            MatchingActors.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), MatchingActors);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorType;
    if (!Params->TryGetStringField(TEXT("type"), ActorType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
    }

    // Get actor name (required parameter)
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Get optional transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Create the actor based on type
    AActor* NewActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    // Check if an actor with this name already exists
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor with name '%s' already exists"), *ActorName));
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;

    if (ActorType == TEXT("StaticMeshActor"))
    {
        NewActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("PointLight"))
    {
        NewActor = World->SpawnActor<APointLight>(APointLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("SpotLight"))
    {
        NewActor = World->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("DirectionalLight"))
    {
        NewActor = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("CameraActor"))
    {
        NewActor = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown actor type: %s"), *ActorType));
    }

    if (NewActor)
    {
        // Set scale (since SpawnActor only takes location and rotation)
        FTransform Transform = NewActor->GetTransform();
        Transform.SetScale3D(Scale);
        NewActor->SetActorTransform(Transform);

        // Return the created actor's details
        return FUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create actor"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleDeleteActor(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            // Store actor info before deletion for the response
            TSharedPtr<FJsonObject> ActorInfo = FUnrealMCPCommonUtils::ActorToJsonObject(Actor);
            
            // Delete the actor
            Actor->Destroy();
            
            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetObjectField(TEXT("deleted_actor"), ActorInfo);
            return ResultObj;
        }
    }
    
    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get transform parameters
    FTransform NewTransform = TargetActor->GetTransform();

    if (Params->HasField(TEXT("location")))
    {
        NewTransform.SetLocation(FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        NewTransform.SetRotation(FQuat(FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"))));
    }
    if (Params->HasField(TEXT("scale")))
    {
        NewTransform.SetScale3D(FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
    }

    // Set the new transform
    TargetActor->SetActorTransform(NewTransform);

    // Return updated actor info
    return FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Always return detailed properties for this command
    return FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get property name
    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    // Get property value
    if (!Params->HasField(TEXT("property_value")))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
    }
    
    TSharedPtr<FJsonValue> PropertyValue = Params->Values.FindRef(TEXT("property_value"));
    
    // Set the property using our utility function
    FString ErrorMessage;
    if (FUnrealMCPCommonUtils::SetObjectProperty(TargetActor, PropertyName, PropertyValue, ErrorMessage))
    {
        // Property set successfully
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("actor"), ActorName);
        ResultObj->SetStringField(TEXT("property"), PropertyName);
        ResultObj->SetBoolField(TEXT("success"), true);
        
        // Also include the full actor details
        ResultObj->SetObjectField(TEXT("actor_details"), FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true));
        return ResultObj;
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(ErrorMessage);
    }
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    // Find the blueprint
    if (BlueprintName.IsEmpty())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint name is empty"));
    }

    FString Root      = TEXT("/Game/Blueprints/");
    FString AssetPath = Root + BlueprintName;

    if (!FPackageName::DoesPackageExist(AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found – it must reside under /Game/Blueprints"), *BlueprintName));
    }

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Spawn the actor
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    FTransform SpawnTransform;
    SpawnTransform.SetLocation(Location);
    SpawnTransform.SetRotation(FQuat(Rotation));
    SpawnTransform.SetScale3D(Scale);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;

    AActor* NewActor = World->SpawnActor<AActor>(Blueprint->GeneratedClass, SpawnTransform, SpawnParams);
    if (NewActor)
    {
        return FUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn blueprint actor"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFocusViewport(const TSharedPtr<FJsonObject>& Params)
{
    // Get target actor name if provided
    FString TargetActorName;
    bool HasTargetActor = Params->TryGetStringField(TEXT("target"), TargetActorName);

    // Get location if provided
    FVector Location(0.0f, 0.0f, 0.0f);
    bool HasLocation = false;
    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
        HasLocation = true;
    }

    // Get distance
    float Distance = 1000.0f;
    if (Params->HasField(TEXT("distance")))
    {
        Distance = Params->GetNumberField(TEXT("distance"));
    }

    // Get orientation if provided
    FRotator Orientation(0.0f, 0.0f, 0.0f);
    bool HasOrientation = false;
    if (Params->HasField(TEXT("orientation")))
    {
        Orientation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("orientation"));
        HasOrientation = true;
    }

    // Get the active viewport
    FLevelEditorViewportClient* ViewportClient = (FLevelEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
    if (!ViewportClient)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get active viewport"));
    }

    // If we have a target actor, focus on it
    if (HasTargetActor)
    {
        // Find the actor
        AActor* TargetActor = nullptr;
        TArray<AActor*> AllActors;
        UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
        
        for (AActor* Actor : AllActors)
        {
            if (Actor && Actor->GetName() == TargetActorName)
            {
                TargetActor = Actor;
                break;
            }
        }

        if (!TargetActor)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *TargetActorName));
        }

        // Focus on the actor
        ViewportClient->SetViewLocation(TargetActor->GetActorLocation() - FVector(Distance, 0.0f, 0.0f));
    }
    // Otherwise use the provided location
    else if (HasLocation)
    {
        ViewportClient->SetViewLocation(Location - FVector(Distance, 0.0f, 0.0f));
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Either 'target' or 'location' must be provided"));
    }

    // Set orientation if provided
    if (HasOrientation)
    {
        ViewportClient->SetViewRotation(Orientation);
    }

    // Force viewport to redraw
    ViewportClient->Invalidate();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleTakeScreenshot(const TSharedPtr<FJsonObject>& Params)
{
    // Get file path parameter
    FString FilePath;
    if (!Params->TryGetStringField(TEXT("filepath"), FilePath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'filepath' parameter"));
    }
    
    // Ensure the file path has a proper extension
    if (!FilePath.EndsWith(TEXT(".png")))
    {
        FilePath += TEXT(".png");
    }

    // Get the active viewport
    if (GEditor && GEditor->GetActiveViewport())
    {
        FViewport* Viewport = GEditor->GetActiveViewport();
        TArray<FColor> Bitmap;
        FIntRect ViewportRect(0, 0, Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y);
        
        if (Viewport->ReadPixels(Bitmap, FReadSurfaceDataFlags(), ViewportRect))
        {
            TArray<uint8> CompressedBitmap;
            FImageUtils::CompressImageArray(Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y, Bitmap, CompressedBitmap);
            
            if (FFileHelper::SaveArrayToFile(CompressedBitmap, *FilePath))
            {
                TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
                ResultObj->SetStringField(TEXT("filepath"), FilePath);
                return ResultObj;
            }
        }
    }
    
    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to take screenshot"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleRunConsoleCommand(const TSharedPtr<FJsonObject>& Params)
{
    FString Command;
    if (!Params->TryGetStringField(TEXT("command"), Command))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'command' parameter"));
    }

    if (Command.IsEmpty())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Command string is empty"));
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    FStringOutputDevice StringOutput;
    bool bSuccess = GEngine->Exec(World, *Command, StringOutput);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("command"), Command);
    ResultObj->SetStringField(TEXT("output"), *StringOutput);
    ResultObj->SetBoolField(TEXT("success"), bSuccess);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSaveAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    bool bOnlyIfDirty = true;
    if (Params->HasField(TEXT("only_if_dirty")))
    {
        bOnlyIfDirty = Params->GetBoolField(TEXT("only_if_dirty"));
    }

    bool bSaved = UEditorAssetLibrary::SaveAsset(AssetPath, bOnlyIfDirty);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("asset_path"), AssetPath);
    ResultObj->SetBoolField(TEXT("saved"), bSaved);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleDuplicateAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString SourcePath;
    if (!Params->TryGetStringField(TEXT("source_path"), SourcePath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_path' parameter"));
    }

    FString DestinationPath;
    if (!Params->TryGetStringField(TEXT("destination_path"), DestinationPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'destination_path' parameter"));
    }

    UObject* DuplicatedObject = UEditorAssetLibrary::DuplicateAsset(SourcePath, DestinationPath);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("source_path"), SourcePath);
    ResultObj->SetStringField(TEXT("destination_path"), DestinationPath);
    if (DuplicatedObject)
    {
        ResultObj->SetStringField(TEXT("duplicated_path"), DestinationPath);
        ResultObj->SetBoolField(TEXT("success"), true);
    }
    else
    {
        ResultObj->SetBoolField(TEXT("success"), false);
        ResultObj->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to duplicate asset from '%s' to '%s'"), *SourcePath, *DestinationPath));
    }
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleRenameAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString SourcePath;
    if (!Params->TryGetStringField(TEXT("source_path"), SourcePath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_path' parameter"));
    }

    FString DestinationPath;
    if (!Params->TryGetStringField(TEXT("destination_path"), DestinationPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'destination_path' parameter"));
    }

    // If destination doesn't start with '/', treat it as a name in the same directory
    if (!DestinationPath.StartsWith(TEXT("/")))
    {
        DestinationPath = FPaths::GetPath(SourcePath) / DestinationPath;
    }

    bool bRenamed = UEditorAssetLibrary::RenameAsset(SourcePath, DestinationPath);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("source_path"), SourcePath);
    ResultObj->SetStringField(TEXT("destination_path"), DestinationPath);
    ResultObj->SetBoolField(TEXT("success"), bRenamed);
    if (!bRenamed)
    {
        ResultObj->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to rename asset from '%s' to '%s'"), *SourcePath, *DestinationPath));
    }
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleDeleteAssetCommand(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
    }

    // Close any editor that has this asset open (prevents TickTaskLevel crash)
    UObject* AssetObj = UEditorAssetLibrary::LoadAsset(AssetPath);
    if (AssetObj)
    {
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllEditorsForAsset(AssetObj);
        // Force GC to detach ticking components before deletion
        CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
    }

    bool bDeleted = UEditorAssetLibrary::DeleteAsset(AssetPath);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("asset_path"), AssetPath);
    ResultObj->SetBoolField(TEXT("deleted"), bDeleted);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleRunPythonInUnreal(const TSharedPtr<FJsonObject>& Params)
{
    FString PythonCode;
    if (!Params->TryGetStringField(TEXT("code"), PythonCode))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'code' parameter"));
    }

    if (PythonCode.IsEmpty())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Python code is empty"));
    }

    // Set up temp file paths
    FString TempDir = FPaths::ProjectSavedDir() / TEXT("MCP");
    IFileManager::Get().MakeDirectory(*TempDir, true);
    FString UserCodePath = TempDir / TEXT("mcp_user_code.py");
    FString WrapperPath = TempDir / TEXT("mcp_wrapper.py");
    FString OutputPath = TempDir / TEXT("mcp_output.json");

    // Clean up old output
    IFileManager::Get().Delete(*OutputPath);

    // Write user code to file (avoids all quoting issues)
    FFileHelper::SaveStringToFile(PythonCode, *UserCodePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    // Write wrapper that captures stdout/stderr and writes result to JSON
    FString OutputPathEscaped = OutputPath.Replace(TEXT("\\"), TEXT("/"));
    FString UserCodePathEscaped = UserCodePath.Replace(TEXT("\\"), TEXT("/"));
    FString WrapperCode = FString::Printf(TEXT(
        "import sys, io, json, traceback\n"
        "_mcp_stdout = io.StringIO()\n"
        "_mcp_stderr = io.StringIO()\n"
        "_mcp_old_stdout = sys.stdout\n"
        "_mcp_old_stderr = sys.stderr\n"
        "sys.stdout = _mcp_stdout\n"
        "sys.stderr = _mcp_stderr\n"
        "_mcp_error = None\n"
        "try:\n"
        "    with open(r'%s', 'r', encoding='utf-8') as _mcp_f:\n"
        "        exec(compile(_mcp_f.read(), 'mcp_user_code.py', 'exec'))\n"
        "except Exception:\n"
        "    _mcp_error = traceback.format_exc()\n"
        "finally:\n"
        "    sys.stdout = _mcp_old_stdout\n"
        "    sys.stderr = _mcp_old_stderr\n"
        "with open(r'%s', 'w', encoding='utf-8') as _mcp_f:\n"
        "    json.dump({\n"
        "        'output': _mcp_stdout.getvalue(),\n"
        "        'errors': _mcp_stderr.getvalue(),\n"
        "        'exception': _mcp_error\n"
        "    }, _mcp_f)\n"
    ), *UserCodePathEscaped, *OutputPathEscaped);

    FFileHelper::SaveStringToFile(WrapperCode, *WrapperPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    // Execute via UE's py console command
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    FString PyCommand = FString::Printf(TEXT("py \"%s\""), *WrapperPath);
    FStringOutputDevice ConsoleOutput;
    GEngine->Exec(World, *PyCommand, ConsoleOutput);

    // Read the output JSON
    FString OutputJson;
    if (!FFileHelper::LoadFileToString(OutputJson, *OutputPath))
    {
        // No output file means Python didn't execute at all
        FString ConsoleStr = ConsoleOutput.IsEmpty() ? TEXT("(none)") : *ConsoleOutput;
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Python execution failed. Is the Python Editor Script Plugin enabled? Console output: %s"), *ConsoleStr));
    }

    // Parse the output
    TSharedPtr<FJsonObject> OutputObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(OutputJson);
    if (!FJsonSerializer::Deserialize(Reader, OutputObj) || !OutputObj.IsValid())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to parse Python output JSON"));
    }

    FString Output = OutputObj->GetStringField(TEXT("output"));
    FString Errors;
    OutputObj->TryGetStringField(TEXT("errors"), Errors);
    FString Exception;
    OutputObj->TryGetStringField(TEXT("exception"), Exception);

    // Build result
    TSharedPtr<FJsonObject> ResultObj = FUnrealMCPCommonUtils::CreateSuccessResponse();
    ResultObj->SetStringField(TEXT("output"), Output);
    if (!Errors.IsEmpty())
    {
        ResultObj->SetStringField(TEXT("errors"), Errors);
    }
    if (!Exception.IsEmpty())
    {
        ResultObj->SetStringField(TEXT("exception"), Exception);
        ResultObj->SetStringField(TEXT("status"), TEXT("error"));
    }

    // Cleanup temp files
    IFileManager::Get().Delete(*UserCodePath);
    IFileManager::Get().Delete(*WrapperPath);
    IFileManager::Get().Delete(*OutputPath);

    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSearchOutputLog(const TSharedPtr<FJsonObject>& Params)
{
    FString Pattern;
    if (!Params->TryGetStringField(TEXT("pattern"), Pattern))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pattern'"));

    int32 MaxResults = 50;
    Params->TryGetNumberField(TEXT("max_results"), MaxResults);
    MaxResults = FMath::Clamp(MaxResults, 1, 500);

    bool bCaseSensitive = false;
    Params->TryGetBoolField(TEXT("case_sensitive"), bCaseSensitive);

    // Find the current log file
    FString OutputLogPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectLogDir() / FApp::GetProjectName() + TEXT(".log"));
    if (!FPaths::FileExists(OutputLogPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Log file not found: %s"), *OutputLogPath));
    }

    GLog->Flush();

    TArray<FString> Lines;
    FString LogContent;
    // Use CreateFileReader for shared-read access (editor holds log open for writing)
    TUniquePtr<FArchive> Reader(IFileManager::Get().CreateFileReader(*OutputLogPath, FILEREAD_AllowWrite));
    if (Reader)
    {
        int64 Size = Reader->TotalSize();
        TArray<uint8> RawData;
        RawData.SetNumUninitialized(Size);
        Reader->Serialize(RawData.GetData(), Size);
        Reader->Close();
        FFileHelper::BufferToString(LogContent, RawData.GetData(), RawData.Num());
        LogContent.ParseIntoArrayLines(Lines, false);
    }

    TArray<TSharedPtr<FJsonValue>> Matches;
    // Search from end (most recent first)
    for (int32 i = Lines.Num() - 1; i >= 0 && Matches.Num() < MaxResults; --i)
    {
        bool bMatch = bCaseSensitive
            ? Lines[i].Contains(Pattern, ESearchCase::CaseSensitive)
            : Lines[i].Contains(Pattern, ESearchCase::IgnoreCase);
        if (bMatch)
        {
            TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
            Entry->SetNumberField(TEXT("line"), i + 1);
            Entry->SetStringField(TEXT("text"), Lines[i]);
            Matches.Insert(MakeShared<FJsonValueObject>(Entry), 0);
        }
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("log_path"), OutputLogPath);
    Result->SetNumberField(TEXT("total_lines"), Lines.Num());
    Result->SetNumberField(TEXT("match_count"), Matches.Num());
    Result->SetArrayField(TEXT("matches"), Matches);
    return Result;
}