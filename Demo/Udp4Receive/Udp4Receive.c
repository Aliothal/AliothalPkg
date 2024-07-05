#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/ServiceBinding.h>
#include <Protocol/Udp4.h>

typedef struct {
  EFI_HANDLE                  ChildHandle;
  EFI_UDP4_PROTOCOL           *Udp4;
  EFI_UDP4_CONFIG_DATA        ConfigData;
  EFI_UDP4_COMPLETION_TOKEN   TokenReceive;
  EFI_EVENT_NOTIFY            NotifyFunction;
} UDP4_SOCKET;

EFI_UDP4_PROTOCOL                     *gUdp4Protocol = NULL;
EFI_IPv4_ADDRESS                      StationAddress = {{0, 0, 0, 0}};
UINT16                                StationPort = 5566;
EFI_IPv4_ADDRESS                      SubnetMask = {{0, 0, 0, 0}};
EFI_IPv4_ADDRESS                      RemoteAddress = {{0, 0, 0, 0}};
UINT16                                RemotePort = 0;
EFI_UDP4_COMPLETION_TOKEN             Udp4RecvToken;
EFI_UDP4_COMPLETION_TOKEN             Udp4SendToken;
EFI_UDP4_TRANSMIT_DATA                Udp4TransmitData;
// EFI_UDP4_TRANSMIT_DATA                *gTxData;
EFI_UDP4_SESSION_DATA                 gUdpSessionData;
EFI_IPv4_ADDRESS                      gGatewayAddress = {{0, 0, 0, 0}};


EFI_UDP4_CONFIG_DATA  Udp4ConfigData = {
                      TRUE,  // AcceptBroadcast
                      FALSE,   // AcceptPromiscuous
                      FALSE,   // AcceptAnyPort
                      TRUE,   // AllowDuplicatePort
                      0,      // TypeOfService
                      16,      // TimeToLive
                      TRUE,   // DoNotFragment
                      0,      // ReceiveTimeout
                      0,      // TransmitTimeout
                      FALSE,  // UseDefaultAddress
                      {{0, 0, 0, 0}},  // StationAddress
                      {{0, 0, 0, 0}},  // SubnetMask
                      0,      // StationPort
                      {{0, 0, 0, 0}},  // RemoteAddress
                      0,      // RemotePort
                    };

VOID TimerNotify(IN EFI_EVENT  Event,  IN VOID *Context)
{
  gUdp4Protocol->Receive(gUdp4Protocol, &Udp4RecvToken);
}

EFI_STATUS Udp4SendMessage(UINT32 Length, CHAR8 *Message)
{
  Udp4SendToken.Packet.TxData = &Udp4TransmitData;
  Udp4TransmitData.UdpSessionData = &gUdpSessionData;

  // gBS->SetMem(&Udp4TransmitData.GatewayAddress, sizeof(Udp4TransmitData.GatewayAddress), 0x00);
  Udp4TransmitData.GatewayAddress = &gGatewayAddress;
  Udp4TransmitData.DataLength = Length;
  Udp4TransmitData.FragmentCount = 1;
  Udp4TransmitData.FragmentTable[0].FragmentLength = Udp4TransmitData.DataLength;
  Udp4TransmitData.FragmentTable[0].FragmentBuffer = Message;

  return gUdp4Protocol->Transmit(gUdp4Protocol, &Udp4SendToken);
}


VOID Udp4RecvNotify(IN EFI_EVENT  Event,  IN VOID *Context)
{
  EFI_UDP4_COMPLETION_TOKEN   *ReceiveToken = &Udp4RecvToken;
  EFI_UDP4_RECEIVE_DATA       *RxData = ReceiveToken->Packet.RxData;
  UINT32                      TotalLength = RxData->DataLength;
  UINT32                      FragmentCount = RxData->FragmentCount;
  CHAR8                       *Buffer = AllocateZeroPool(TotalLength + 1);
  UINT32                      BufferIndex = 0;
  for (UINT32 Index = 0; Index < FragmentCount; Index++) {
    UINT32                    FragmentLength = RxData->FragmentTable[Index].FragmentLength;
    CopyMem(Buffer + BufferIndex, RxData->FragmentTable[Index].FragmentBuffer, FragmentLength);
    BufferIndex += FragmentLength;
  }
  gUdpSessionData.SourceAddress = RxData->UdpSession.DestinationAddress;
  gUdpSessionData.SourcePort = RxData->UdpSession.DestinationPort;
  gUdpSessionData.DestinationAddress = RxData->UdpSession.SourceAddress;
  gUdpSessionData.DestinationPort = RxData->UdpSession.SourcePort;
  AsciiPrint("TotalLength:%d,Data:", TotalLength);
  for (UINT32 Index = 0; Index < TotalLength; Index++) {
    AsciiPrint("%c", Buffer[Index]);
  }
  AsciiPrint("\n");
  CmdExecute(Buffer);
  FreePool(Buffer);
}

VOID  Udp4SendNotify(IN EFI_EVENT  Event,  IN VOID *Context)
{

}

EFI_STATUS OpenUdp4Protocol()
{
  EFI_STATUS                          Status                          = EFI_SUCCESS;
  EFI_SERVICE_BINDING_PROTOCOL        *Udp4ServiceBindingProtocol     = NULL;
  EFI_HANDLE                          ChildHandle                     = NULL;
  Status = gBS->LocateProtocol(&gEfiUdp4ServiceBindingProtocolGuid, NULL, (VOID**)&Udp4ServiceBindingProtocol);
  if (Status == EFI_SUCCESS) {
    Status = Udp4ServiceBindingProtocol->CreateChild(Udp4ServiceBindingProtocol, &ChildHandle);
    
    Status = gBS->OpenProtocol(ChildHandle,
                              &gEfiUdp4ProtocolGuid,
                              (VOID**)&gUdp4Protocol,
                              gImageHandle,
                              ChildHandle,
                              EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(Status)) {
      AsciiPrint("Can not Open Udp4 Protocol! Status:%r\n", Status);
    }
  } else {
    AsciiPrint("Locate Udp4 Services failed! Status: %r\n", Status);
  }
  return Status;
}

EFI_STATUS ConfigureUdp4()
{
  EFI_STATUS    Status;
  Udp4ConfigData.StationAddress = StationAddress;
  Udp4ConfigData.StationPort = StationPort;
  Udp4ConfigData.SubnetMask = SubnetMask;
  Udp4ConfigData.RemoteAddress = RemoteAddress;
  Udp4ConfigData.RemotePort = RemotePort;
  Status = gUdp4Protocol->Configure(gUdp4Protocol, &Udp4ConfigData);
  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Udp4RecvNotify, NULL, &Udp4RecvToken.Event);
  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Udp4SendNotify, NULL, &Udp4SendToken.Event);
  return Status;
}


EFI_STATUS
Udp4ReceiveEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                        Status = EFI_SUCCESS;
  Status = OpenUdp4Protocol();
  Status = ConfigureUdp4();
  EFI_EVENT             MyEvent;
  Status = gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)TimerNotify, NULL, &MyEvent);
  Status = gBS->SetTimer(MyEvent, TimerPeriodic, 30000000);

  return EFI_SUCCESS;
}