#include "SpdInfo.h"

UINT16      gDimmNum = 0;
LIST_ENTRY  gMemoryDeviceList = INITIALIZE_LIST_HEAD_VARIABLE (gMemoryDeviceList);

EFI_STATUS GetSpd4IoByteData(UINT8 Slave, UINT8 *SpdData)
{
  IoWrite8(0xEFA0 + HSTS_OFFSET, 0xFF); //Check复查放在切page前还是后
  IoWrite8(0xEFA0 + TSA_OFFSET, 0x6C);
  IoWrite8(0xEFA0 + HCTL_OFFSET, 0x40);  //Page 0

  IoWrite8(0xEFA0 + TSA_OFFSET, (Slave << 1) | 1);
  for (UINT8 i = 0; i < 0xFF; i++) {
    IoWrite8(0xEFA0 + HSTS_OFFSET, 0x42);
    IoWrite8(0xEFA0 + HD0_OFFSET, 0);
    IoWrite8(0xEFA0 + HCMD_OFFSET, i);
    IoWrite8(0xEFA0 + HCTL_OFFSET, 0x48);
    while (IoRead8(0xEFA0 + HSTS_OFFSET) & 0x01) {}
    SpdData[i] = IoRead8(0xEFA0 + HD0_OFFSET);
  }
  IoWrite8(0xEFA0 + TSA_OFFSET, 0x6E);
  IoWrite8(0xEFA0 + HCTL_OFFSET, 0x40);  //Page 1
  IoWrite8(0xEFA0 + TSA_OFFSET, (Slave << 1) | 1);
  for (UINT8 i = 0; i < 0xFF; i++) {
    IoWrite8(0xEFA0 + HSTS_OFFSET, 0x42);
    IoWrite8(0xEFA0 + HD0_OFFSET, 0);
    IoWrite8(0xEFA0 + HCMD_OFFSET, i);
    IoWrite8(0xEFA0 + HCTL_OFFSET, 0x48);
    while (IoRead8(0xEFA0 + HSTS_OFFSET) & 0x01) {}
    SpdData[i + 0xFF] = IoRead8(0xEFA0 + HD0_OFFSET);
  }
  IoWrite8(0xEFA0 + TSA_OFFSET, 0x6C);
  IoWrite8(0xEFA0 + HCTL_OFFSET, 0x40);  //Page 0

  return 0;
}

EFI_STATUS GetSpd5IoByteData(UINT8 Slave, UINT8 *SpdData)
{
  IoWrite8(0xEFA0 + HSTS_OFFSET, 0xFF);
  IoWrite8(0xEFA0 + TSA_OFFSET, 0x6C);
  IoWrite8(0xEFA0 + HCTL_OFFSET, 0x40);  //Page 0

  IoWrite8(0xEFA0 + TSA_OFFSET, (Slave << 1) | 1);
  for (UINT8 i = 0; i < 0xFF; i++) {
    IoWrite8(0xEFA0 + HSTS_OFFSET, 0x42);
    IoWrite8(0xEFA0 + HD0_OFFSET, 0);
    IoWrite8(0xEFA0 + HCMD_OFFSET, i);
    IoWrite8(0xEFA0 + HCTL_OFFSET, 0x48);
    while (IoRead8(0xEFA0 + HSTS_OFFSET) & 0x01) {}
    SpdData[i] = IoRead8(0xEFA0 + HD0_OFFSET);
  }
  IoWrite8(0xEFA0 + TSA_OFFSET, 0x6E);
  IoWrite8(0xEFA0 + HCTL_OFFSET, 0x40);  //Page 1
  IoWrite8(0xEFA0 + TSA_OFFSET, (Slave << 1) | 1);
  for (UINT8 i = 0; i < 0xFF; i++) {
    IoWrite8(0xEFA0 + HSTS_OFFSET, 0x42);
    IoWrite8(0xEFA0 + HD0_OFFSET, 0);
    IoWrite8(0xEFA0 + HCMD_OFFSET, i);
    IoWrite8(0xEFA0 + HCTL_OFFSET, 0x48);
    while (IoRead8(0xEFA0 + HSTS_OFFSET) & 0x01) {}
    SpdData[i + 0xFF] = IoRead8(0xEFA0 + HD0_OFFSET);
  }
  IoWrite8(0xEFA0 + TSA_OFFSET, 0x6C);
  IoWrite8(0xEFA0 + HCTL_OFFSET, 0x40);  //Page 0

  return 0;
}

