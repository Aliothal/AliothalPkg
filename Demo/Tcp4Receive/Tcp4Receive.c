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
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>

#define TCP_RECEIVE_DATA_LENGTH     10

EFI_TCP4_PROTOCOL                   *gTcp4Protocol              = NULL;
EFI_IPv4_ADDRESS                    gStationAddress             = {192, 168, 1, 150};
EFI_IPv4_ADDRESS                    gSubnetMask                 = {255, 255, 255, 0};
UINT16                              gStationPort                = 54545;
EFI_IPv4_ADDRESS                    gRemoteAddress              = {192, 168, 1, 101};
UINT16                              gRemotePort                 = 54545;
EFI_TCP4_CONFIG_DATA                gTcp4ConfigData             = {
                                                                    0x00,                         //ToS
                                                                    0x10,                         //TTL
                                                                    {
                                                                      FALSE,                      //UseDefaultAddress
                                                                      {
                                                                        {0, 0, 0, 0}              //StationAddress
                                                                      },
                                                                      {
                                                                        {0, 0, 0, 0}              //SubnetMask
                                                                      },
                                                                      0,                          //StationPort
                                                                      {
                                                                        {0, 0, 0, 0}              //RemoteAddress
                                                                      },
                                                                      0,                          //RemotePort
                                                                      TRUE                        //ActiveFlag
                                                                    },
                                                                    NULL                          //ControlOption
                                                                  };


EFI_TCP4_CONNECTION_TOKEN           gTcp4ConnectToken;
// EFI_TCP4_LISTEN_TOKEN               gTcp4ListenToken;
EFI_TCP4_IO_TOKEN                   gTcp4ReceiveToken;
EFI_TCP4_IO_TOKEN                   gTcp4SendToken;
EFI_TCP4_RECEIVE_DATA               gTcp4ReceiveData;
EFI_TCP4_TRANSMIT_DATA              gTcp4SendData;
EFI_TCP4_CLOSE_TOKEN                gTcp4CloseToken;
VOID                                *gTcp4RecvBuffer;
EFI_EVENT                           Tcp4ReceiveEvent;

VOID  Tcp4ConnectNotify(IN EFI_EVENT  Event,  IN VOID *Context)
{
}
VOID  Tcp4ReceiveNotify(IN EFI_EVENT  Event,  IN VOID *Context)
{
  EFI_STATUS                  Status;
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
VOID  Tcp4SendNotify(IN EFI_EVENT  Event,  IN VOID *Context)
{

}
VOID  Tcp4CloseNotify(IN EFI_EVENT  Event,  IN VOID *Context)
{
}
VOID  Tcp4RecvPolling(IN EFI_EVENT  Event,  IN VOID *Context)
{
  EFI_STATUS          Status;
  gTcp4ReceiveData.UrgentFlag = TRUE;
  gTcp4ReceiveData.DataLength = TCP_RECEIVE_DATA_LENGTH;
  gTcp4ReceiveData.FragmentCount = 1;
  gTcp4ReceiveData.FragmentTable[0].FragmentLength = gTcp4ReceiveData.DataLength;
  gTcp4ReceiveData.FragmentTable[0].FragmentBuffer = gTcp4RecvBuffer;
  gTcp4ReceiveToken.Packet.RxData = &gTcp4ReceiveData;
  Status = gTcp4Protocol->Receive(gTcp4Protocol, &gTcp4ReceiveToken);
}
VOID Tcp4SendMessage(CHAR8 *Message, UINT32 Length)
{
  EFI_STATUS        Status;
  gTcp4SendData.Push = TRUE;
  gTcp4SendData.Urgent = TRUE;
  gTcp4SendData.DataLength = Length + 2;
  gTcp4SendData.FragmentCount = 1;
  gTcp4SendData.FragmentTable[0].FragmentLength = Length + 2;
  gTcp4SendData.FragmentTable[0].FragmentBuffer = Message;
  gTcp4SendToken.Packet.TxData = &gTcp4SendData;
  Status = gTcp4Protocol->Transmit(gTcp4Protocol, &gTcp4SendToken);
}
EFI_STATUS
ConfigureTcp4()
{
  EFI_STATUS            Status;
  gTcp4ConfigData.AccessPoint.StationAddress = gStationAddress;
  gTcp4ConfigData.AccessPoint.SubnetMask = gSubnetMask;
  gTcp4ConfigData.AccessPoint.StationPort = gStationPort;
  gTcp4ConfigData.AccessPoint.RemoteAddress = gRemoteAddress;
  gTcp4ConfigData.AccessPoint.RemotePort = gRemotePort;
  Status = gTcp4Protocol->Configure(gTcp4Protocol, &gTcp4ConfigData);

  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4ConnectNotify, (VOID *)&gTcp4ConnectToken, &gTcp4ConnectToken.CompletionToken.Event);
  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4ReceiveNotify, (VOID *)&gTcp4ReceiveToken, &gTcp4ReceiveToken.CompletionToken.Event);
  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4CloseNotify, (VOID *)&gTcp4CloseToken, &gTcp4CloseToken.CompletionToken.Event);
  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4SendNotify, (VOID *)&gTcp4SendToken, &gTcp4SendToken.CompletionToken.Event);
  return Status;
}


