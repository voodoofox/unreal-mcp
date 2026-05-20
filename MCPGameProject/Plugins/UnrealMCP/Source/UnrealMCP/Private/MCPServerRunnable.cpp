#include "MCPServerRunnable.h"
#include "UnrealMCPBridge.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "JsonObjectConverter.h"
#include "Misc/ScopeLock.h"
#include "HAL/PlatformTime.h"

static constexpr int32 SocketBufferSize = 1024 * 1024;
static constexpr int32 RecvChunkSize    = 65536;
static constexpr int32 MaxClientBuffer  = 16 * 1024 * 1024;
static constexpr int32 SendChunkSize    = 65536;
static constexpr double SendTimeoutSec  = 30.0;

static bool SendAll(FSocket* Sock, const uint8* Data, int32 TotalBytes)
{
	int32 Offset = 0;
	double Deadline = FPlatformTime::Seconds() + SendTimeoutSec;

	while (Offset < TotalBytes)
	{
		int32 ChunkSize = FMath::Min(SendChunkSize, TotalBytes - Offset);
		int32 BytesSent = 0;
		bool bOk = Sock->Send(Data + Offset, ChunkSize, BytesSent);

		if (bOk && BytesSent > 0)
		{
			Offset += BytesSent;
			continue;
		}

		// Non-blocking socket: buffer full (EWOULDBLOCK) — retry after short sleep
		if (FPlatformTime::Seconds() > Deadline)
		{
			UE_LOG(LogTemp, Warning, TEXT("MCP SendAll: timed out after %.0fs, sent %d/%d bytes"), SendTimeoutSec, Offset, TotalBytes);
			return false;
		}

		FPlatformProcess::Sleep(0.005f);
	}
	return true;
}

FMCPServerRunnable::FMCPServerRunnable(UUnrealMCPBridge* InBridge, TSharedPtr<FSocket> InListenerSocket)
    : Bridge(InBridge)
    , ListenerSocket(InListenerSocket)
    , bRunning(true)
{
    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Created server runnable"));
}

FMCPServerRunnable::~FMCPServerRunnable()
{
}

bool FMCPServerRunnable::Init()
{
    return true;
}

