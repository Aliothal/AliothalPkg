#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

EFI_STATUS
MyPrintEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  for (UINTN i = 0; i < gEfiShellParametersProtocol->Argc; i++) {
    Print(L"Argv[%d]: %s\n", i, gEfiShellParametersProtocol->Argv[i]);
  }
  return 0;
}