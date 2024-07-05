#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/CpuLib.h>

#include <Register/Intel/Msr.h>
#include <Register/Intel/Cpuid.h>

#include <stdio.h>

typedef struct {
  UINT64          TscBefore;
  UINT64          TscAfter;
  UINT64          C0_MCNT_Before;
  UINT64          C0_MCNT_After;
  UINT64          C0_ACNT_Before;
  UINT64          C0_ACNT_After;
  UINT64          Unhalted_Core_Before;
  UINT64          Unhalted_Core_After;
  UINT64          Unhalted_Ref_Before;
  UINT64          Unhalted_Ref_After;
} CPU_MSR_COUNTER;

CPU_MSR_COUNTER Cpu0;

INT32                                           CursorColumn;
INT32                                           CursorRow;
UINT64                                          TscFreq;
UINT64                                          CurFreq;
UINT8                                           UtilizationCpu;
// EFI_INPUT_KEY                                   Key;

VOID MyEventNotify(IN EFI_EVENT Event, IN VOID *Context)
{
  Cpu0.C0_MCNT_After = AsmReadMsr64(MSR_IA32_MPERF);
  Cpu0.C0_ACNT_After = AsmReadMsr64(MSR_IA32_APERF);
  Cpu0.TscAfter = AsmReadMsr64(MSR_IA32_TIME_STAMP_COUNTER);
  Cpu0.Unhalted_Ref_After = AsmReadMsr64(MSR_IA32_FIXED_CTR2);
  Cpu0.Unhalted_Core_After = AsmReadMsr64(MSR_IA32_FIXED_CTR1);

  CurFreq = TscFreq * (Cpu0.C0_ACNT_After - Cpu0.C0_ACNT_Before) / (Cpu0.C0_MCNT_After - Cpu0.C0_MCNT_Before);
  UtilizationCpu = (UINT8)((Cpu0.Unhalted_Ref_After - Cpu0.Unhalted_Ref_Before) * 100 / (Cpu0.TscAfter - Cpu0.TscBefore));

  gST->ConOut->SetCursorPosition(gST->ConOut, CursorColumn, CursorRow);
  Print(L"Current frequency   :%11ldHz\n", CurFreq);
  Print(L"Current utilization :%4d%%\n", UtilizationCpu);

  Cpu0.C0_MCNT_Before = Cpu0.C0_MCNT_After;
  Cpu0.C0_ACNT_Before = Cpu0.C0_ACNT_After;
  Cpu0.Unhalted_Ref_Before = Cpu0.Unhalted_Ref_After;
  Cpu0.TscBefore = Cpu0.TscAfter;
  Cpu0.Unhalted_Core_Before = Cpu0.Unhalted_Core_After;
  // if (gST->ConIn->ReadKeyStroke(gST->ConIn, &Key) == EFI_SUCCESS) {
  //   if (Key.UnicodeChar == L'q') {
  //     AsmWriteMsr64(MSR_IA32_FIXED_CTR_CTRL, 0);
  //     AsmWriteMsr64(MSR_IA32_PERF_GLOBAL_CTRL,0xFF);
  //     gBS->CloseEvent(Event);
  //   }
  // }
}

int
main(
  IN int Argc,
  IN char *Argv[]
  )
{
  CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING_EAX  EaxPmu;
  CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING_EBX  EbxPmu;
  UINT32                                          EcxPmu;
  CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING_EDX  EdxPmu;
  UINT32                                          EaxTsc;
  UINT32                                          EbxTsc;
  UINT32                                          EcxTsc;


  CursorColumn = gST->ConOut->Mode->CursorColumn;
  CursorRow = gST->ConOut->Mode->CursorRow;
  gST->ConOut->EnableCursor(gST->ConOut, FALSE);

  AsmCpuid(CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING, &EaxPmu.Uint32, &EbxPmu.Uint32, &EcxPmu, &EdxPmu.Uint32);
  AsmCpuid(CPUID_TIME_STAMP_COUNTER, &EaxTsc, &EbxTsc, &EcxTsc, NULL);

  TscFreq = (UINT64)EcxTsc * EbxTsc / EaxTsc;
  Cpu0.C0_MCNT_Before = AsmReadMsr64(MSR_IA32_MPERF);
  Cpu0.C0_ACNT_Before = AsmReadMsr64(MSR_IA32_APERF);
  Cpu0.TscBefore = AsmReadMsr64(MSR_IA32_TIME_STAMP_COUNTER);
  AsmWriteMsr64(MSR_IA32_FIXED_CTR_CTRL, 0x3333);
  AsmWriteMsr64(MSR_IA32_PERF_GLOBAL_CTRL, 0xF000000FF);
  Cpu0.Unhalted_Ref_Before = AsmReadMsr64(MSR_IA32_FIXED_CTR2);
  Cpu0.Unhalted_Core_Before = AsmReadMsr64(MSR_IA32_FIXED_CTR1);

  EFI_EVENT MyEvent;
  gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, MyEventNotify, NULL, &MyEvent);
  gBS->SetTimer(MyEvent, TimerPeriodic, 10000000);

  // DisableInterrupts();
  // gBS->Stall(10000000);
  // CpuSleep();
  // gBS->Stall(10000000);
  // EnableInterrupts();
  return 0;
}