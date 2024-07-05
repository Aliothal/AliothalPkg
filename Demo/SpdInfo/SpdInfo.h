#ifndef __SPD_INFO_H__
#define __SPD_INFO_H__

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ShellCommandLib.h>

#include <Library/IoLib.h>
#include <Library/SmbusLib.h>
#include <Guid/SmBios.h>
#include <IndustryStandard/SdramSpd.h>
#include <IndustryStandard/SmBios.h>

// #pragma pack (push, 1)
// #pragma pack (pop)

#define TOTAL_SPD_DDR4_LENGTH       512
#define TOTAL_SPD_DDR5_LENGTH       1024

typedef struct {
  LIST_ENTRY            Link;
  UINT16                Type17Index;
  SMBIOS_TABLE_TYPE17   Type17;
  UINT8                 SpdSlaveAddr;
  UINT16                SpdDataLen;
  UINT8                 SpdData[1];
} MEMORY_DEVICE_INFO;

#define MEMORY_DEVICE_INFO_FROM_THIS(a) BASE_CR(a, MEMORY_DEVICE_INFO, Link)

INT16 Crc16(char *ptr, int count);
EFI_STATUS LibSmbiosInit(VOID);
CHAR8 *LibGetSmbiosString(SMBIOS_STRUCTURE_POINTER *Smbios, UINT16 StringNumber);
BOOLEAN LibGetSmbiosStructure (UINT8 Type, UINT16 Index, UINT8 **Buffer, UINT16 *Length);
BOOLEAN GetSpd4Data(UINT8 Slave, UINT8 *SpdData);

UINT16 GetNumberOfDimm(VOID);
VOID AllocateMemoryDevice(UINT16 NumberOf17);
VOID FreeMemoryDevice(VOID);

// IO port
#define       HSTS_OFFSET     0
#define       HCTL_OFFSET     2
#define       HCMD_OFFSET     3
#define       TSA_OFFSET      4
#define       HD0_OFFSET      5
#define       HD1_OFFSET      6
#define       HBD_OFFSET      7

typedef union {
  struct { 
    UINT8       HBSY      : 1;
    UINT8       INTR      : 1;
    UINT8       DERR      : 1;
    UINT8       BERR      : 1;
    UINT8       FAIL      : 1;
    UINT8       SMSTS     : 1;
    UINT8       IUS       : 1;
    UINT8       BDS       : 1;
  } Bits;
  UINT8 Byte;
} HSTS_BYTE;

typedef union {
  struct {
    UINT8       INTREN    : 1;
    UINT8       KILL      : 1;
    UINT8       SMB_CMD   : 3;
    UINT8       LAST_BYTE : 1;
    UINT8       START     : 1;
    UINT8       PEC_EN    : 1;
  } Bits;
  UINT8 Byte;
} HCTL_BYTE;

typedef struct {
  UINT8 Offset;
} HCMD_BYTE;

typedef union {
  struct {
    UINT8       RW        : 1;
    UINT8       ADDR      : 7;
  } Bits;
  UINT8 Byte;
} TSA_BYTE;

typedef struct {
  UINT8 DATA0_COUNT;
} HD0_BYTE;

typedef struct {
  UINT8 DATA1;
} HD1_BYTE;

typedef struct {
  UINT8 BDTA;
} HBD_BYTE;

#endif