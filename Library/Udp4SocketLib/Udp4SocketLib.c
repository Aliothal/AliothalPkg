#include <Library/Udp4SocketLib.h>

static EFI_SERVICE_BINDING_PROTOCOL        *Udp4Service     = NULL;
static CHAR8 StartOfBuffer[MAX_UDP4_FRAGMENT_LENGTH];

EFI_STATUS EFIAPI CreateUdp4Socket(EFI_UDP4_CONFIG_DATA *ConfigData, EFI_EVENT_NOTIFY NotifyReceive, EFI_EVENT_NOTIFY NotifyTransmit, UDP4_SOCKET **Socket)
{
  if (ConfigData == NULL || NotifyReceive == NULL || NotifyTransmit == NULL || Socket == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  EFI_STATUS  Status = 0;
  *Socket = AllocateZeroPool(sizeof(UDP4_SOCKET));
  Status = Udp4Service->CreateChild(Udp4Service, &(*Socket)->ChildHandle);
  if (!EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol( (*Socket)->ChildHandle,
                                &gEfiUdp4ProtocolGuid,
                                (VOID**)&(*Socket)->Udp4,
                                gImageHandle,
                                (*Socket)->ChildHandle,
                                EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (!EFI_ERROR (Status)) {
      (*Socket)->ConfigData = *ConfigData;
      Status = (*Socket)->Udp4->Configure((*Socket)->Udp4, ConfigData);
      if (!EFI_ERROR (Status)) {
        (*Socket)->TokenReceive.Packet.RxData = AllocateZeroPool(sizeof(EFI_UDP4_RECEIVE_DATA));
        Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, NotifyReceive,
                                  *Socket, &(*Socket)->TokenReceive.Event);
        if (EFI_ERROR (Status)) {
          FreePool((*Socket)->TokenReceive.Packet.RxData);
          goto error;
        }
        (*Socket)->TokenTransmit.Packet.TxData = AllocateZeroPool(sizeof(EFI_UDP4_TRANSMIT_DATA));
        Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, NotifyTransmit,
                                  *Socket, &(*Socket)->TokenTransmit.Event);
        if (EFI_ERROR (Status)) {
          FreePool((*Socket)->TokenTransmit.Packet.TxData);
          goto error;
        }

        return Status;
      }
    }
  }

error:
  FreePool(*Socket);
  *Socket = NULL;
  return Status;
}

EFI_STATUS EFIAPI CloseUdp4Socket(UDP4_SOCKET *Socket)
{
  EFI_STATUS Status = 0;
  Status = Socket->Udp4->Cancel(Socket->Udp4, NULL);
  Status |= Socket->Udp4->Configure(Socket->Udp4, NULL);
  Status |= gBS->CloseEvent(Socket->TokenReceive.Event);
  Status |= gBS->CloseEvent(Socket->TokenTransmit.Event);
  Status |= Udp4Service->DestroyChild(Udp4Service, Socket->ChildHandle);
  FreePool(Socket->TokenReceive.Packet.RxData);
  FreePool(Socket->TokenTransmit.Packet.TxData);
  FreePool(Socket);
  Socket = NULL;
  return Status;
}

EFI_STATUS EFIAPI AsciiUdp4Write(UDP4_SOCKET *Socket, CONST CHAR8 *FormatString, ...)
{
  if (Socket == NULL)
    return EFI_NOT_FOUND;
  VA_LIST  Marker;
  UINT32   NumberOfPrinted;
  VA_START (Marker, FormatString);
  NumberOfPrinted = (UINT32)AsciiVSPrint (StartOfBuffer, MAX_UDP4_FRAGMENT_LENGTH, FormatString, Marker);
  VA_END (Marker);
  EFI_UDP4_TRANSMIT_DATA *TxData = Socket->TokenTransmit.Packet.TxData;
  ZeroMem(TxData, sizeof(EFI_UDP4_TRANSMIT_DATA));
  TxData->DataLength = NumberOfPrinted;
  TxData->FragmentCount = 1;
  TxData->FragmentTable[0].FragmentLength = NumberOfPrinted;
  TxData->FragmentTable[0].FragmentBuffer = StartOfBuffer;
  return Socket->Udp4->Transmit(Socket->Udp4, &Socket->TokenTransmit);
}

EFI_STATUS
EFIAPI
Udp4SocketLibConstructor (
  VOID
  )
{
  EFI_STATUS                        Status = EFI_SUCCESS;

  Status = gBS->LocateProtocol(&gEfiUdp4ServiceBindingProtocolGuid, NULL, (VOID**)&Udp4Service);

  return Status;
}