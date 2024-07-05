#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FileHandleLib.h>
#include <Library/PrintLib.h>

void GetGdt(void)
{
  IA32_DESCRIPTOR               Gdtr;
  UINT16                        GdtCount;
  IA32_SEGMENT_DESCRIPTOR       *Gdt;

  AsmReadGdtr(&Gdtr);
  Gdt = (IA32_SEGMENT_DESCRIPTOR *)Gdtr.Base;
  GdtCount = (Gdtr.Limit + 1) / sizeof(IA32_SEGMENT_DESCRIPTOR);
  AsciiPrint("Gdtr lmit:0x%x, base:0x%lx\n", Gdtr.Limit, Gdtr.Base);
  for (UINT16 i = 0; i < GdtCount; i++) {
    gST->ConOut->SetAttribute(gST->ConOut, EFI_YELLOW);
    AsciiPrint("Gdt:%016lx\n", Gdt->Uint64);
    gST->ConOut->SetAttribute(gST->ConOut, EFI_LIGHTGRAY);

    if (Gdt->Bits.S) {    //data or code
      if ((Gdt->Bits.Type & BIT3)) {      //code
        AsciiPrint("code execute");
        if (Gdt->Bits.Type & BIT0) {      //accessed
          AsciiPrint(" accessed");
        }
        if (Gdt->Bits.Type & BIT1) {      //read
          AsciiPrint(" read");
        }
        if (Gdt->Bits.Type & BIT2) {      //conforming
          AsciiPrint(" conforming");
        }
        if (Gdt->Bits.DB) {
          AsciiPrint(" Default operation size:32");
        } else {
          AsciiPrint(" Default operation size:16");
        }
        AsciiPrint("\n");
      } else {                            //data
        AsciiPrint("data read");
        if (Gdt->Bits.Type & BIT0) {      //accessed
          AsciiPrint(" accessed");
        }
        if (Gdt->Bits.Type & BIT1) {      //write
          AsciiPrint(" write");
        }
        if (Gdt->Bits.Type & BIT2) {      //expand-down
          AsciiPrint(" expand-down");
          if (Gdt->Bits.DB) {
            AsciiPrint(" upper bound:4GB");
          } else {
            AsciiPrint(" upper bound:64KB");
          }
        }
        AsciiPrint("\n");
      }
      UINT32  SegmentLimit;
      UINT32  SegmentBase;
      SegmentLimit = (Gdt->Bits.LimitLow | (Gdt->Bits.LimitHigh << 16)) + 1;
      SegmentBase = (Gdt->Bits.BaseLow | (Gdt->Bits.BaseMid << 16) | (Gdt->Bits.BaseHigh << 24));
      if (Gdt->Bits.G) {
        AsciiPrint("Segment Limit:0x%x pages, SegmentBase:0x%08x, DPL:%d\n", SegmentLimit, SegmentBase, Gdt->Bits.DPL);
      } else {
        AsciiPrint("Segment Limit:0x%x bytes, SegmentBase:0x%08x, DPL:%d\n", SegmentLimit, SegmentBase, Gdt->Bits.DPL);
      }
      if (Gdt->Bits.P) {
        AsciiPrint("Present in memory");
      } else {
        AsciiPrint("Not present in memory");
      }
      if (Gdt->Bits.L) {
        AsciiPrint(" 64bit code segment");
      } else {
        AsciiPrint(" compatibility mode");
      }
      if (Gdt->Bits.AVL) {
        AsciiPrint(" Available for use by system software\n");
      } else {
        AsciiPrint(" Not available for use by system software\n");
      }
    } else {              //system
#ifdef MDE_CPU_X64
      switch (Gdt->Bits.Type)
      {
      case 2:
        AsciiPrint("LDT\n");
        break;
      case 9:
        AsciiPrint("64bit TSS(Available)\n");
        break;
      case 11:
        AsciiPrint("64bit TSS(Busy)\n");
        break;
      case 12:
        AsciiPrint("64bit Call Gate\n");
        break;
      case 14:
        AsciiPrint("64bit Interrupt Gate\n");
        break;
      case 15:
        AsciiPrint("64bit Trap Gate\n");
        break;
      default:
        break;
      }
#endif
    }
    
    Gdt++;
  }
}

EFI_STATUS
EFIAPI
GetTableEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                    Status = EFI_SUCCESS;

  return Status;
}