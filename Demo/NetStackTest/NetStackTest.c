#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

#include <Guid/SmBios.h>
#include <Guid/Acpi.h>
#include <IndustryStandard/SmBios.h>
#include <IndustryStandard/Acpi.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/NetworkInterfaceIdentifier.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/ManagedNetwork.h>

VOID MnpTest(VOID)
{

}

VOID SnpTest(VOID)
{
  EFI_STATUS          Status = 0;
  EFI_SIMPLE_NETWORK_PROTOCOL                 *Snp = NULL;

  Status = gBS->LocateProtocol(&gEfiSimpleNetworkProtocolGuid, NULL, (VOID **)&Snp);
  
}

VOID NiiTest(VOID)
{
  EFI_STATUS          Status = 0;
  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL   *Nii = NULL;

  Status = gBS->LocateProtocol(&gEfiNetworkInterfaceIdentifierProtocolGuid_31, NULL, (VOID **)&Nii);
  Print(L"Locate Nii Status: %r, Address: %x\n", Status, Nii);
  DumpHex(2, 0, sizeof(EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL), Nii);
  PXE_UNDI            *Pxe = NULL;
  Pxe = (PXE_UNDI *)(UINTN)(Nii->Id);
  Print(L"PXE_UNDI:\n");
  DumpHex(2, 0, Pxe->hw.Len, Pxe);
}

EFI_STATUS
NetStackTestEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status = 0;

  NiiTest();
  SnpTest();

  return Status;
}