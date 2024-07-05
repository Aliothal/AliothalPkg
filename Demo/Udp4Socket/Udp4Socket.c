// #include <Library/ShellLib.h>
// #include <Library/ShellCommandLib.h>
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/Udp4SocketLib.h>
#include <library/PrintLib.h>
#include <Library/cJSONLib.h>

#include <Protocol/ServiceBinding.h>
#include <Protocol/Udp4.h>
#include <Protocol/Ip4Config2.h>

UDP4_SOCKET *SocketTrans = NULL;

VOID EFIAPI Udp4ReceiveHandler(IN EFI_EVENT  Event,  IN VOID *Context)
{
  UDP4_SOCKET                 *Socket = Context;
  EFI_UDP4_RECEIVE_DATA       *RxData = Socket->TokenReceive.Packet.RxData;
  if (Socket->TokenReceive.Status == EFI_ABORTED || RxData->DataLength == 0)
    return;
  gBS->SignalEvent(RxData->RecycleSignal);
  cJSON *Tree = NULL;
  Tree = cJSON_ParseWithLength(RxData->FragmentTable[0].FragmentBuffer, RxData->FragmentTable[0].FragmentLength);
  cJSON *Item = cJSON_GetObjectItem(Tree, "CMD");
  cJSON_Delete(Tree);
  SocketTrans->ConfigData.RemoteAddress = RxData->UdpSession.SourceAddress;
  SocketTrans->ConfigData.RemotePort = RxData->UdpSession.SourcePort;
  SocketTrans->Udp4->Configure(SocketTrans->Udp4, NULL);
  SocketTrans->Udp4->Configure(SocketTrans->Udp4, &SocketTrans->ConfigData);
  AsciiUdp4Write(SocketTrans, "{\"MAC\": \"ff:22:33:44:55:66\", \"IP\": \"%d.%d.%d.%d\", \"SN\": \"12345678\", \"TYPE\": 0, \"SLOTS\": 16, \"STATUS\": 0}",
                SocketTrans->ConfigData.StationAddress.Addr[0], SocketTrans->ConfigData.StationAddress.Addr[1],
                SocketTrans->ConfigData.StationAddress.Addr[2], SocketTrans->ConfigData.StationAddress.Addr[3]);

  Socket->Udp4->Receive(Socket->Udp4, &Socket->TokenReceive);
  return;
}

VOID EFIAPI Udp4NullHandler(IN EFI_EVENT  Event,  IN VOID *Context)
{

}

EFI_STATUS EFIAPI Udp4Receive(UDP4_SOCKET *Socket)
{
  return Socket->Udp4->Receive(Socket->Udp4, &Socket->TokenReceive);
}

EFI_STATUS
Udp4SocketEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                        Status = EFI_SUCCESS;
//   EFI_IP4_CONFIG2_PROTOCOL          *Ip4Cfg2;
//   EFI_IP4_CONFIG2_POLICY            Policy = Ip4Config2PolicyDhcp;
//   UINTN                             DataSize = 0;
//   EFI_IP4_CONFIG2_INTERFACE_INFO    *IfInfo;

//   Status = gBS->LocateProtocol(&gEfiIp4Config2ProtocolGuid, NULL, (VOID **)&Ip4Cfg2);
//   Status = Ip4Cfg2->GetData(Ip4Cfg2, Ip4Config2DataTypeInterfaceInfo, &DataSize, NULL);
//   IfInfo = AllocateZeroPool(DataSize);
//   Print(L"Sz:0x%x, 0x%x\n", DataSize, sizeof(EFI_IP4_CONFIG2_INTERFACE_INFO));
//   Status = Ip4Cfg2->GetData(Ip4Cfg2, Ip4Config2DataTypeInterfaceInfo, &DataSize, IfInfo);
//   Print(L"MAC:%02x-%02x-%02x-%02x-%02x-%02x\n", IfInfo->HwAddress.Addr[0],IfInfo->HwAddress.Addr[1],
//   IfInfo->HwAddress.Addr[2],IfInfo->HwAddress.Addr[3],IfInfo->HwAddress.Addr[4],IfInfo->HwAddress.Addr[5]);
//   Print(L"Ip:%d.%d.%d.%d\n", IfInfo->StationAddress.Addr[0],IfInfo->StationAddress.Addr[1],
//   IfInfo->StationAddress.Addr[2],IfInfo->StationAddress.Addr[3]);
//   Status = Ip4Cfg2->SetData(Ip4Cfg2, Ip4Config2DataTypePolicy, sizeof(EFI_IP4_CONFIG2_POLICY), &Policy);

// return 0;
  EFI_UDP4_CONFIG_DATA ConfigData;
  UDP4_SOCKET *TestSock = NULL;
  SetMem(&ConfigData, sizeof(EFI_UDP4_CONFIG_DATA), 0);
  ConfigData.AcceptBroadcast = TRUE;
  ConfigData.AllowDuplicatePort = TRUE;
  ConfigData.TimeToLive = 16;
  ConfigData.DoNotFragment = TRUE;
  ConfigData.StationPort = 5566;
  Status = CreateUdp4Socket(&ConfigData, (EFI_EVENT_NOTIFY)Udp4ReceiveHandler, (EFI_EVENT_NOTIFY)Udp4NullHandler, &TestSock);
  if (EFI_ERROR (Status))
    return Status;

  Status = Udp4Receive(TestSock);

  EFI_UDP4_CONFIG_DATA ConfigData1 = {
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
                          {{192, 168, 101, 101}},  // StationAddress
                          {{255, 255, 255, 0}},  // SubnetMask
                          5566,      // StationPort
                          {{192, 168, 101, 189}},  // RemoteAddress
                          // {{0, 0, 0, 0}},  // RemoteAddress
                          5566,      // RemotePort
  };
  
  Status = CreateUdp4Socket(&ConfigData1, (EFI_EVENT_NOTIFY)Udp4NullHandler, (EFI_EVENT_NOTIFY)Udp4NullHandler, &SocketTrans);
  if (EFI_ERROR (Status))
    return Status;

  // Status = CloseUdp4Socket(TestSock);

  return Status;
}