EFI_STATUS
Tcp4ReceiveEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                          Status                    = EFI_SUCCESS;
  EFI_SERVICE_BINDING_PROTOCOL        *Tcp4BindingProtocol      = NULL;
  EFI_HANDLE                          ChildHandle               = NULL;


  Status = gBS->LocateProtocol(&gEfiTcp4ServiceBindingProtocolGuid, NULL, (VOID **)&Tcp4BindingProtocol);
  if (EFI_ERROR(Status)) return Status;
  else {
    Status = Tcp4BindingProtocol->CreateChild(Tcp4BindingProtocol, &ChildHandle);
    Status = gBS->OpenProtocol(ChildHandle, &gEfiTcp4ProtocolGuid, (VOID**)&gTcp4Protocol, ImageHandle, ChildHandle, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  }

  Status = ConfigureTcp4();
  if (EFI_ERROR(Status))
    return Status;

  Status = gTcp4Protocol->Connect(gTcp4Protocol, &gTcp4ConnectToken);

  gTcp4RecvBuffer = AllocateZeroPool(TCP_RECEIVE_DATA_LENGTH);

  Status = gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4RecvPolling, NULL, &Tcp4ReceiveEvent);
  Status = gBS->SetTimer(Tcp4ReceiveEvent, TimerPeriodic, 10000000);

  // if (gConInProtocol != NULL)
  //   gConInProtocol = NULL;

  return EFI_SUCCESS;
}


// #include <Protocol/ServiceBinding.h>
// #include <Protocol/Tcp4.h>

// VOID TickCmdExecute(CHAR8 *CMD);
// VOID CmdExecute(CHAR8 *CMD);

// #define TCP_TICK_RECEIVE_DATA_LENGTH     20
// #define TCP_CMD_RECEIVE_DATA_LENGTH      20

// EFI_TCP4_PROTOCOL                   *gTcp4Tick                  = NULL;
// EFI_TCP4_PROTOCOL                   *gTcp4                      = NULL;
// EFI_IPv4_ADDRESS                    gSubnetMask                 = {{255, 255, 255, 0}};
// UINT16                              gTickPort                   = 60900;
// UINT16                              gCmdPort                    = 60800;
// EFI_TCP4_CONFIG_DATA                gTcp4ConfigData             = {
//                                                                     0x00,                         //ToS
//                                                                     0x10,                         //TTL
//                                                                     {
//                                                                       FALSE,                      //UseDefaultAddress
//                                                                       {
//                                                                         {0, 0, 0, 0}              //StationAddress
//                                                                       },
//                                                                       {
//                                                                         {0, 0, 0, 0}              //SubnetMask
//                                                                       },
//                                                                       0,                          //StationPort
//                                                                       {
//                                                                         {0, 0, 0, 0}              //RemoteAddress
//                                                                       },
//                                                                       0,                          //RemotePort
//                                                                       TRUE                        //ActiveFlag
//                                                                     },
//                                                                     NULL                          //ControlOption
//                                                                   };

// CHAR8                       *CmdLines[] = {
//                                             "1234",
//                                             "5678",
//                                             "rbot",
//                                             NULL
//                                           };
// enum {
//   CmdFlagChar1234,
//   CmdFlagChar5678,
//   CmdFlagCharrbot,
//   CmdFlagUnkown
// };

// EFI_TCP4_CONNECTION_TOKEN           gTcp4TickConnect;
// // EFI_TCP4_LISTEN_TOKEN               gTcp4ListenToken;
// EFI_TCP4_IO_TOKEN                   gTcp4TickReceive;
// EFI_TCP4_IO_TOKEN                   gTcp4TickSend;
// EFI_TCP4_RECEIVE_DATA               gTcp4TickReceiveData;
// EFI_TCP4_TRANSMIT_DATA              gTcp4TickSendData;
// EFI_TCP4_CLOSE_TOKEN                gTcp4TickClose;
// VOID                                *gTcp4TickRecvBuffer;
// EFI_EVENT                           gTcp4TickReceiveEvent;

