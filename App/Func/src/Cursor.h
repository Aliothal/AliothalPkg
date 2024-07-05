#ifndef _CURSOR_H_
#define _CURSOR_H_

#include "../image/normal.h"
#include "../image/link.h"
#include "../image/edit.h"
#include "../image/load.h"

typedef struct {
  INT32      X;
  INT32      Y;
  INT32      Z;
  BOOLEAN    Left;
  BOOLEAN    Right;
} CURSOR_STATE;

typedef enum {
  CursorShapeNormal = 0x0,
  CursorShapeLink = 0x1,
  CursorShapeEdit = 0x2,
  CursorShapeLoad = 0x3,
} CURSOR_SHAPE_TYPE;

typedef struct {
  CURSOR_SHAPE_TYPE Type;
  INT32 W, H, X, Y;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Pixel;
} CURSOR_IMAGE;

typedef struct {
  CURSOR_STATE      State;
  EFI_EVENT         CursorEvent;
  UINT8             ImageCount;
  CURSOR_IMAGE      *Images;
} CURSOR_DEV;

EFI_STATUS CursorInit(VOID);
EFI_STATUS CursorClose(VOID);
VOID SetCursorPos(INT32 x, INT32 y);
VOID GetCursorPos(INT32 *x, INT32 *y);
VOID SetCursorImage(CURSOR_SHAPE_TYPE type, INT32 w, INT32 h, INT32 x, INT32 y, const EFI_GRAPHICS_OUTPUT_BLT_PIXEL *image);
VOID GetCursorImage(CURSOR_SHAPE_TYPE type, CURSOR_IMAGE *images);

#endif