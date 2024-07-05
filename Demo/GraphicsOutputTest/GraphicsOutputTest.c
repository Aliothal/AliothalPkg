#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TimerLib.h>
#include <Library/NetLib.h>

#include <Protocol/GraphicsOutput.h>


EFI_STATUS
GraphicsOutputTestEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;
  EFI_IPv4_ADDRESS ip4;
  Status = NetLibAsciiStrToIp4("192.168.1.1", &ip4);
  Print(L"Status:%r,ip4:%d.%d.%d.%d\n",Status, ip4.Addr[0],ip4.Addr[1],ip4.Addr[2],ip4.Addr[3]);
  return EFI_SUCCESS;
}