// EFI_TCP4_CONNECTION_TOKEN           gTcp4CmdConnect;
// EFI_TCP4_IO_TOKEN                   gTcp4CmdReceive;
// EFI_TCP4_IO_TOKEN                   gTcp4CmdSend;
// EFI_TCP4_RECEIVE_DATA               gTcp4CmdReceiveData;
// EFI_TCP4_TRANSMIT_DATA              gTcp4CmdSendData;
// EFI_TCP4_CLOSE_TOKEN                gTcp4CmdClose;
// VOID                                *gTcp4CmdRecvBuffer;
// EFI_EVENT                           gTcp4CmdReceiveEvent;

// VOID Tcp4TickConnectNotify(IN EFI_EVENT  Event,  IN VOID *Context)
// {
// }
// VOID Tcp4TickReceiveNotify(IN EFI_EVENT  Event,  IN VOID *Context)
// {
//   EFI_STATUS                  Status;
//   EFI_TCP4_IO_TOKEN           *ReceiveToken = &gTcp4TickReceive;
//   EFI_TCP4_RECEIVE_DATA       *RxData = ReceiveToken->Packet.RxData;
//   CHAR8                       *RecvBuffer = AllocateZeroPool(RxData->DataLength + 1);
//   CopyMem(RecvBuffer, RxData->FragmentTable[0].FragmentBuffer, RxData->DataLength);
//   Status = ReceiveToken->CompletionToken.Status;
//   AsciiPrint("\nrecv Status:%r, Data: ", Status);
//   for (UINT32 i = 0; i < RxData->DataLength; i++) {
//     AsciiPrint("%c-", RecvBuffer[i]);
//   }
//   TickCmdExecute(RecvBuffer);
//   FreePool(RecvBuffer);
// }
// VOID Tcp4TickSendNotify(IN EFI_EVENT  Event,  IN VOID *Context)
// {
// }
// VOID Tcp4TickCloseNotify(IN EFI_EVENT  Event,  IN VOID *Context)
// {
// }

// VOID Tcp4CmdConnectNotify(IN EFI_EVENT  Event,  IN VOID *Context)
// {
// }
// VOID Tcp4CmdReceiveNotify(IN EFI_EVENT  Event,  IN VOID *Context)
// {
//   EFI_STATUS                  Status;
//   EFI_TCP4_IO_TOKEN           *ReceiveToken = &gTcp4CmdReceive;
//   EFI_TCP4_RECEIVE_DATA       *RxData = ReceiveToken->Packet.RxData;
//   CHAR8                       *RecvBuffer = AllocateZeroPool(RxData->DataLength + 1);
//   CopyMem(RecvBuffer, RxData->FragmentTable[0].FragmentBuffer, RxData->DataLength);
//   Status = ReceiveToken->CompletionToken.Status;
//   AsciiPrint("\nrecv Status:%r, Data: ", Status);
//   for (UINT32 i = 0; i < RxData->DataLength; i++) {
//     AsciiPrint("%c-", RecvBuffer[i]);
//   }
//   CmdExecute(RecvBuffer);
//   FreePool(RecvBuffer);
// }
// VOID Tcp4CmdSendNotify(IN EFI_EVENT  Event,  IN VOID *Context)
// {
// }
// VOID Tcp4CmdCloseNotify(IN EFI_EVENT  Event,  IN VOID *Context)
// {
// }