bool FMCPServerRunnable::TickClient(int32 ClientIndex)
{
    TSharedPtr<FSocket> Sock = ClientSockets[ClientIndex];
    if (!Sock.IsValid())
        return false;

    uint32 PendingSize = 0;
    if (!Sock->HasPendingData(PendingSize) || PendingSize == 0)
        return true;

    // Read into the persistent raw byte buffer, then scan for complete UTF8 lines
    uint8 TempBuf[RecvChunkSize];
    int32 BytesRead = 0;
    if (!Sock->Recv(TempBuf, sizeof(TempBuf), BytesRead) || BytesRead == 0)
        return false;

    TArray<uint8>& RawBuf = ClientRawBuffers[ClientIndex];
    RawBuf.Append(TempBuf, BytesRead);

    if (RawBuf.Num() > MaxClientBuffer)
    {
        UE_LOG(LogTemp, Warning, TEXT("MCP[%d]: Client buffer exceeded %d bytes, dropping connection"), ClientIndex, MaxClientBuffer);
        return false;
    }

    // Process complete newline-terminated messages from raw bytes
    for (;;)
    {
        int32 NewlinePos = INDEX_NONE;
        for (int32 i = 0; i < RawBuf.Num(); i++)
        {
            if (RawBuf[i] == '\n')
            {
                NewlinePos = i;
                break;
            }
        }
        if (NewlinePos == INDEX_NONE)
            break;

        // Null-terminate at newline to convert only this line's bytes
        uint8 Saved = RawBuf[NewlinePos];
        RawBuf[NewlinePos] = 0;
        FString Message = UTF8_TO_TCHAR((const char*)RawBuf.GetData());
        RawBuf[NewlinePos] = Saved;

        RawBuf.RemoveAt(0, NewlinePos + 1, EAllowShrinking::No);

        if (Message.IsEmpty()) continue;

        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
        if (!FJsonSerializer::Deserialize(Reader, JsonObject))
        {
            UE_LOG(LogTemp, Warning, TEXT("MCP[%d]: Failed to parse JSON"), ClientIndex);
            continue;
        }

        FString CommandType;
        TSharedPtr<FJsonObject> Params;

        if (JsonObject->TryGetStringField(TEXT("type"), CommandType) ||
            JsonObject->TryGetStringField(TEXT("command"), CommandType))
        {
            if (JsonObject->HasField(TEXT("params")))
            {
                const TSharedPtr<FJsonObject>* ParamsObj = nullptr;
                if (JsonObject->TryGetObjectField(TEXT("params"), ParamsObj) && ParamsObj)
                {
                    Params = *ParamsObj;
                }
                else
                {
                    Params = MakeShared<FJsonObject>();
                }
            }
            else
            {
                Params = MakeShared<FJsonObject>();
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("MCP[%d]: Missing 'type'/'command' field"), ClientIndex);
            continue;
        }

        UE_LOG(LogTemp, Display, TEXT("MCP[%d]: %s"), ClientIndex, *CommandType);

        FString Response = Bridge->ExecuteCommand(CommandType, Params);
        FString ResponseLine = Response + TEXT("\n");

        FTCHARToUTF8 Utf8(*ResponseLine);
        int32 Utf8Len = Utf8.Length();

        if (!SendAll(Sock.Get(), (const uint8*)Utf8.Get(), Utf8Len))
        {
            UE_LOG(LogTemp, Warning, TEXT("MCP[%d]: Failed to send response (%d bytes)"), ClientIndex, Utf8Len);
            return false;
        }
    }

    return true;
}

uint32 FMCPServerRunnable::Run()
{
    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Server thread starting (multi-client)..."));

    ListenerSocket->SetNonBlocking(true);

    while (bRunning)
    {
        bool bPending = false;
        if (ListenerSocket->HasPendingConnection(bPending) && bPending)
        {
            TSharedPtr<FSocket> NewClient = MakeShareable(ListenerSocket->Accept(TEXT("MCPClient")));
            if (NewClient.IsValid())
            {
                NewClient->SetNoDelay(true);
                NewClient->SetNonBlocking(true);
                int32 ActualSendBuf = 0, ActualRecvBuf = 0;
                NewClient->SetSendBufferSize(SocketBufferSize, ActualSendBuf);
                NewClient->SetReceiveBufferSize(SocketBufferSize, ActualRecvBuf);

                FScopeLock Lock(&ClientsMutex);
                ClientSockets.Add(NewClient);
                ClientRawBuffers.AddDefaulted();
                UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Client %d connected (total: %d, send_buf: %d, recv_buf: %d)"),
                    ClientSockets.Num() - 1, ClientSockets.Num(), ActualSendBuf, ActualRecvBuf);
            }
        }

        {
            FScopeLock Lock(&ClientsMutex);
            for (int32 i = ClientSockets.Num() - 1; i >= 0; i--)
            {
                if (!TickClient(i))
                {
                    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Client %d disconnected (remaining: %d)"),
                        i, ClientSockets.Num() - 1);
                    ClientSockets[i]->Close();
                    ClientSockets.RemoveAt(i);
                    ClientRawBuffers.RemoveAt(i);
                }
            }
        }

        FPlatformProcess::Sleep(0.01f);
    }

    {
        FScopeLock Lock(&ClientsMutex);
        for (auto& Sock : ClientSockets)
        {
            if (Sock.IsValid()) Sock->Close();
        }
        ClientSockets.Empty();
        ClientRawBuffers.Empty();
    }

    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Server thread stopping"));
    return 0;
}

void FMCPServerRunnable::Stop()
{
    bRunning = false;
}

void FMCPServerRunnable::Exit()
{
}
