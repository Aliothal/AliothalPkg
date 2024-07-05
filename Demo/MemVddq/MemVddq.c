#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

#include <Library/PciLib.h>

#define MCHBAR_OFFSET                 0x48

typedef struct
{
  UINT64  MCHBAREN            : 1;            //  RW
  UINT64  Reserved0           : 16;           //  RO
  UINT64  MCHBAR              : 25;           //  RW
  UINT64  Reserved1           : 22;           //  RO
} MCHBAR_BASE_ADD_REG;

typedef struct
{
  UINT32  MC_PLL_RATIO        : 8;            //  RW/L
  UINT32  MC_PLL_REF          : 4;            //  RW/L
  UINT32  GEAR                : 2;            //  RW/L
  UINT32  Reserved0           : 3;            //  RO
  UINT32  VDDQ_TX_VOLTAGE     : 10;           //  RW
  UINT32  VDDQ_TX_ICCMAX      : 4;            //  RW
  UINT32  Reserved1           : 1;            //  RO
} MC_BIOS_DATA;



EFI_STATUS
MemVddqEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN                     PCI_0_0_0_48_Addr   = 0;
  UINT64                    MchRegAddr;
  MCHBAR_BASE_ADD_REG       *MchReg;
  MC_BIOS_DATA              *McData;
  UINT32                    MemVddqVoltage;

  PCI_0_0_0_48_Addr = PCI_LIB_ADDRESS(0, 0, 0, MCHBAR_OFFSET);

  MchRegAddr = PciRead32(PCI_0_0_0_48_Addr) | PciRead32(PCI_0_0_0_48_Addr + 4) << 32;
  MchReg = (MCHBAR_BASE_ADD_REG *)&MchRegAddr;

  if (MchReg->MCHBAREN != 0x01) {
    PciWrite8(PCI_0_0_0_48_Addr, 0x01);
    MchRegAddr = PciRead32(PCI_0_0_0_48_Addr) | PciRead32(PCI_0_0_0_48_Addr + 4) << 32;
  }

  McData = (MC_BIOS_DATA *)((MchReg->MCHBAR << 17) + 0x5e04);
  MemVddqVoltage = McData->VDDQ_TX_VOLTAGE * 5;
  Print(L"MchRegAddr: 0X%x, MemVddqVoltage: %d(0x%x * 5)mV\n", MchRegAddr, MemVddqVoltage, McData->VDDQ_TX_VOLTAGE);

  return EFI_SUCCESS;
}