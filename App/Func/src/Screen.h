#ifndef _SCREEN_H_
#define _SCREEN_H_

typedef struct {
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Buffer;
} SCREEN_DEV;

EFI_STATUS DispInit(VOID);
VOID DispClose(VOID);
VOID GetScreenResolution(UINT32 *h, UINT32 *v);
VOID VideoToBuffer(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Buffer, UINTN SourceX, UINTN SourceY, UINTN DestinationX, UINTN DestinationY, UINTN Width, UINTN Height, UINTN Delta);
VOID BufferToVideo(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Buffer, UINTN SourceX, UINTN SourceY, UINTN DestinationX, UINTN DestinationY, UINTN Width, UINTN Height, UINTN Delta);
VOID ComposeNewZone(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *NewZone, EFI_GRAPHICS_OUTPUT_BLT_PIXEL *TopZone, INT32 NewWidth, INT32 NewHeight, INT32 TopWidth, INT32 TopHeight);

#endif