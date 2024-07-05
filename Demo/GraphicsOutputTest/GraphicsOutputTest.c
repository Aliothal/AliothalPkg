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
  Status = ShellExecute(&gImageHandle, L"ifconfig -s eth0 static 192.168.1.50 255.255.255.0 192.168.1.1", FALSE, NULL, &Status);
  Print(L"%r\n", Status);
  return Status;
}