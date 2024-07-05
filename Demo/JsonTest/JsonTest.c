#include <Library/FileHandleLib.h>
#include <Library/ShellLib.h>
#include <Library/cJSONLib.h>
#include <Library/BaseCryptLib.h>

EFI_STATUS
EntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                    Status = EFI_SUCCESS;
  SHELL_FILE_HANDLE             FileHandle;
  UINT64                        Size;

  Status = ShellOpenFileByName(L"test.json", &FileHandle, EFI_FILE_MODE_READ, 0);
  if (Status == EFI_SUCCESS)
    Status = FileHandleGetSize(FileHandle, &Size);
  else
    return Status;
  CHAR8 *Buffer = AllocateZeroPool(Size);
  Status = ShellReadFile(FileHandle, &Size, Buffer);
  Status = ShellCloseFile(&FileHandle);

  cJSON *Tree = NULL;
  Tree = cJSON_ParseWithLength(Buffer, Size);
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Aes128CbcKey[] = {
  0xc2, 0x86, 0x69, 0x6d, 0x88, 0x7c, 0x9a, 0xa0, 0x61, 0x1b, 0xbb, 0x3e, 0x20, 0x25, 0xa4, 0x5a
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Aes128CbcIvec[] = {
  0x56, 0x2e, 0x17, 0x99, 0x6d, 0x09, 0x3d, 0x28, 0xdd, 0xb3, 0xba, 0x69, 0x5a, 0x2e, 0x6f, 0x58
};

// GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Aes128CbcCipher[] = {
//   0xd2, 0x96, 0xcd, 0x94, 0xc2, 0xcc, 0xcf, 0x8a, 0x3a, 0x86, 0x30, 0x28, 0xb5, 0xe1, 0xdc, 0x0a,
//   0x75, 0x86, 0x60, 0x2d, 0x25, 0x3c, 0xff, 0xf9, 0x1b, 0x82, 0x66, 0xbe, 0xa6, 0xd6, 0x1a, 0xb1
// };
  VOID *AesContext = AllocatePool(AesGetContextSize());

  AesInit(AesContext, Aes128CbcKey, 128);

  CHAR8 *str = cJSON_PrintUnformatted(Tree);
  UINTN strlen = AsciiStrLen(str);
  UINTN InputSize = ALIGN_VALUE(strlen, 16);
  UINT8 *Input = AllocateZeroPool(InputSize);
  CopyMem(Input, str, strlen);
  UINT8 *Output = AllocatePool(InputSize);

  BOOLEAN Ok = AesCbcEncrypt(AesContext, Input, InputSize, Aes128CbcIvec, Output);
  Print(L"AesCbcEncrypt %d\n", Ok);
  Ok = AesCbcDecrypt(AesContext, Output, InputSize, Aes128CbcIvec, Input);
  Print(L"AesCbcDecrypt %d\n", Ok);

  // cJSON *Spd = cJSON_AddStringToObject(Tree, "SPD", "");
  // cJSON_SetValuestring(Spd, "sssspppp");

  // cJSON *Item = cJSON_GetObjectItemCaseSensitive(Tree, "a");
  // cJSON_DeleteItemFromObjectCaseSensitive(Tree, "a");
  // if (Item)
  //   cJSON_SetIntValue(Item, 31);
  // Print(L"%d\n", Item->valueint);
  // /*cJSON *SlotInfo = */cJSON_AddArrayToObject(Tree, "SLOTINFO");
  // for (UINT8 i = 0; i < 5; i++) {
  //     cJSON *Info = cJSON_CreateObject();
  //     cJSON_AddNumberToObject(Info, "ONLINE", 1);
  //     cJSON_AddNumberToObject(Info, "TEMP", 55);
  //     cJSON_AddItemToArray(SlotInfo, Info);
  // }
  // cJSON_AddBoolToObject(Tree, "SUCCESS", TRUE);

  cJSON *Tree1 = NULL;
  Tree1 = cJSON_ParseWithLength(Input, InputSize);
  Print(L"%a\n", cJSON_PrintUnformatted(Tree1));

  cJSON_Delete(Tree);
  cJSON_Delete(Tree1);
  FreePool(Buffer);
  FreePool(AesContext);
  FreePool(Output);
  FreePool(Input);
  return Status;
}