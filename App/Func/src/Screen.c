#include "../Fun.h"

typedef struct {
  INT32 x, y, w, h;
} CURSOR_IMAGE_STATE;


SCREEN_DEV *gScreenDev = NULL;

EFI_STATUS DispInit(VOID)
{
  EFI_STATUS  Status = EFI_SUCCESS;
  UINTN       SizeOfInfo = 0;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info = NULL;
  if (gScreenDev != NULL)
    return EFI_ALREADY_STARTED;
  gScreenDev = AllocateZeroPool(sizeof(SCREEN_DEV));
  Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, &gScreenDev->Gop);
  for (UINT32 i = 0; i < gScreenDev->Gop->Mode->MaxMode; i++) {
    gScreenDev->Gop->QueryMode(gScreenDev->Gop, i, &SizeOfInfo, &Info);
    Print(L"Mode %d: Resolution - %d * %d\n", i, Info->HorizontalResolution, Info->VerticalResolution);
  }

  UINTN ReadSize = 10;
  CHAR16 *ModeString = AllocateZeroPool(10);
  UINT32 ModeNumber = 0;

  while (TRUE) {
    Print(L"Please select mode('q' to quit): \n");
    Status = gEfiShellProtocol->ReadFile(gEfiShellParametersProtocol->StdIn, &ReadSize, ModeString);
    ModeNumber = (UINT32)ShellStrToUintn(ModeString);
    if (ModeNumber < gScreenDev->Gop->Mode->MaxMode) {
      gScreenDev->Gop->SetMode(gScreenDev->Gop, ModeNumber);
      break;
    } else if (ModeString[0] == L'q') {
      break;
    }
  }
  FreePool(ModeString);
  gST->ConOut->EnableCursor(gST->ConOut, FALSE);
  gST->ConOut->ClearScreen(gST->ConOut);

  return Status;
}

VOID DispClose(VOID)
{
  if (gScreenDev->Gop != NULL)
    gScreenDev->Gop = NULL;
  if (gScreenDev != NULL) {
    FreePool(gScreenDev);
    gScreenDev = NULL;
  }
}

VOID GetScreenResolution(UINT32 *h, UINT32 *v)
{
  *h = gScreenDev->Gop->Mode->Info->HorizontalResolution;
  *v = gScreenDev->Gop->Mode->Info->VerticalResolution;
}

VOID VideoToBuffer(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Buffer, UINTN SourceX, UINTN SourceY, UINTN DestinationX, UINTN DestinationY, UINTN Width, UINTN Height, UINTN Delta)
{
  gScreenDev->Gop->Blt( gScreenDev->Gop,
                        Buffer,
                        EfiBltVideoToBltBuffer,
                        SourceX, SourceY,
                        DestinationX, DestinationY,
                        Width, Height,
                        Delta);
}

VOID BufferToVideo(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Buffer, UINTN SourceX, UINTN SourceY, UINTN DestinationX, UINTN DestinationY, UINTN Width, UINTN Height, UINTN Delta)
{
  gScreenDev->Gop->Blt( gScreenDev->Gop,
                        Buffer,
                        EfiBltBufferToVideo,
                        SourceX, SourceY,
                        DestinationX, DestinationY,
                        Width, Height,
                        Delta);
}

VOID ComposeNewZone(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *NewZone, EFI_GRAPHICS_OUTPUT_BLT_PIXEL *TopZone, INT32 NewWidth, INT32 NewHeight, INT32 TopWidth, INT32 TopHeight)
{
  INT64     IndexTop = 0, IndexNew = 0;
  UINT8     Alpha = 0;
  UINT16    Temp = 0;

  for (UINT16 i = 0; i < MIN(NewHeight, TopHeight); i++) {
    IndexTop = i * TopWidth;
    for (UINT16 j = 0; j < MIN(NewWidth, TopWidth); j++) {
      Alpha = 0xFF - TopZone[IndexTop].Reserved;
      Temp = NewZone[IndexNew].Blue * Alpha + TopZone[IndexTop].Blue * TopZone[IndexTop].Reserved + 0x80;
      NewZone[IndexNew].Blue = (UINT8)((Temp + (Temp >> 8)) >> 8);
      Temp = NewZone[IndexNew].Green * Alpha + TopZone[IndexTop].Green * TopZone[IndexTop].Reserved + 0x80;
      NewZone[IndexNew].Green = (UINT8)((Temp + (Temp >> 8)) >> 8);
      Temp = NewZone[IndexNew].Red * Alpha + TopZone[IndexTop].Red * TopZone[IndexTop].Reserved + 0x80;
      NewZone[IndexNew].Red = (UINT8)((Temp + (Temp >> 8)) >> 8);
      IndexNew++;
      IndexTop++;
    }
  }
}

VOID DrawCursor(CURSOR_IMAGE image)
{
  CURSOR_IMAGE_STATE last, current;
  last.w = image.W;
  last.h = image.H;
  GetCursorPos(&last.x, &last.y);

  if (last.)


}