// VOID Tcp4TickPolling(IN EFI_EVENT  Event,  IN VOID *Context)
// {
//   EFI_STATUS          Status;
//   Tcp4TickSendMessage("tick", 5);
//   gTcp4TickReceiveData.UrgentFlag = TRUE;
//   gTcp4TickReceiveData.DataLength = TCP_TICK_RECEIVE_DATA_LENGTH;
//   gTcp4TickReceiveData.FragmentCount = 1;
//   gTcp4TickReceiveData.FragmentTable[0].FragmentLength = gTcp4TickReceiveData.DataLength;
//   gTcp4TickReceiveData.FragmentTable[0].FragmentBuffer = gTcp4TickRecvBuffer;
//   gTcp4TickReceive.Packet.RxData = &gTcp4TickReceiveData;
//   Status = gTcp4Tick->Receive(gTcp4Tick, &gTcp4TickReceive);
// }
// VOID Tcp4CmdPolling(IN EFI_EVENT  Event,  IN VOID *Context)
// {
//   EFI_STATUS          Status;
//   gTcp4CmdReceiveData.UrgentFlag = TRUE;
//   gTcp4CmdReceiveData.DataLength = TCP_CMD_RECEIVE_DATA_LENGTH;
//   gTcp4CmdReceiveData.FragmentCount = 1;
//   gTcp4CmdReceiveData.FragmentTable[0].FragmentLength = gTcp4CmdReceiveData.DataLength;
//   gTcp4CmdReceiveData.FragmentTable[0].FragmentBuffer = gTcp4CmdRecvBuffer;
//   gTcp4CmdReceive.Packet.RxData = &gTcp4CmdReceiveData;
//   Status = gTcp4->Receive(gTcp4, &gTcp4CmdReceive);
// }
// VOID Tcp4TickSendMessage(CONST CHAR8 *Message, UINT32 Length)
// {
//   EFI_STATUS        Status;
//   gTcp4TickSendData.Push = TRUE;
//   gTcp4TickSendData.Urgent = TRUE;
//   gTcp4TickSendData.DataLength = Length + 1;
//   gTcp4TickSendData.FragmentCount = 1;
//   gTcp4TickSendData.FragmentTable[0].FragmentLength = Length + 1;
//   gTcp4TickSendData.FragmentTable[0].FragmentBuffer = (VOID*)Message;
//   gTcp4TickSend.Packet.TxData = &gTcp4TickSendData;
//   Status = gTcp4Tick->Transmit(gTcp4Tick, &gTcp4TickSend);
// }
// VOID Tcp4CmdSendMessage(CONST CHAR8 *Message, UINT32 Length)
// {
//   EFI_STATUS        Status;
//   gTcp4CmdSendData.Push = TRUE;
//   gTcp4CmdSendData.Urgent = TRUE;
//   gTcp4CmdSendData.DataLength = Length + 1;
//   gTcp4CmdSendData.FragmentCount = 1;
//   gTcp4CmdSendData.FragmentTable[0].FragmentLength = Length + 1;
//   gTcp4CmdSendData.FragmentTable[0].FragmentBuffer = (VOID*)Message;
//   gTcp4CmdSend.Packet.TxData = &gTcp4CmdSendData;
//   Status = gTcp4->Transmit(gTcp4, &gTcp4CmdSend);
// }
// VOID TickCmdExecute(CHAR8 *CMD)
// {
//   UINT8 i = 0;
//   while ((CmdLines[i] != NULL) && AsciiStrCmp((CONST CHAR8*)CmdLines[i], (CONST CHAR8*)CMD) != 0)
//   {
//     i++;
//   }
//   CONST CHAR8     *UnkownMessage = "Unkown CMD";
//   switch (i)
//   {
//   case CmdFlagChar1234:
//     break;
//   case CmdFlagChar5678:
//     break;
//   case CmdFlagCharrbot:
//     gRT->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
//     break;
//   case CmdFlagUnkown:
//     Tcp4TickSendMessage(UnkownMessage, (UINT32)AsciiStrSize(UnkownMessage));
//     break;
//   default:
//     break;
//   }
// }

// VOID CmdExecute(CHAR8 *CMD)
// {
//   UINT8 i = 0;
//   while ((CmdLines[i] != NULL) && AsciiStrCmp((CONST CHAR8*)CmdLines[i], (CONST CHAR8*)CMD) != 0)
//   {
//     i++;
//   }
//   CONST CHAR8     *UnkownMessage = "Unkown CMD";
//   switch (i)
//   {
//   case CmdFlagChar1234:
//     break;
//   case CmdFlagChar5678:
//     break;
//   case CmdFlagCharrbot:
//     gRT->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
//     break;
//   case CmdFlagUnkown:
//     Tcp4CmdSendMessage(UnkownMessage, (UINT32)AsciiStrSize(UnkownMessage));
//     break;
//   default:
//     break;
//   }
// }

