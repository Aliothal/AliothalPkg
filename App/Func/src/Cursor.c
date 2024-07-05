#include "../Fun.h"

EFI_SIMPLE_POINTER_PROTOCOL *gSimplePointer = NULL;
UINT8 ScaleX = 0, ScaleY = 0;
CURSOR_DEV *gCursorDev = NULL;

VOID CursorEventNotify(EFI_EVENT Event, void *Context)
{
  EFI_SIMPLE_POINTER_STATE State;
  UINT32 h, v;
  EFI_STATUS Status = gSimplePointer->GetState(gSimplePointer, &State);
  if (EFI_ERROR (Status))
    return;
  gCursorDev->State.X += (State.RelativeMovementX >> ScaleX) << 2;
  gCursorDev->State.Y += (State.RelativeMovementY >> ScaleY) << 2;
  gCursorDev->State.Z = State.RelativeMovementZ;
  gCursorDev->State.Left = State.LeftButton;
  gCursorDev->State.Right = State.RightButton;
  GetScreenResolution(&h, &v);
  if (gCursorDev->State.X < 0)
    gCursorDev->State.X = 0;
  if (gCursorDev->State.X > h - 1)
    gCursorDev->State.X = h - 1;
  if (gCursorDev->State.Y < 0)
    gCursorDev->State.Y = 0;
  if (gCursorDev->State.Y > v - 1)
    gCursorDev->State.Y = v - 1;
  return;
}

EFI_STATUS CursorInit(VOID)
{
  EFI_STATUS Status = EFI_SUCCESS;
  if (gSimplePointer != NULL)
    return EFI_ALREADY_STARTED;
  Status = gBS->HandleProtocol(gST->ConsoleInHandle, &gEfiSimplePointerProtocolGuid, &gSimplePointer);
  if (EFI_ERROR(Status)) {
    Status = gBS->LocateProtocol(&gEfiSimplePointerProtocolGuid, NULL, &gSimplePointer);
    if (EFI_ERROR(Status)) {
      gSimplePointer = NULL;
      return Status;
    }
  }
  gCursorDev = AllocateZeroPool(sizeof(CURSOR_DEV));
  UINT64 ResolutionX, ResolutionY;
  ResolutionX = gSimplePointer->Mode->ResolutionX;
  ResolutionY = gSimplePointer->Mode->ResolutionY;
  while (ResolutionX != 1) {
    ResolutionX >>= 1;
    ScaleX++;
  }
  while (ResolutionY != 1) {
    ResolutionY >>= 1;
    ScaleY++;
  }
  SetCursorImage(CursorShapeNormal, CURSOR_NORMAL_WIDTH, CURSOR_NORMAL_HEIGHT, CursorImageNormalX, CursorImageNormalY, CursorImageNormal);
  SetCursorImage(CursorShapeLink, CURSOR_LINK_WIDTH, CURSOR_LINK_HEIGHT, CursorImageLinkX, CursorImageLinkY, CursorImageLink);
  SetCursorImage(CursorShapeEdit, CURSOR_EDIT_WIDTH, CURSOR_EDIT_HEIGHT, CursorImageEditX, CursorImageEditY, CursorImageEdit);
  SetCursorImage(CursorShapeLoad, CURSOR_LOAD_WIDTH, CURSOR_LOAD_HEIGHT, CursorImageLoadX, CursorImageLoadY, CursorImageLoad);
  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL|EVT_TIMER, TPL_CALLBACK, CursorEventNotify, NULL, &gCursorDev->CursorEvent);
  Status = gBS->SetTimer(gCursorDev->CursorEvent, TimerPeriodic, 0x28000);//61Hz
  return Status;
}

EFI_STATUS CursorClose(VOID)
{
  if (gSimplePointer == NULL)
    return EFI_NOT_STARTED;
  gBS->CloseEvent(gCursorDev->CursorEvent);
  gSimplePointer = NULL;
  for (UINT8 i = 0; i < gCursorDev->ImageCount; i++) {
    FreePool(gCursorDev->Images[i].Pixel);
    gCursorDev->Images[i].Pixel = NULL;
  }
  if (gCursorDev->ImageCount != 0) {
    FreePool(gCursorDev->Images);
    gCursorDev->Images = NULL;
    gCursorDev->ImageCount = 0;
  }
  if (gCursorDev) {
    FreePool(gCursorDev);
    gCursorDev = NULL;
  }
  return EFI_SUCCESS;
}

VOID SetCursorPos(INT32 x, INT32 y)
{
  gCursorDev->State.X = x;
  gCursorDev->State.Y = y;
}

VOID GetCursorPos(INT32 *x, INT32 *y)
{
  *x = gCursorDev->State.X;
  *y = gCursorDev->State.Y;
}

UINT8 FindCursorImage(CURSOR_SHAPE_TYPE type)
{
  for (UINT8 i = 0; i < gCursorDev->ImageCount; i++) {
    if (gCursorDev->Images->Type == type)
      return i;
  }
  return 0xff;
}

VOID SetCursorImage(CURSOR_SHAPE_TYPE type, INT32 w, INT32 h, INT32 x, INT32 y, const EFI_GRAPHICS_OUTPUT_BLT_PIXEL *image)
{
  UINT8 Index = FindCursorImage(type);
  if (Index != 0xff) {
    FreePool(gCursorDev->Images[Index].Pixel);
    gCursorDev->Images[Index].Pixel = AllocateZeroPool(w * h * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    gCursorDev->Images[Index].W = w;
    gCursorDev->Images[Index].H = h;
    gCursorDev->Images[Index].X = x;
    gCursorDev->Images[Index].Y = y;
    gCursorDev->Images[Index].Type = type;
    CopyMem(gCursorDev->Images[Index].Pixel, image, w * h * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  } else {
    gCursorDev->Images = ReallocatePool(gCursorDev->ImageCount * sizeof(CURSOR_IMAGE),
                                       (gCursorDev->ImageCount + 1) * sizeof(CURSOR_IMAGE),
                                        gCursorDev->Images);
    gCursorDev->Images[gCursorDev->ImageCount].Pixel = AllocateZeroPool(w * h * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    gCursorDev->Images[gCursorDev->ImageCount].W = w;
    gCursorDev->Images[gCursorDev->ImageCount].H = h;
    gCursorDev->Images[gCursorDev->ImageCount].X = x;
    gCursorDev->Images[gCursorDev->ImageCount].Y = y;
    gCursorDev->Images[gCursorDev->ImageCount].Type = type;
    CopyMem(gCursorDev->Images[gCursorDev->ImageCount].Pixel, image, w * h * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    gCursorDev->ImageCount++;
  }
}

VOID GetCursorImage(CURSOR_SHAPE_TYPE type, CURSOR_IMAGE *images)
{
  UINT8 Index = FindCursorImage(type);
  if (Index != 0xff) {
    CopyMem(images, &gCursorDev->Images[Index], sizeof(CURSOR_IMAGE));
  } else {
    images = NULL;
  }
}