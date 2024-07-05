#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/Usb2HostController.h>
#include <Protocol/UsbFunctionIo.h>
#include <Protocol/SimpleFileSystem.h>

EFI_STATUS
UsbRSEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                *UsbIoHandleList;
  UINTN                     UsbIoNumber;
  
  Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiUsbIoProtocolGuid, NULL, &UsbIoNumber, &UsbIoHandleList);
  for (UINTN i = 0; i < UsbIoNumber; i++) {
    EFI_USB_IO_PROTOCOL         *UsbIo;
    EFI_USB_DEVICE_DESCRIPTOR   UsbDeviceDescriptor;
    Status = gBS->HandleProtocol(UsbIoHandleList[i], &gEfiUsbIoProtocolGuid, (VOID **)&UsbIo);
    Status = UsbIo->UsbGetDeviceDescriptor(UsbIo, &UsbDeviceDescriptor);
    if (UsbDeviceDescriptor.IdVendor == 0x6666 && UsbDeviceDescriptor.IdProduct == 0x8888) {
      
      EFI_USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
      Status = UsbIo->UsbGetInterfaceDescriptor(UsbIo, &InterfaceDescriptor);

      EFI_USB_ENDPOINT_DESCRIPTOR EndpointDescriptor;
      for (UINT8 i = 0; i < InterfaceDescriptor.NumEndpoints; i++) {
        Status = UsbIo->UsbGetEndpointDescriptor(UsbIo, i, &EndpointDescriptor);
        if ((EndpointDescriptor.EndpointAddress & 0x80 == EfiUsbDataIn) &&
            (EndpointDescriptor.Attributes & USB_ENDPOINT_TYPE_MASK == USB_ENDPOINT_INTERRUPT)) {
          break;
        }
      }

      // Data[0] 0x11 0x14 0x15 0x16 0x17 0x32 0x33 0x34  Data[1] TRUE FALSE
      // 
      UINT8 Data[2];
      UINTN DataLength = 2;
      UINT32 UsbStatus;
      // Status = UsbIo->UsbBulkTransfer(UsbIo, 2, Data, &DataLength, 0, &UsbStatus);
      Status = UsbIo->UsbSyncInterruptTransfer(UsbIo, EndpointDescriptor.EndpointAddress, Data, &DataLength, 1000, &UsbStatus);
      AsciiPrint("UsbBulkTransfer Status %r OutStatus %r DataLength 0x%x\n", Status, UsbStatus, DataLength);
      
      break;
    }
  }

  FreePool(UsbIoHandleList);
  return Status;
}