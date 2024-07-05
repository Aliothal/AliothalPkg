#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FileHandleLib.h>
#include <Protocol/GraphicsOutput.h>

#include "Gif.h"

typedef struct {
  UINT64        Index;
  UINT16        Left;
  UINT16        Top;
  UINT16        Width;
  UINT16        Height;
  UINT8         LctFlage;
  UINT16        LctSize;
  GIF_COLOR     *Lct;
  UINT8         SortFlag;
  UINT8         InterlaceFlag;
} GIF_FRAME_INFO;

typedef struct {
  UINT64        GifSize;
  UINT64        AppExCount;
  UINT64        CmtExCount;
  UINT64        GcExCount;
  UINT64        PtExCount;
  UINT64        ImgDscCount;
  // Header
  UINT8         Version;
  UINT16        LogicWidth;
  UINT16        LogicHeight;
  UINT8         GctFlag;
  UINT16        GctSize;
  GIF_COLOR     *Gct;
  UINT8         GctBackIndex;
  UINT8         GctColorRes;
  UINT8         GctSortFlag;
  // double        AspectRatio;    //LibC only

  // Application Extension
  UINT16        LoopCount;

  // Comment Extension

  // Graphic Control Extension
  UINT8         TransparentColorFlag;
  UINT8         TransparentColorIndex;
  UINT8         UserInputFlag;
  UINT8         DisposalMethod;
  UINT16        DelayTime;

  // Plain Text Extension

  // Image Descriptor
  GIF_FRAME_INFO *FrameList;
} GIF_INFO;

GIF_INFO   GifInfo;

EFI_STATUS ReadGifBuffer(UINT8 **Buffer)
{
  EFI_STATUS                    Status = EFI_SUCCESS;
  SHELL_FILE_HANDLE             FileHandle;
  UINT64                        Size;
  
  if (gEfiShellParametersProtocol->Argc != 2) {
    Print(L"ShowGif <Gif>\n");
    return EFI_UNSUPPORTED;
  }
  Status = ShellOpenFileByName(L"Panda.gif", &FileHandle, EFI_FILE_MODE_READ, 0);
  Status = FileHandleGetSize(FileHandle, &Size);
  *Buffer = AllocateZeroPool(Size);
  Status = ShellReadFile(FileHandle, &Size, *Buffer);
  GifInfo.GifSize = Size;
  Status = ShellCloseFile(&FileHandle);
  GifInfo.AppExCount = 0;
  GifInfo.CmtExCount = 0;
  GifInfo.GcExCount = 0;
  GifInfo.PtExCount = 0;
  GifInfo.ImgDscCount = 0;

  return Status;
}

BOOLEAN GifHeaderDecode(UINT8 *Buffer)
{
  GIF_HEADER *Header;
  CopyMem(Header, Buffer, sizeof(GIF_HEADER));
  if (CompareMem(Header->Signature, "GIF", 3)) {
    Print(L"gif format error\n");
    return FALSE;
  }
  if (CompareMem(Header->Version, "87a", 3) == 0) {
    GifInfo.Version = 1;
    Print(L"gif 87a is not supported\n");
    return FALSE;
  } else if (CompareMem(Header->Version, "89a", 3) == 0) {
    GifInfo.Version = 2;
  } else {
    Print(L"gif format error\n");
    return FALSE;
  }
  GifInfo.LogicWidth = Header->Width;
  GifInfo.LogicHeight = Header->Height;
  GifInfo.GctFlag = Header->GctFlag;
  if (Header->GctFlag) {
    GifInfo.GctSize = (1 << (Header->GctSize + 1)) * 3;
    if (GifInfo.GifSize < GifInfo.GctSize + sizeof(GIF_HEADER)) {
      Print(L"gif size error\n");
      return FALSE;
    }
    GifInfo.GctBackIndex = Header->BackgroundColorIndex;
    GifInfo.Gct = AllocateCopyPool(GifInfo.GctSize, Buffer + sizeof(GIF_HEADER));
  } else {
    GifInfo.GctSize = 0;
    GifInfo.GctBackIndex = 0xff;
    GifInfo.Gct = NULL;
  }
  GifInfo.GctColorRes = Header->ColorRes + 1;
  GifInfo.GctSortFlag = Header->SortFlag;
  // if (Header->PixelAspectRatio)
  //   GifInfo.AspectRatio = (Header->PixelAspectRatio + 15) / 64;
  // else
  //   GifInfo.AspectRatio = 0;
  return TRUE;
}


