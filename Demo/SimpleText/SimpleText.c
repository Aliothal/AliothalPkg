#include "SimpleText.h"

EFI_STATUS
SimpleTextEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                      Status = EFI_SUCCESS;
  // EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut = gST->ConOut;
  UINT16                          BltCount = 0;
  UINT8                           ColorWeight = 0;

  gMouseBlt = AllocateZeroPool(MOUSE_WEIGHT * MOUSE_HEIGHT * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  
  for (UINT8 i = 0; i < MOUSE_HEIGHT; i++) {
    for (UINT8 j = 0; j < MOUSE_WEIGHT; j++) {
      switch (zMouse[i][j] & 0xf0) {
        case 0x10:
          ColorWeight = 0x00;
          break;
        case 0x20:
          ColorWeight = 0x30;
          break;
        case 0x40:
          ColorWeight = 0x98;
          break;
        case 0x80:
          ColorWeight = 0xff;
          break;
      }
      if (zMouse[i][j] & 0x01)
        gMouseBlt[BltCount].Blue = ColorWeight;
      if (zMouse[i][j] & 0x02)
        gMouseBlt[BltCount].Green = ColorWeight;
      if (zMouse[i][j] & 0x04)
        gMouseBlt[BltCount].Red = ColorWeight;
      Print(L"{0x%02x, 0x%02x, 0x%02x, 0x%02x},",
      gMouseBlt[BltCount].Blue, gMouseBlt[BltCount].Green, gMouseBlt[BltCount].Red, gMouseBlt[BltCount].Reserved);
      BltCount++;
      if (BltCount % 16 == 0)
        Print(L"\n");
    }
  }

  Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, &Gop);
  Status = Gop->Blt(Gop, gMouseBlt, EfiBltBufferToVideo, 0, 0, 100, 100, MOUSE_WEIGHT, MOUSE_HEIGHT, 0);

  UINTN           FsNumber = 0;
  EFI_HANDLE      *FsList = NULL;

  Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &FsNumber, &FsList);
  
  for (UINT8 i = 0; i < FsNumber; i++) {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs = NULL;
    // EFI_FILE_PROTOCOL *Root = NULL;
    // EFI_FILE_PROTOCOL *FileHandle = NULL;
    // UINTN BufferSize = MOUSE_WEIGHT * MOUSE_HEIGHT * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    Status = gBS->HandleProtocol(FsList[i], &gEfiSimpleFileSystemProtocolGuid, &Fs);
    EFI_USB_IO_PROTOCOL *UsbIo = NULL;
    Status = gBS->HandleProtocol(FsList[i], &gEfiUsbIoProtocolGuid, &UsbIo);
    Print(L"%d Handle UsbIo Status:%r FsHandle:0x%x\n", i, Status, FsList[i]);
    // UsbIo->UsbGetDeviceDescriptor(UsbIo,)
    // Status = Fs->OpenVolume(Fs, &Root);
    // Status = Root->Open(Root, &FileHandle, L"MouseBlt.h", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    // FileHandle->Write(FileHandle, MOUSE_WEIGHT * MOUSE_HEIGHT * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL), gMouseBlt);
    EFI_DEVICE_PATH_PROTOCOL *Dp= NULL;
    Dp = DevicePathFromHandle(FsList[i]);
    CHAR16 *PathName = ConvertDevicePathToText(Dp, TRUE, TRUE);
    Print(PathName);
    Print(L"\n");
  }
  FreePool(FsList);
  FreePool(gMouseBlt);

  return Status;
}