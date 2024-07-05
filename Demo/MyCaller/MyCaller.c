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
#include "EfiBinary.h"

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
  }
  FreePool (NewCmdLine);
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
  // EFI_DEVICE_PATH_PROTOCOL  *MyPath;
  // gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &MyNum, &MyFsHandle);
  // for (UINTN i = 0; i < MyNum; i++) {
  //   // Determine which U disk by USB Io
  //   // todo
  //   MyPath = FileDevicePath(MyFsHandle[i], EfiName);
  //   Status = MyExecute(MyPath, L"MyPrint.efi asdf qwer zxcv", NULL, 0, NULL);
  // }
  // if(MyFsHandle) {
  //   FreePool(MyFsHandle);
  // }

  Status = EfiBinaryExecute(NULL, L"SceEfi64.efi /i /ms \"Memory Frequency\" /lang en-US /qv 0x1D", EfiBinary, sizeof(EfiBinary), NULL);

  return Status;
}