BOOLEAN GifExtensionDecode(UINT8 *Buffer)
{
  UINT64              Offset = 0;

  GIF_EXTENSION_HEADER  *ExHeader;
  CopyMem(ExHeader, Buffer + Offset, sizeof(GIF_EXTENSION_HEADER));
  if (ExHeader->Introducer == '!') {
    switch (ExHeader->Label) {
      case 0xFF:
        GIF_APPLICATION_EXTENSION *ExApp;
        CopyMem(ExApp, Buffer + Offset, sizeof(GIF_APPLICATION_EXTENSION));
        if (ExApp->Size != 0x0B || CompareMem(ExApp->Identifier, "NETSCAPE2.0", 11) || ExApp->SubSize != 0x03) {
          Print(L"ExApp verify error\n");
          return FALSE;
        }
        GifInfo.LoopCount = ExApp->LoopCount;
        Offset += (sizeof(GIF_APPLICATION_EXTENSION) + 1);
        GifInfo.AppExCount++;
        break;
      case 0xFE:
        GIF_COMMENT_EXTENSION *ExComment;
        CopyMem(ExComment, Buffer + Offset, sizeof(GIF_COMMENT_EXTENSION));
        Offset += (ExComment->Size + sizeof(GIF_COMMENT_EXTENSION) + 1);
        GifInfo.CmtExCount++;
        break;
      case 0xF9:
        GIF_GRAPHIC_CONTROL_EXTENSION *ExGc;
        CopyMem(ExGc, Buffer + Offset, sizeof(GIF_GRAPHIC_CONTROL_EXTENSION));
        if (ExGc->Size != 0x04) {
          Print(L"ExGc verify error\n");
          return FALSE;
        }
        GifInfo.TransparentColorFlag = ExGc->TransparentColorFlag;
        GifInfo.TransparentColorIndex = ExGc->TransparentColorIndex;
        GifInfo.UserInputFlag = ExGc->UserInputFlag;
        GifInfo.DisposalMethod = ExGc->DisposalMethod;
        GifInfo.DelayTime = ExGc->DelayTime;
        Offset += (sizeof(GIF_GRAPHIC_CONTROL_EXTENSION) + 1);
        GifInfo.GcExCount++;
        break;
      case 0x01:
        GIF_PLAIN_TEXT_EXTENSION *ExPt;
        CopyMem(ExPt, Buffer + Offset, sizeof(GIF_PLAIN_TEXT_EXTENSION));
        if (ExPt->Size != 0x0C) {
          Print(L"ExPt verify error\n");
          return FALSE;
        }
        UINT8 *Ptr = Buffer + Offset + sizeof(GIF_PLAIN_TEXT_EXTENSION);
        while (*Ptr++) {
          Offset++;
        }
        Offset += sizeof(GIF_PLAIN_TEXT_EXTENSION) + 1;
        GifInfo.PtExCount++;
        break;
      default:
        Print(L"unkown extension\n");
        return FALSE;
    }
  } else if (ExHeader->Introducer == ',') {
    GIF_IMAGE_DESCRIPTOR *ImageDsc;
    CopyMem(ImageDsc, Buffer + Offset, sizeof(GIF_IMAGE_DESCRIPTOR));
    
  } else if (ExHeader->Introducer == ';') {
    return TRUE;
  } else {
    Print(L"gif format error\n");
    return FALSE;
  }
}

EFI_STATUS
ShowGifEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS              Status = EFI_SUCCESS;
  UINT8                   *Buffer;

  Status = ReadGifBuffer(&Buffer);
  if (EFI_ERROR(Status))
    return Status;
  if (!GifHeaderDecode(Buffer + GifInfo.GctSize + sizeof(GIF_HEADER)))
    return Status;
  Print(L"offset - %d\n", GifInfo.GctSize + sizeof(GIF_HEADER));
  SHELL_FREE_NON_NULL(GifInfo.Gct);
  SHELL_FREE_NON_NULL(Buffer);
  return Status;
}