#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SynchronizationLib.h>

#include <Pi/PiMultiPhase.h>
#include <Protocol/MpService.h>

UINTN     gNumber = 0;
SPIN_LOCK  gLock;

VOID Test1(VOID *Parameter)
{
  UINTN       i = 20;
  while (i--) {
    gBS->Stall(10);
    AcquireSpinLock(&gLock);
    gNumber++;
    ReleaseSpinLock(&gLock);
  }
}

VOID Test2(VOID *Parameter)
{
  UINTN       i = 20;
  while (i--) {
    gBS->Stall(10);
    AcquireSpinLock(&gLock);
    gNumber++;
    ReleaseSpinLock(&gLock);
  }
}

EFI_STATUS
MultiProcessorsEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status = EFI_SUCCESS;
  EFI_MP_SERVICES_PROTOCOL    *MpServices = NULL;
  UINTN                       ProNum = 0;
  UINTN                       EnProNum = 0;

  Status = gBS->LocateProtocol(&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpServices);
  Status = MpServices->WhoAmI(MpServices, &ProNum);
  Print(L"bsp number:%d\n", ProNum);
  Status = MpServices->GetNumberOfProcessors(MpServices, &ProNum, &EnProNum);
  Print(L"NumberOfProcessors : %d, NumberOfEnabledProcessors :%d\n", ProNum, EnProNum);

  // for (UINTN i = 0; i < ProNum; i++) {
  //   EFI_PROCESSOR_INFORMATION  ProcessorInfo;
  //   Status = MpServices->GetProcessorInfo(MpServices, i, &ProcessorInfo);
  //   Print(L"Processor[%d] Id:0x%x, Flag:0x%x, ", i, ProcessorInfo.ProcessorId, ProcessorInfo.StatusFlag);
  //   Print(L"1 Package:0x%x, Core:0x%x, Thread:0x%x\n", ProcessorInfo.Location.Package, ProcessorInfo.Location.Core, ProcessorInfo.Location.Thread);
  //   Print(L"2 Package:0x%x, Module:0x%x, Tile:0x%x, Die:0x%x, Core:0x%x, Thread:0x%x\n",
  //   ProcessorInfo.ExtendedInformation.Location2.Package,ProcessorInfo.ExtendedInformation.Location2.Module,
  //   ProcessorInfo.ExtendedInformation.Location2.Tile, ProcessorInfo.ExtendedInformation.Location2.Die,
  //   ProcessorInfo.ExtendedInformation.Location2.Core, ProcessorInfo.ExtendedInformation.Location2.Thread);
  // }
  InitializeSpinLock(&gLock);
  EFI_EVENT                 WaitEvent1;
  BOOLEAN          Finish1 = FALSE;
  Status = gBS->CreateEvent(0, TPL_NOTIFY, NULL, NULL, &WaitEvent1);
  Status = MpServices->StartupThisAP(MpServices, Test1, 2, WaitEvent1, 0, NULL, &Finish1);
  gBS->Stall(1000000);
  EFI_EVENT                 WaitEvent2;
  BOOLEAN          Finish2 = FALSE;
  Status = gBS->CreateEvent(0, TPL_NOTIFY, NULL, NULL, &WaitEvent2);
  Status = MpServices->StartupThisAP(MpServices, Test2, 2, WaitEvent2, 0, NULL, &Finish2);

  UINTN i = 40;
  while (i--) {
    gBS->Stall(10);
    AcquireSpinLock(&gLock);
    Print(L"%d ", gNumber);
    ReleaseSpinLock(&gLock);
  }
  gBS->Stall(1000000);
  Print(L"Finish1:%d, Finish2:%d\n", Finish1, Finish2);
  UINTN   Index;
  gBS->WaitForEvent(1, &WaitEvent2, &Index);

  return EFI_SUCCESS;
}