// EFI_STATUS
// ConfigureTcp4Tick()
// {
//   EFI_STATUS            Status;
//   gTcp4ConfigData.AccessPoint.StationAddress = ClientIP.v4;
//   gTcp4ConfigData.AccessPoint.SubnetMask = gSubnetMask;
//   gTcp4ConfigData.AccessPoint.StationPort = gTickPort;
//   gTcp4ConfigData.AccessPoint.RemoteAddress = ServerIP.v4;
//   gTcp4ConfigData.AccessPoint.RemotePort = gTickPort;
//   Status = gTcp4Tick->Configure(gTcp4Tick, &gTcp4ConfigData);
//   Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4TickConnectNotify, (VOID *)&gTcp4TickConnect, &gTcp4TickConnect.CompletionToken.Event);
//   Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4TickReceiveNotify, (VOID *)&gTcp4TickReceive, &gTcp4TickReceive.CompletionToken.Event);
//   Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4TickCloseNotify, (VOID *)&gTcp4TickClose, &gTcp4TickClose.CompletionToken.Event);
//   Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4TickSendNotify, (VOID *)&gTcp4TickSend, &gTcp4TickSend.CompletionToken.Event);
//   return Status;
// }

// EFI_STATUS
// ConfigureTcp4Cmd()
// {
//   EFI_STATUS            Status;
//   gTcp4ConfigData.AccessPoint.StationAddress = ClientIP.v4;
//   gTcp4ConfigData.AccessPoint.SubnetMask = gSubnetMask;
//   gTcp4ConfigData.AccessPoint.StationPort = gCmdPort;
//   gTcp4ConfigData.AccessPoint.RemoteAddress = ServerIP.v4;
//   gTcp4ConfigData.AccessPoint.RemotePort = gCmdPort;
//   Status = gTcp4->Configure(gTcp4, &gTcp4ConfigData);
//   Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4CmdConnectNotify, (VOID *)&gTcp4CmdConnect, &gTcp4CmdConnect.CompletionToken.Event);
//   Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4CmdReceiveNotify, (VOID *)&gTcp4CmdReceive, &gTcp4CmdReceive.CompletionToken.Event);
//   Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4CmdCloseNotify, (VOID *)&gTcp4CmdClose, &gTcp4CmdClose.CompletionToken.Event);
//   Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4CmdSendNotify, (VOID *)&gTcp4CmdSend, &gTcp4CmdSend.CompletionToken.Event);
//   return Status;
// }
// EFI_STATUS
// Tcp4ReceiveHandleCommand(EFI_HANDLE Handle)
// {
//   EFI_STATUS                          Status                    = EFI_SUCCESS;
//   EFI_SERVICE_BINDING_PROTOCOL        *Tcp4BindingProtocol      = NULL;
//   EFI_HANDLE                          ChildHandle               = NULL;
//   EFI_HANDLE                          ChildHandle1              = NULL;

//   Status = gBS->LocateProtocol(&gEfiTcp4ServiceBindingProtocolGuid, NULL, (VOID **)&Tcp4BindingProtocol);
//   if (EFI_ERROR(Status)) {
//     if (Tcp4BindingProtocol == NULL) {
//     }
//     return Status;
//   } else {
//     Status = Tcp4BindingProtocol->CreateChild(Tcp4BindingProtocol, &ChildHandle);
//     Status = gBS->OpenProtocol(ChildHandle, &gEfiTcp4ProtocolGuid, (VOID**)&gTcp4Tick, Handle, ChildHandle, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
//     Status = Tcp4BindingProtocol->CreateChild(Tcp4BindingProtocol, &ChildHandle1);
//     Status = gBS->OpenProtocol(ChildHandle1, &gEfiTcp4ProtocolGuid, (VOID**)&gTcp4, Handle, ChildHandle1, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
//   }

//   Status = ConfigureTcp4Tick();
//   if (EFI_ERROR(Status))
//     return Status;
  
//   Status = gTcp4Tick->Connect(gTcp4Tick, &gTcp4TickConnect);
//   if (!EFI_ERROR (Status)) {
//     gTcp4TickRecvBuffer = AllocateZeroPool(TCP_TICK_RECEIVE_DATA_LENGTH);
//     Status = gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4TickPolling, NULL, &gTcp4TickReceiveEvent);
//     Status = gBS->SetTimer(gTcp4TickReceiveEvent, TimerPeriodic, 10000000);
//   }

//   Status = ConfigureTcp4Cmd();
//   if (EFI_ERROR(Status))
//     return Status;
//   Status = gTcp4->Connect(gTcp4, &gTcp4CmdConnect);
//   if (!EFI_ERROR (Status)) {
//     gTcp4CmdRecvBuffer = AllocateZeroPool(TCP_CMD_RECEIVE_DATA_LENGTH);
//     Status = gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Tcp4CmdPolling, NULL, &gTcp4CmdReceiveEvent);
//     Status = gBS->SetTimer(gTcp4CmdReceiveEvent, TimerPeriodic, 10000000);
//   }
//   return EFI_SUCCESS;
// }