#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/SerialIo.h>

EFI_HANDLE                          *gSerialHandleBuffer = NULL;
UINTN                               gHandleCount = 0;
EFI_SERIAL_IO_PROTOCOL              *gSerialIo;
UINTN             BufferSize = 5;
CHAR8             *Buffer = "NONE";

VOID ShowAllSerialPortInfo()
{
  EFI_STATUS                         Status;
  UINTN                              HandleIndex = 0;
	
	Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSerialIoProtocolGuid, NULL, &gHandleCount, &gSerialHandleBuffer);
  Print(L"Handle Buffer Status:%r\n\r", Status);

  Status = gBS->LocateProtocol(&gEfiSerialIoProtocolGuid, NULL, (VOID**)&gSerialIo);
  Print(L"Locate Protocol Status:%r\n\r", Status);
	if (EFI_ERROR(Status))	return;
	Print(L"All SerialPort Count: %d\n", gHandleCount);
	for (HandleIndex = 0; HandleIndex < gHandleCount; HandleIndex++) {
		Status = gBS->HandleProtocol(gSerialHandleBuffer[HandleIndex], &gEfiSerialIoProtocolGuid, (VOID**)&gSerialIo);
		if (EFI_ERROR(Status))
      continue;
		else {
			Print(L"Index:%d, Handle=0x%x\n", HandleIndex, gSerialHandleBuffer[HandleIndex]);
			Print(L"--ControlMask=0x%x\n", gSerialIo->Mode->ControlMask);
			Print(L"--Timeout=%d\n", gSerialIo->Mode->Timeout);
			Print(L"--BaudRate=%d\n", gSerialIo->Mode->BaudRate);
			Print(L"--ReceiveFifoDepth=%d\n", gSerialIo->Mode->ReceiveFifoDepth);
			Print(L"--DataBits=%d\n", gSerialIo->Mode->DataBits);
			Print(L"--Parity=%d\n", gSerialIo->Mode->Parity);
			Print(L"--StopBits=%d\n", gSerialIo->Mode->StopBits);
		}
	}
}

VOID ShowAllProtoclInfo()
{
  EFI_STATUS        Status;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY       *ProtocolInfoList;
  UINTN             Count;
  UINTN             Index;
  Status = gBS->OpenProtocolInformation(gSerialHandleBuffer[0], &gEfiSerialIoProtocolGuid, &ProtocolInfoList, &Count);
  Print(L"Totol Count:%d\n", Count);
  for (Index = 0; Index < Count; Index++) {
    Print(L"--ProtocolInfo %d\n", Index);
    Print(L"----AgentHandle:0x%x\n", ProtocolInfoList[Index].AgentHandle);
    Print(L"----ControllerHandle:0x%x\n", ProtocolInfoList[Index].ControllerHandle);
    Print(L"----Attributes:0x%x\n", ProtocolInfoList[Index].Attributes);
    Print(L"----OpenCount:0x%x\n", ProtocolInfoList[Index].OpenCount);
  }
}

VOID
GetCommand (
  IN  EFI_EVENT                Event,
  IN  VOID                     *Context
  )
{
  EFI_STATUS        Status;
  UINT32			      Control;


  Print(L"Read start\n");
  Status = gSerialIo->GetControl(gSerialIo, &Control);
  Print(L"Getcontrol Status:%x, Control:%x\n", Status, Control);
  // if ((Control & EFI_SERIAL_INPUT_BUFFER_EMPTY) == EFI_SERIAL_INPUT_BUFFER_EMPTY) {
  //   Print(L"Input Buffer empty\n");
  //   return;
  // }
  Status = gSerialIo->Read(gSerialIo, &BufferSize, Buffer);
  // if (Buffer[0] == 'q') {
  //   gBS->SetTimer(Event, TimerCancel, 0);
  //   AsciiPrint("exit event\n");
  //   gBS->CloseEvent(Event);
  // }
  AsciiPrint("read Buffer Status:%x, data:%s\n", Status, Buffer);
  return;
}


VOID CreateMyevent()
{
  EFI_EVENT         Myevent;
  gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, GetCommand, NULL, &Myevent);
  gBS->SetTimer(Myevent, TimerPeriodic, 10000000);
}

VOID CloseSerialPort()
{
  EFI_STATUS        Status;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY       *ProtocolInfoList;
  UINTN             Count;

  Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSerialIoProtocolGuid, NULL, &gHandleCount, &gSerialHandleBuffer);
  Status = gBS->OpenProtocolInformation(gSerialHandleBuffer[0], &gEfiSerialIoProtocolGuid, &ProtocolInfoList, &Count);
    Print(L"Totol Count:%d\n", Count);
  for (UINTN Index = 0; Index < Count; Index++) {
    Print(L"--ProtocolInfo %d\n", Index);
    Print(L"----AgentHandle:0x%x\n", ProtocolInfoList[Index].AgentHandle);
    Print(L"----ControllerHandle:0x%x\n", ProtocolInfoList[Index].ControllerHandle);
    Print(L"----Attributes:0x%x\n", ProtocolInfoList[Index].Attributes);
    Print(L"----OpenCount:0x%x\n", ProtocolInfoList[Index].OpenCount);
  }
}

EFI_STATUS
SerialPortEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS        Status;
  ShowAllSerialPortInfo();
  // ShowAllProtoclInfo();

  if (gSerialHandleBuffer != NULL) {
    gBS->FreePool(gSerialHandleBuffer);
  }
  return EFI_SUCCESS;
}