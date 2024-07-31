#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>

#include <Protocol/ShellParameters.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/UsbIo.h>
#include <Protocol/SimpleTextOut.h>
#include <Guid/FileInfo.h>
#include "EfiBinary.h"

#define SHELL_1_SUPPORT

#ifdef SHELL_1_SUPPORT
#include <Protocol/EfiShellInterface.h>
#include <Protocol/EfiShellEnvironment2.h>
#endif

static EFI_STATUS
TrimSpaces (
  IN CHAR16  **String
  )
{
  while (((*String)[0] == L' ') || ((*String)[0] == L'\t')) {
    CopyMem ((*String), (*String)+1, StrSize ((*String)) - sizeof ((*String)[0]));
  }

  while ((StrLen (*String) > 0) && (((*String)[StrLen ((*String))-1] == L' ') || ((*String)[StrLen ((*String))-1] == L'\t'))) {
    (*String)[StrLen ((*String))-1] = CHAR_NULL;
  }

  return (EFI_SUCCESS);
}

static CONST CHAR16 *
FindFirstCharacter (
  IN CONST CHAR16  *String,
  IN CONST CHAR16  *CharacterList,
  IN CONST CHAR16  EscapeCharacter
  )
{
  UINT32  WalkChar;
  UINT32  WalkStr;

  for (WalkStr = 0; WalkStr < StrLen (String); WalkStr++) {
    if (String[WalkStr] == EscapeCharacter) {
      WalkStr++;
      continue;
    }

    for (WalkChar = 0; WalkChar < StrLen (CharacterList); WalkChar++) {
      if (String[WalkStr] == CharacterList[WalkChar]) {
        return (&String[WalkStr]);
      }
    }
  }

  return (String + StrLen (String));
}

static CONST CHAR16 *
FindEndOfParameter (
  IN CONST CHAR16  *String
  )
{
  CONST CHAR16  *First;
  CONST CHAR16  *CloseQuote;

  First = FindFirstCharacter (String, L" \"", L'^');

  //
  // nothing, all one parameter remaining
  //
  if (*First == CHAR_NULL) {
    return (First);
  }

  //
  // If space before a quote (or neither found, i.e. both CHAR_NULL),
  // then that's the end.
  //
  if (*First == L' ') {
    return (First);
  }

  CloseQuote = FindFirstCharacter (First+1, L"\"", L'^');

  //
  // We did not find a terminator...
  //
  if (*CloseQuote == CHAR_NULL) {
    return (NULL);
  }

  return (FindEndOfParameter (CloseQuote+1));
}

static EFI_STATUS
GetNextParameter (
  IN OUT CHAR16   **Walker,
  IN OUT CHAR16   **TempParameter,
  IN CONST UINTN  Length,
  IN BOOLEAN      StripQuotation
  )
{
  CONST CHAR16  *NextDelim;

  if (  (Walker           == NULL)
     || (*Walker          == NULL)
     || (TempParameter    == NULL)
     || (*TempParameter   == NULL)
        )
  {
    return (EFI_INVALID_PARAMETER);
  }

  //
  // make sure we dont have any leading spaces
  //
  while ((*Walker)[0] == L' ') {
    (*Walker)++;
  }

  //
  // make sure we still have some params now...
  //
  if (StrLen (*Walker) == 0) {
    DEBUG_CODE_BEGIN ();
    *Walker = NULL;
    DEBUG_CODE_END ();
    return (EFI_INVALID_PARAMETER);
  }

  NextDelim = FindEndOfParameter (*Walker);

  if (NextDelim == NULL) {
    DEBUG_CODE_BEGIN ();
    *Walker = NULL;
    DEBUG_CODE_END ();
    return (EFI_NOT_FOUND);
  }

  StrnCpyS (*TempParameter, Length / sizeof (CHAR16), (*Walker), NextDelim - *Walker);

  //
  // Add a CHAR_NULL if we didn't get one via the copy
  //
  if (*NextDelim != CHAR_NULL) {
    (*TempParameter)[NextDelim - *Walker] = CHAR_NULL;
  }

  //
  // Update Walker for the next iteration through the function
  //
  *Walker = (CHAR16 *)NextDelim;

  //
  // Remove any non-escaped quotes in the string
  // Remove any remaining escape characters in the string
  //
  for (NextDelim = FindFirstCharacter (*TempParameter, L"\"^", CHAR_NULL)
       ; *NextDelim != CHAR_NULL
       ; NextDelim = FindFirstCharacter (NextDelim, L"\"^", CHAR_NULL)
       )
  {
    if (*NextDelim == L'^') {
      //
      // eliminate the escape ^
      //
      CopyMem ((CHAR16 *)NextDelim, NextDelim + 1, StrSize (NextDelim + 1));
      NextDelim++;
    } else if (*NextDelim == L'\"') {
      //
      // eliminate the unescaped quote
      //
      if (StripQuotation) {
        CopyMem ((CHAR16 *)NextDelim, NextDelim + 1, StrSize (NextDelim + 1));
      } else {
        NextDelim++;
      }
    }
  }

  return EFI_SUCCESS;
}

