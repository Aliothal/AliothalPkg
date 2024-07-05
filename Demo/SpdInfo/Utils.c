#include "SpdInfo.h"

STATIC SMBIOS_TABLE_ENTRY_POINT			*mSmbiosTable = NULL;
STATIC SMBIOS_STRUCTURE_POINTER			m_SmbiosStruct;
STATIC SMBIOS_STRUCTURE_POINTER			*mSmbiosStruct = &m_SmbiosStruct;

INT16 Crc16(char *ptr, int count)
{
  INT16 crc, i;
  crc = 0;
  while (--count >= 0)
  {
  	crc = crc ^ (INT16)(int)*ptr++ << 8;
  	for (i = 0; i < 8; ++i)
  	{
	  if (crc & 0x8000)
	  	crc = crc << 1 ^ 0x1021;
	  else
	  	crc = crc << 1;
  	}
  }
  return (crc & 0xFFFF);
}

static void trim(char* str)
{
  if(!str)
  	return;
  char* ptr = str;
  UINTN len = AsciiStrLen(ptr);
  while(len-1 > 0 && ptr[len-1] == ' ')
  	ptr[--len] = 0;
  
  while(*ptr && *ptr == ' ')
  	++ptr, --len;
  CopyMem(str, ptr, len + 1);
}

EFI_STATUS LibSmbiosInit(VOID)
{
  EFI_STATUS  Status;

  Status = EfiGetSystemConfigurationTable (&gEfiSmbiosTableGuid, (VOID**)&mSmbiosTable);
  if (mSmbiosTable == NULL) {
    return EFI_NOT_FOUND;
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }
  mSmbiosStruct->Raw  = (UINT8 *)(UINTN)(mSmbiosTable->TableAddress);
  return EFI_SUCCESS;
}

CHAR8* LibGetSmbiosString(SMBIOS_STRUCTURE_POINTER *Smbios, UINT16 StringNumber)
{
  UINT16  Index;
  CHAR8   *String;

  String = (CHAR8 *) (Smbios->Raw + Smbios->Hdr->Length);
  for (Index = 1; Index <= StringNumber; Index++) {
    if (StringNumber == Index)
      return String;
    for (; *String != 0; String++);
    String++;
    if (*String == 0) {
      Smbios->Raw = (UINT8 *)++String;
      return NULL;
    }
  }
  return NULL;
}

BOOLEAN LibGetSmbiosStructure(UINT8 Type, UINT16 Index, UINT8 **Buffer, UINT16 *Length)
{
  SMBIOS_STRUCTURE_POINTER  Smbios;
  SMBIOS_STRUCTURE_POINTER  SmbiosEnd;
  UINT8                     *Raw;
  UINT16                    Count = 0;

  if ((Buffer == NULL) || (Length == NULL))
    return FALSE;
  if (mSmbiosTable == NULL)
	  LibSmbiosInit();
  if (mSmbiosTable == NULL)
	  return FALSE;

  *Length       = 0;
  Smbios.Hdr    = mSmbiosStruct->Hdr;
  SmbiosEnd.Raw = Smbios.Raw + mSmbiosTable->TableLength;
  while (Smbios.Raw < SmbiosEnd.Raw) {
    if (Smbios.Hdr->Type == Type) {
      if (Index == Count)
      {
        Raw = Smbios.Raw;
        LibGetSmbiosString(&Smbios, (UINT16)(-1));
        *Length = (UINT16)(Smbios.Raw - Raw);
        *Buffer = Raw;
        return TRUE;
      }
		  Count++;
    }
    LibGetSmbiosString(&Smbios, (UINT16)(-1));
  }
  return FALSE;
}

BOOLEAN GetSpd4Data(UINT8 Slave, UINT8 *SpdData)
{
  EFI_STATUS        Status = 0;
  UINTN             Addr = 0;
  UINT16            Word = 0;

  SmBusQuickWrite(SMBUS_LIB_ADDRESS(0x36, 0, 1, FALSE), &Status);         //Page 0
  if (EFI_ERROR(Status))
    return FALSE;

  for (UINT16 i = 0; i < TOTAL_SPD_DDR4_LENGTH / 2; i += 2) {
    Addr = SMBUS_LIB_ADDRESS(Slave, i, 2, FALSE);
    Word = SmBusReadDataWord(Addr, &Status);
    SpdData[i] = (UINT8)Word;
    SpdData[i + 1] = (UINT8)(Word >> 8);
  }
  SmBusQuickWrite(SMBUS_LIB_ADDRESS(0x37, 0, 1, FALSE), &Status);         //Page 1
  for (UINT16 i = 0; i < TOTAL_SPD_DDR4_LENGTH / 2; i += 2) {
    Addr = SMBUS_LIB_ADDRESS(Slave, i, 2, FALSE);
    Word = SmBusReadDataWord(Addr, &Status);
    SpdData[i + TOTAL_SPD_DDR4_LENGTH / 2] = (UINT8)Word;
    SpdData[i + 1 + TOTAL_SPD_DDR4_LENGTH / 2] = (UINT8)(Word >> 8);
  }
  SmBusQuickWrite(SMBUS_LIB_ADDRESS(0x36, 0, 1, FALSE), &Status);         //Page 0
  if (Status == 0)
    return TRUE;
}