EFI_STATUS GetSpd5IoI2C(UINT8 Slave, UINT8 *SpdData)
{
  IoWrite8(0xEFA0 + HSTS_OFFSET, 0xFF);

  IoWrite8(0xEFA0 + HD0_OFFSET, 0);
  IoWrite8(0xEFA0 + HBD_OFFSET, 0);
  IoWrite8(0xEFA0 + HD1_OFFSET, 0x80);
  IoWrite8(0xEFA0 + TSA_OFFSET, (Slave << 1) | 1);
  IoWrite8(0xEFA0 + HCMD_OFFSET, 0);
  IoWrite8(0xEFA0 + HCTL_OFFSET, 0x58);

  for (UINT16 i = 0; i < 1024; i++) {
    while ((IoRead8(0xEFA0 + HSTS_OFFSET) & 0x80) == 0) {}
    SpdData[i] = IoRead8(0xEFA0 + HBD_OFFSET);
    if (i == 1022)
      IoWrite8(0xEFA0 + HCTL_OFFSET, 0x38);
    IoWrite8(0xEFA0 + HSTS_OFFSET, 0x80);
  }

  return 0;
}

EFI_STATUS
SpdInfoEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;

  // HD1_BYTE      Hd1;
  // HBD_BYTE      Hbd;

  // gDimmNum = GetNumberOfDimm();
  // AllocateMemoryDevice(gDimmNum);

  // FreeMemoryDevice();
  UINT8 *Data = AllocateZeroPool(1024);
  GetSpd5IoI2C(0x51, Data);
  DumpHex(2, 0, 1024, Data);
  FreePool(Data);
  return Status;
}

UINT16 GetNumberOfDimm(VOID)
{
  SMBIOS_TABLE_TYPE16           *SmbiosType16 = NULL;
  UINT16                        Type16Length = 0;

  if (LibGetSmbiosStructure(SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, 0, (UINT8 **)&SmbiosType16, &Type16Length))
    return SmbiosType16->NumberOfMemoryDevices;
  else
    return 0;
}

VOID AllocateMemoryDevice(UINT16 NumberOf17)
{
  for (UINT16 i = 0; i < NumberOf17; i++) {
    SMBIOS_TABLE_TYPE17     *SmbiosType17 = NULL;
    UINT16                  Type17Length = 0;
    MEMORY_DEVICE_INFO      *MemDeviceInfo = NULL;
    if (LibGetSmbiosStructure(SMBIOS_TYPE_MEMORY_DEVICE, i, (UINT8 **)&SmbiosType17, &Type17Length)) {
      switch (SmbiosType17->MemoryType) {
        case MemoryTypeDdr4:
          MemDeviceInfo = AllocateZeroPool(sizeof(MEMORY_DEVICE_INFO) + TOTAL_SPD_DDR4_LENGTH - 1);
          MemDeviceInfo->SpdDataLen = TOTAL_SPD_DDR4_LENGTH;
          GetSpd4Data((UINT8)(i + 0x50), MemDeviceInfo->SpdData);
          break;
        case MemoryTypeDdr5:
          MemDeviceInfo = AllocateZeroPool(sizeof(MEMORY_DEVICE_INFO) + TOTAL_SPD_DDR5_LENGTH - 1);
          MemDeviceInfo->SpdDataLen = TOTAL_SPD_DDR5_LENGTH;
          break;
        case MemoryTypeUnknown:
          MemDeviceInfo = AllocateZeroPool(sizeof(MEMORY_DEVICE_INFO));
          MemDeviceInfo->SpdDataLen = 0;
          break;
        default:
          break;
      }
      MemDeviceInfo->Type17Index = i;
      MemDeviceInfo->SpdSlaveAddr = (UINT8)(i + 0x50);
      CopyMem(&MemDeviceInfo->Type17, SmbiosType17, sizeof(SMBIOS_TABLE_TYPE17));
      InsertTailList(&gMemoryDeviceList, &MemDeviceInfo->Link);
    }
  }
}

VOID FreeMemoryDevice(VOID)
{
  LIST_ENTRY              *Entry = NULL;
  MEMORY_DEVICE_INFO      *MemDeviceInfo = NULL;
  BASE_LIST_FOR_EACH(Entry, &gMemoryDeviceList) {
    MemDeviceInfo = MEMORY_DEVICE_INFO_FROM_THIS(Entry);
    if (MemDeviceInfo->Type17.MemoryType == MemoryTypeDdr4)
      Print(L"Ddr4 Spd Data[4]: 0x%x\n", MemDeviceInfo->SpdData[4]);
    FreePool(MemDeviceInfo);
  }
}