static EFI_STATUS
ParseCommandLineToArgs (
  IN CONST CHAR16  *CommandLine,
  IN BOOLEAN       StripQuotation,
  IN OUT CHAR16    ***Argv,
  IN OUT UINTN     *Argc
  )
{
  UINTN       Count;
  CHAR16      *TempParameter;
  CHAR16      *Walker;
  CHAR16      *NewParam;
  CHAR16      *NewCommandLine;
  UINTN       Size;
  EFI_STATUS  Status;

  if ((CommandLine == NULL) || (StrLen (CommandLine) == 0)) {
    (*Argc) = 0;
    (*Argv) = NULL;
    return (EFI_SUCCESS);
  }

  NewCommandLine = AllocateCopyPool (StrSize (CommandLine), CommandLine);
  if (NewCommandLine == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  TrimSpaces (&NewCommandLine);
  Size          = StrSize (NewCommandLine);
  TempParameter = AllocateZeroPool (Size);
  if (TempParameter == NULL) {
    if (NewCommandLine) {
      FreePool(NewCommandLine);
      NewCommandLine = NULL;
    }
    return (EFI_OUT_OF_RESOURCES);
  }

  for ( Count = 0,
        Walker = (CHAR16 *)NewCommandLine
        ; Walker != NULL && *Walker != CHAR_NULL
        ; Count++
        )
  {
    if (EFI_ERROR (GetNextParameter (&Walker, &TempParameter, Size, TRUE))) {
      break;
    }
  }

  (*Argv) = AllocateZeroPool ((Count)*sizeof (CHAR16 *));
  if (*Argv == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  *Argc  = 0;
  Walker = (CHAR16 *)NewCommandLine;
  while (Walker != NULL && *Walker != CHAR_NULL) {
    SetMem16 (TempParameter, Size, CHAR_NULL);
    if (EFI_ERROR (GetNextParameter (&Walker, &TempParameter, Size, StripQuotation))) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    NewParam = AllocateCopyPool (StrSize (TempParameter), TempParameter);
    if (NewParam == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    ((CHAR16 **)(*Argv))[(*Argc)] = NewParam;
    (*Argc)++;
  }

  Status = EFI_SUCCESS;

Done:
  if (TempParameter != NULL) {
    FreePool(TempParameter);
    TempParameter = NULL;
  }
  if (NewCommandLine != NULL) {
    FreePool(NewCommandLine);
    NewCommandLine = NULL;
  }
  return Status;
}


// If execute in uefi shell environment, ShellExecute() is recommended.
// If you only execute efi app in usb and do not enter the shell, you can use DevicePath and CommandLine.
  // (DevicePath != NULL) && (SourceBuffer == NULL) && (SourceSize == 0) is must
// If you want to add an efi app to the execution in binary form, you can use CommandLine, SourceBuffer, and SourceSize.
  // 
// CommandLine[0] set to EfiName
EFI_STATUS
EfiBinaryExecute (
  IN CONST EFI_DEVICE_PATH_PROTOCOL   *DevicePath OPTIONAL,
  IN CONST CHAR16                     *CommandLine,
  VOID                                *SourceBuffer OPTIONAL,
  UINTN                               SourceSize OPTIONAL,
  OUT EFI_STATUS                      *StartImageStatus OPTIONAL
  )
{
  EFI_STATUS Status = 0;
  EFI_SHELL_PARAMETERS_PROTOCOL ShellParamsProtocol;
#ifdef SHELL_1_SUPPORT
  EFI_SHELL_INTERFACE ShellInterface;
  EFI_SHELL_ENVIRONMENT2 ShellEnv2;
#endif
  CHAR16 *NewCmdLine = NULL;
  EFI_HANDLE NewHandle;
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  EFI_STATUS StartStatus;

  if (((DevicePath != NULL) && (SourceBuffer == NULL) && (SourceSize == 0)) ||
      ((DevicePath == NULL) && (SourceBuffer != NULL) && (SourceSize != 0))) {
        
  } else {
    return EFI_INVALID_PARAMETER;
  }
  NewCmdLine = AllocateCopyPool (StrSize (CommandLine), CommandLine);
  if (NewCmdLine == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  Status = gBS->LoadImage(FALSE, gImageHandle, (EFI_DEVICE_PATH_PROTOCOL *)DevicePath, SourceBuffer, SourceSize, &NewHandle);
  if (EFI_ERROR (Status)) {
    if (NewHandle != NULL) {
      gBS->UnloadImage (NewHandle);
    }
    FreePool (NewCmdLine);
    return (Status);
  }

  Status = gBS->OpenProtocol (
                  NewHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    if (LoadedImage->ImageCodeType != EfiLoaderCode) {
      Print(L"The image is not an application.\n");
      goto UnloadImage;
    }

    if (NewCmdLine != NULL) {
      LoadedImage->LoadOptionsSize = (UINT32)StrSize (NewCmdLine);
      LoadedImage->LoadOptions     = (VOID *)NewCmdLine;
    }

    ZeroMem (&ShellParamsProtocol, sizeof (EFI_SHELL_PARAMETERS_PROTOCOL));
    Status = ParseCommandLineToArgs (NewCmdLine, TRUE, &(ShellParamsProtocol.Argv), &(ShellParamsProtocol.Argc));
    if (EFI_ERROR (Status)) {
      goto UnloadImage;
    }
    Status = gBS->InstallProtocolInterface (&NewHandle, &gEfiShellParametersProtocolGuid, EFI_NATIVE_INTERFACE, &ShellParamsProtocol);

#ifdef SHELL_1_SUPPORT
    ZeroMem (&ShellInterface, sizeof (EFI_SHELL_INTERFACE));
    ZeroMem (&ShellEnv2, sizeof (EFI_SHELL_ENVIRONMENT2));
    ShellEnv2.SESGuid = gEfiShellEnvironment2ExtGuid;
    ShellEnv2.MajorVersion = EFI_SHELL_MAJOR_VER;
    ShellEnv2.MinorVersion = EFI_SHELL_MINOR_VER;
    Status = ParseCommandLineToArgs (NewCmdLine, TRUE, &(ShellInterface.Argv), &(ShellInterface.Argc));
    if (EFI_ERROR (Status)) {
      goto UnloadImage;
    }
    Status = gBS->InstallProtocolInterface (&NewHandle, &gEfiShellInterfaceGuid, EFI_NATIVE_INTERFACE, &ShellInterface);
    Status = gBS->InstallProtocolInterface (&NewHandle, &gEfiShellEnvironment2Guid, EFI_NATIVE_INTERFACE, &ShellEnv2);
#endif

    if (!EFI_ERROR (Status)) {
      StartStatus = gBS->StartImage (
                           NewHandle,
                           0,
                           NULL
                           );
      if (StartImageStatus != NULL) {
        *StartImageStatus = StartStatus;
      }

      Status = gBS->UninstallProtocolInterface (
                             NewHandle,
                             &gEfiShellParametersProtocolGuid,
                             &ShellParamsProtocol
                             );
#ifdef SHELL_1_SUPPORT
      Status = gBS->UninstallProtocolInterface (
                             NewHandle,
                             &gEfiShellInterfaceGuid,
                             &ShellInterface
                             );
      Status = gBS->UninstallProtocolInterface (
                             NewHandle,
                             &gEfiShellEnvironment2Guid,
                             &ShellEnv2
                             );
#endif
      goto FreeAlloc;
    }

UnloadImage:
    gBS->UnloadImage (NewHandle);

FreeAlloc:
    if (ShellParamsProtocol.Argv != NULL) {
      for (UINTN i = 0; i < ShellParamsProtocol.Argc; i++) {
        FreePool (ShellParamsProtocol.Argv[i]);
      }
      FreePool (ShellParamsProtocol.Argv);
    }
#ifdef SHELL_1_SUPPORT
    if (ShellInterface.Argv != NULL) {
      for (UINTN i = 0; i < ShellInterface.Argc; i++) {
        FreePool (ShellInterface.Argv[i]);
      }
      FreePool (ShellInterface.Argv);
    }
#endif
  }
  FreePool (NewCmdLine);
  return Status;
}

typedef struct {
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    NewSimpleTextOut;
  EFI_HANDLE                         NewHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *OrgSimpleTextOut;
  EFI_HANDLE                         OrgHandle;
} NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

EFI_STATUS
EFIAPI
NewSimpleTextOutReset (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          ExtendedVerification
  )
{
  return ((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut->Reset(((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut, ExtendedVerification);
}

EFI_STATUS
EFIAPI
NewSimpleTextOutTestString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *String
  )
{
  return ((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut->TestString(((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut, String);
}

EFI_STATUS
EFIAPI
NewSimpleTextOutQueryMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber,
  OUT UINTN                            *Columns,
  OUT UINTN                            *Rows
  )
{
  return ((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut->QueryMode(
                              ((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut,
                              ModeNumber,
                              Columns,
                              Rows
                              );
}

EFI_STATUS
EFIAPI
NewSimpleTextOutSetMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber
  )
{
  return ((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut->SetMode(((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut, ModeNumber);
}

EFI_STATUS
EFIAPI
NewSimpleTextOutSetAttribute (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Attribute
  )
{
  return ((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut->SetAttribute(((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut, Attribute);
}

EFI_STATUS
EFIAPI
NewSimpleTextOutClearScreen (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This
  )
{
  return ((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut->ClearScreen(((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut);
}

EFI_STATUS
EFIAPI
NewSimpleTextOutSetCursorPosition (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Column,
  IN  UINTN                            Row
  )
{
  return ((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut->SetCursorPosition(((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut, Column, Row);
}

EFI_STATUS
EFIAPI
NewSimpleTextOutEnableCursor (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          Visible
  )
{
  return ((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut->EnableCursor(((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut, Visible);
}

EFI_STATUS
EFIAPI
NewSimpleTextOutOutputString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *String
  )
{
  gBS->Stall(5000000);
  return ((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut->OutputString(((NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)This)->OrgSimpleTextOut, String);
}

NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *InstallNewStdOut()
{
  EFI_STATUS Status;
  NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *NewStdOut = AllocateZeroPool(sizeof(NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL));
  NewStdOut->OrgHandle = gST->ConsoleOutHandle;
  NewStdOut->OrgSimpleTextOut = gST->ConOut;
  NewStdOut->NewHandle = gImageHandle;
  NewStdOut->NewSimpleTextOut.Reset             = NewSimpleTextOutReset;
  NewStdOut->NewSimpleTextOut.TestString        = NewSimpleTextOutTestString;
  NewStdOut->NewSimpleTextOut.QueryMode         = NewSimpleTextOutQueryMode;
  NewStdOut->NewSimpleTextOut.SetMode           = NewSimpleTextOutSetMode;
  NewStdOut->NewSimpleTextOut.SetAttribute      = NewSimpleTextOutSetAttribute;
  NewStdOut->NewSimpleTextOut.ClearScreen       = NewSimpleTextOutClearScreen;
  NewStdOut->NewSimpleTextOut.SetCursorPosition = NewSimpleTextOutSetCursorPosition;
  NewStdOut->NewSimpleTextOut.EnableCursor      = NewSimpleTextOutEnableCursor;
  NewStdOut->NewSimpleTextOut.OutputString      = NewSimpleTextOutOutputString;
  NewStdOut->NewSimpleTextOut.Mode              = AllocateCopyPool(sizeof(EFI_SIMPLE_TEXT_OUTPUT_MODE), NewStdOut->OrgSimpleTextOut->Mode);

  Status = gBS->InstallProtocolInterface(
                  &(NewStdOut->NewHandle),
                  &gEfiSimpleTextOutProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &(NewStdOut->NewSimpleTextOut)
                  );

  if (!EFI_ERROR (Status)) {
    gST->ConsoleOutHandle = NewStdOut->NewHandle;
    gST->ConOut = &NewStdOut->NewSimpleTextOut;
    gST->Hdr.CRC32 = 0;
    gBS->CalculateCrc32 (
          (UINT8 *)&gST->Hdr,
          gST->Hdr.HeaderSize,
          &gST->Hdr.CRC32
         );
    return NewStdOut;
  } else {
    if (NewStdOut->NewSimpleTextOut.Mode != NULL) {
      FreePool(NewStdOut->NewSimpleTextOut.Mode);
      NewStdOut->NewSimpleTextOut.Mode = NULL;
    }
    if (NewStdOut != NULL) {
      FreePool(NewStdOut);
      NewStdOut = NULL;
    }
    return NULL;
  }
}

EFI_STATUS UninstallNewStdOut(NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *NewStdOut)
{
  EFI_STATUS Status;
  gST->ConsoleOutHandle = NewStdOut->OrgHandle;
  gST->ConOut = NewStdOut->OrgSimpleTextOut;
  gST->Hdr.CRC32 = 0;
  gBS->CalculateCrc32 (
        (UINT8 *)&gST->Hdr,
        gST->Hdr.HeaderSize,
        &gST->Hdr.CRC32
        );
  Status = gBS->UninstallProtocolInterface(NewStdOut->NewHandle, &gEfiSimpleTextOutProtocolGuid, (VOID *)&NewStdOut->NewSimpleTextOut);
  FreePool(NewStdOut->NewSimpleTextOut.Mode);
  FreePool(NewStdOut);
  return Status;
}

EFI_STATUS
MyCallerEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  // UINTN MyNum = 0;
  // EFI_HANDLE  *MyFsHandle = NULL;
  // EFI_USB_IO_PROTOCOL *UsbIo;
  // Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &MyNum, &MyFsHandle);
  // for (UINTN i = 0; i < MyNum; i++) {
  //   Status = gBS->HandleProtocol(MyFsHandle[i], &gEfiUsbIoProtocolGuid, &UsbIo);
  //   if (Status) {
  //     continue;
  //   }
  //   EFI_USB_DEVICE_DESCRIPTOR   DevDsc;
  //   Status = UsbIo->UsbGetDeviceDescriptor(UsbIo, &DevDsc);
  //   if (Status || DevDsc.IdVendor != 0x781 || DevDsc.IdProduct != 0x5591) {
  //     continue;
  //   }
  //   EFI_USB_INTERFACE_DESCRIPTOR InfDsc;
  //   Status = UsbIo->UsbGetInterfaceDescriptor(UsbIo, &InfDsc);
  //   if (Status || InfDsc.InterfaceClass != USB_MASS_STORE_CLASS || InfDsc.InterfaceSubClass != USB_MASS_STORE_SCSI) {
  //     continue;
  //   }
  //   EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs0;
  //   EFI_FILE_PROTOCOL *root;
  //   EFI_FILE_PROTOCOL *file;
  //   Status = gBS->HandleProtocol(MyFsHandle[i], &gEfiSimpleFileSystemProtocolGuid, &Fs0);
  //   AsciiPrint("gBS->HandleProtocol status :%r\n", Status);
  //   Status = Fs0->OpenVolume(Fs0, &root);
  //   AsciiPrint("Fs0->OpenVolume status :%r\n", Status);
  //   Status = root->Open(root, &file, L"\\aaa.txt", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
  //   AsciiPrint("root->Open status :%r\n", Status);
  //   UINTN BufferSize = 8;
  //   CHAR8 Buffer[9];
  //   UINT64 Position;
  //   Status = file->Read(file, &BufferSize, Buffer);
  //   file->GetPosition(file, &Position);
  //   Buffer[8]=0;
  //   AsciiPrint("file->Read status :%r, size:%d, position:%d\ncontent:%a\n", Status, BufferSize, Position, Buffer);
  //   BufferSize = 5;
  //   Buffer[0] = '1';
  //   file->Write(file, &BufferSize, Buffer);
  //   file->Close(file);
  //   root->Close(root);
    
    // EFI_DEVICE_PATH_PROTOCOL  *MyPath;
    // MyPath = FileDevicePath(MyFsHandle[i], L"\\aaa.txt");
    // EFI_FILE_HANDLE confh;
    // UINTN BufferSize = 0;
    // CHAR8 *Buffer;
    // Status = EfiOpenFileByDevicePath(&MyPath, &confh, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    // Status = confh->Read(confh, &BufferSize, Buffer);
    // AsciiPrint("read status :%r BufferSize:%d\n", Status, BufferSize);
    // if (Status != EFI_BUFFER_TOO_SMALL) {
    //   return 0;
    // }
    // Buffer = AllocatePool(BufferSize);
    // Status = confh->Read(confh, &BufferSize, Buffer);
    // AsciiPrint("%a\n", Buffer);
    // UINT64 Position;
    // Status = confh->GetPosition(confh, &Position);
    // AsciiPrint("P:%d\n", Position);
  
    // Status = EfiBinaryExecute(MyPath, L"MyPrint.efi asdf qwer zxcv", NULL, 0, NULL);
    // FreePool(MyPath);
    // FreePool(Buffer);
    // confh->Close(confh);
  // }
  // if(MyFsHandle) {
  //   FreePool(MyFsHandle);
  // }


  NEW_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *NewStd = InstallNewStdOut();
  Status = EfiBinaryExecute(NULL, L"MyPrint.efi", EfiBinary, sizeof(EfiBinary), NULL);
  if (NewStd != NULL) {
    UninstallNewStdOut(NewStd);
  }

  // gBS->Stall(10000000);
  
  return Status;
}