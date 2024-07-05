#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/ServiceBinding.h>
#include <Protocol/Tcp4.h>

#define TCP_RECEIVE_DATA_LENGTH 20

EFI_SERVICE_BINDING_PROTOCOL                *gTcp4Service = NULL;
VOID                                        *gTcp4RecvBuffer1;

typedef struct {
  EFI_IPv4_ADDRESS              ClientIp;
  UINT16                        ClientPort;
  EFI_IPv4_ADDRESS              ServerIp;
  UINT16                        ServerPort;
} SOCKET_ADDR;

typedef struct {
  EFI_HANDLE                    AgentHandle;
  EFI_HANDLE                    ControllerHandle;
  EFI_TCP4_PROTOCOL             *Tcp4;
  EFI_TCP4_CONNECTION_TOKEN     ConnectToken;
	EFI_TCP4_CLOSE_TOKEN          CloseToken;
	EFI_TCP4_IO_TOKEN             SendToken;
  EFI_TCP4_IO_TOKEN             RecvToken;
} TCP4_SOCKET;

VOID Tcp4ConnectNotify(IN EFI_EVENT  Event,  IN VOID *Context)
{
}
VOID Tcp4CloseNotify(IN EFI_EVENT  Event,  IN VOID *Context)
{
}
VOID Tcp4SendNotify(IN EFI_EVENT  Event,  IN VOID *Context)
{
}
VOID Tcp4ReceiveNotify(IN EFI_EVENT  Event,  IN VOID *Context)
{
  EFI_STATUS                  Status = 0;
  EFI_TCP4_IO_TOKEN           *ReceiveToken = (EFI_TCP4_IO_TOKEN *)Context;
  EFI_TCP4_RECEIVE_DATA       *RxData = ReceiveToken->Packet.RxData;
  UINT8                       *RecvBuffer = AllocateZeroPool(RxData->DataLength + 1);
  CopyMem(RecvBuffer, RxData->FragmentTable[0].FragmentBuffer, RxData->DataLength);
  Status = ReceiveToken->CompletionToken.Status;
  AsciiPrint("\nrecv Status:%r, Data: ", Status);
  for (UINT32 i = 0; i < RxData->DataLength; i++) {
    AsciiPrint("%c-", RecvBuffer[i]);
  }
  FreePool(RecvBuffer);
}

VOID Tcp4RecvStart(IN EFI_EVENT  Event,  IN VOID *Context)
{
  EFI_STATUS                  Status = 0;
  TCP4_SOCKET                 *Socket = (TCP4_SOCKET *)Context;
  Socket->RecvToken.Packet.RxData->UrgentFlag = TRUE;
  Socket->RecvToken.Packet.RxData->DataLength = TCP_RECEIVE_DATA_LENGTH;
  Socket->RecvToken.Packet.RxData->FragmentCount = 1;
  Socket->RecvToken.Packet.RxData->FragmentTable[0].FragmentLength = TCP_RECEIVE_DATA_LENGTH;
  Socket->RecvToken.Packet.RxData->FragmentTable[0].FragmentBuffer = gTcp4RecvBuffer1;
  Status = Socket->Tcp4->Receive(Socket->Tcp4, &Socket->RecvToken);
}

EFI_STATUS SocketInit(TCP4_SOCKET *Socket)
{
  EFI_STATUS                Status = EFI_SUCCESS;
  Socket->ControllerHandle = NULL;
  Status = gTcp4Service->CreateChild(gTcp4Service, &Socket->ControllerHandle);
  Status = gBS->OpenProtocol(Socket->ControllerHandle,
                             &gEfiTcp4ProtocolGuid,
                             (VOID **)&Socket->Tcp4,
                             Socket->AgentHandle,
                             Socket->ControllerHandle,
                             EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4ConnectNotify, NULL, &Socket->ConnectToken.CompletionToken.Event);
  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4CloseNotify, NULL, &Socket->CloseToken.CompletionToken.Event);
  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4SendNotify, NULL, &Socket->SendToken.CompletionToken.Event);
  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4ReceiveNotify, (VOID *)&Socket, &Socket->RecvToken.CompletionToken.Event);
  return Status;
}

EFI_STATUS SocketConfig(TCP4_SOCKET *Socket, SOCKET_ADDR Addr)
{
  EFI_STATUS                Status = EFI_SUCCESS;
  EFI_TCP4_CONFIG_DATA      ConfigData = {
                                           0x00,                         //ToS
                                           0x10,                         //TTL
                                           {
                                             FALSE,                      //UseDefaultAddress
                                             Addr.ClientIp,              //StationAddress
                                             {
                                               {255, 255, 255, 255}      //SubnetMask
                                             },
                                             Addr.ClientPort,            //StationPort
                                             Addr.ServerIp,              //RemoteAddress
                                             Addr.ServerPort,            //RemotePort
                                             TRUE                        //ActiveFlag
                                           },
                                           NULL                          //ControlOption
                                         };

  Status = Socket->Tcp4->Configure(Socket->Tcp4, &ConfigData);

}

EFI_STATUS SocketConnect(TCP4_SOCKET *Socket)
{
  return Socket->Tcp4->Connect(Socket->Tcp4, &Socket->ConnectToken);
}

EFI_STATUS SocketRecv(TCP4_SOCKET *Socket)
{
  EFI_EVENT                 Tcp4ReceiveEvent;
  gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4RecvStart, (VOID *)&Socket->RecvToken, &Tcp4ReceiveEvent);
  return gBS->SetTimer(Tcp4ReceiveEvent, TimerPeriodic, 10000000);
}

EFI_STATUS
Tcp4ClientEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                                  Status = EFI_SUCCESS;
  TCP4_SOCKET                                 SocketHeart;
  SOCKET_ADDR                                 Addr = {{192,168,1,150},54545,{192,168,1,101},54545};

  Status = gBS->LocateProtocol(&gEfiTcp4ServiceBindingProtocolGuid, NULL, (VOID **)&gTcp4Service);
  gTcp4RecvBuffer1 = AllocateZeroPool(TCP_RECEIVE_DATA_LENGTH);
  SocketInit(&SocketHeart);
  SocketConfig(&SocketHeart,Addr);
  SocketConnect(&SocketHeart);
  SocketRecv(&SocketHeart);

  return EFI_SUCCESS;
}