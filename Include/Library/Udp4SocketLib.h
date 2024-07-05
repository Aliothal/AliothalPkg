#ifndef __UDP4_SOCKET_LIB_H__
#define __UDP4_SOCKET_LIB_H__

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>

#include <Protocol/ServiceBinding.h>
#include <Protocol/Udp4.h>

#define MAX_UDP4_FRAGMENT_LENGTH    SIZE_2KB

typedef struct {
  EFI_HANDLE                  ChildHandle;
  EFI_UDP4_PROTOCOL           *Udp4;
  EFI_UDP4_CONFIG_DATA        ConfigData;
  EFI_UDP4_COMPLETION_TOKEN   TokenReceive;
  EFI_UDP4_COMPLETION_TOKEN   TokenTransmit;
} UDP4_SOCKET;

EFI_STATUS EFIAPI CreateUdp4Socket(EFI_UDP4_CONFIG_DATA *ConfigData, EFI_EVENT_NOTIFY NotifyReceive, EFI_EVENT_NOTIFY NotifyTransmit, UDP4_SOCKET **Socket);
EFI_STATUS EFIAPI CloseUdp4Socket(UDP4_SOCKET *Socket);
EFI_STATUS EFIAPI AsciiUdp4Write(UDP4_SOCKET *Socket, CONST CHAR8 *FormatString, ...);

#endif