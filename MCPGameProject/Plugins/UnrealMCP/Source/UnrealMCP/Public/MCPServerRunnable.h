#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "Sockets.h"
#include "Interfaces/IPv4/IPv4Address.h"

class UUnrealMCPBridge;

class FMCPServerRunnable : public FRunnable
{
public:
	FMCPServerRunnable(UUnrealMCPBridge* InBridge, TSharedPtr<FSocket> InListenerSocket);
	virtual ~FMCPServerRunnable();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

protected:
	bool TickClient(int32 ClientIndex);

private:
	UUnrealMCPBridge* Bridge;
	TSharedPtr<FSocket> ListenerSocket;

	TArray<TSharedPtr<FSocket>> ClientSockets;
	TArray<TArray<uint8>> ClientRawBuffers;
	FCriticalSection ClientsMutex;

	bool bRunning;
};
