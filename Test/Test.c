#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SmbusLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BmpSupportLib.h>
#include <Library/FileHandleLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>

#include <Protocol/ServiceBinding.h>
#include <Protocol/Tcp4.h>
#include <Protocol/Udp4.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiPackageList.h>
#include <Protocol/HiiImage.h>
#include <Protocol/HiiImageEx.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/HiiImageDecoder.h>

#include <Guid/SmBios.h>
#include <Guid/Acpi.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/SmBios.h>
#include <Register/Intel/Msr.h>
#include <Register/Intel/Cpuid.h>

#include <stdio.h>
// #pragma warning(disable : 4305)
#include <Library/TimeBaseLib.h>

int
main(
  IN int Argc,
  IN char *Argv[]
  )
{
  FILE *fp = fopen("1234.txt", "rb");
  if (fp == NULL) {
    printf("NULL\n");
  }
  static char buf1[20];
  size_t si = fread(buf1, 1, 20, fp);
  fclose(fp);
  printf("%d %s\n", si, buf1);
  return 0;
}
