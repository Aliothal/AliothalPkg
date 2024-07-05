#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/Bmp.h>
#include <Guid/Acpi.h>
#include <Library/BmpSupportLib.h>

EFI_STATUS
DumpBootLogoEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status = 0;
  EFI_ACPI_6_5_ROOT_SYSTEM_DESCRIPTION_POINTER    *Rsdp = NULL;
  EFI_ACPI_DESCRIPTION_HEADER                     *Xsdt = NULL;
  EFI_ACPI_6_5_BOOT_GRAPHICS_RESOURCE_TABLE       *Bgrt = NULL;
  UINT32              EntryCount = 0;

  Status = EfiGetSystemConfigurationTable(&gEfiAcpiTableGuid, (VOID **)&Rsdp);
  if (Rsdp->Revision >= EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_TABLE_REVISION)
    Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *)Rsdp->XsdtAddress;
  else
    return 0;
  EntryCount = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT64);
  for (UINT32 i = 0; i < EntryCount; i++) {
    Bgrt = (EFI_ACPI_6_5_BOOT_GRAPHICS_RESOURCE_TABLE *)(*((UINT64 *)(Xsdt + 1) + i));
    if (Bgrt->Header.Signature == EFI_ACPI_6_5_BOOT_GRAPHICS_RESOURCE_TABLE_SIGNATURE)
      break;
  }

  BMP_IMAGE_HEADER        *Bmp = NULL;
  SHELL_FILE_HANDLE       FileHandle = NULL;
  UINTN                   BufferSize = 0;

  if (Bgrt->ImageType == EFI_ACPI_6_5_BGRT_IMAGE_TYPE_BMP)
    Bmp = (BMP_IMAGE_HEADER *)Bgrt->ImageAddress;
  
  BufferSize = Bmp->Size;
  Status = ShellOpenFileByName(L"logo.bmp", &FileHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, EFI_FILE_ARCHIVE);
  Print(L"ShellOpenFileByName Status:%r\n", Status);
  Status = ShellWriteFile(FileHandle, &BufferSize, Bmp);
  Print(L"ShellWriteFile Status:%r\n", Status);
  ShellCloseFile(&FileHandle);
  
  // EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *BltPixel = NULL;
  // UINTN                   GopBltSize = 0;
  // UINTN                   PixelHeight = 0;
  // UINTN                   PixelWidth = 0;
  // Status = TranslateBmpToGopBlt(Bmp, Bmp->Size, &BltPixel, &GopBltSize, &PixelHeight, &PixelWidth);

  return EFI_SUCCESS;
}