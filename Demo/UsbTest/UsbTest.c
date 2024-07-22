#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

#include <Library/DevicePathLib.h>
#include <Protocol/Usb2HostController.h>
#include <Protocol/UsbFunctionIo.h>
#include <Protocol/SimpleFileSystem.h>

VOID ShowUsbIoInfo()
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
    AsciiPrint("IoProtocol %d Handle:0x%x\nDevDsc Length:%d DescriptorType:%d BcdUSB:%x DevClass:%d.%d DeviceProtocol:%d MaxPacketSize0:%d Vid:0x%x Pid:0x%x Bcd:0x%x %d\n",
    i, UsbIoHandleList[i], UsbDeviceDescriptor.Length, UsbDeviceDescriptor.DescriptorType, UsbDeviceDescriptor.BcdUSB,
    UsbDeviceDescriptor.DeviceClass, UsbDeviceDescriptor.DeviceSubClass, UsbDeviceDescriptor.DeviceProtocol, UsbDeviceDescriptor.MaxPacketSize0,
    UsbDeviceDescriptor.IdVendor, UsbDeviceDescriptor.IdProduct, UsbDeviceDescriptor.BcdDevice, UsbDeviceDescriptor.NumConfigurations);

    EFI_USB_CONFIG_DESCRIPTOR   UsbConfigDescriptor;
    Status = UsbIo->UsbGetConfigDescriptor(UsbIo, &UsbConfigDescriptor);
    AsciiPrint("CfgDsc Length:%d DescriptorType:%d TotalLength:%d NumInterfaces:%d ConfigurationValue:%d Configuration:%d Attributes:0x%x MaxPower:%d\n",
    UsbConfigDescriptor.Length, UsbConfigDescriptor.DescriptorType, UsbConfigDescriptor.TotalLength,
    UsbConfigDescriptor.NumInterfaces, UsbConfigDescriptor.ConfigurationValue, UsbConfigDescriptor.Configuration,
    UsbConfigDescriptor.Attributes, UsbConfigDescriptor.MaxPower);

    UINT16                      *LangIDTable;
    UINT16                      TableSize;
    Status = UsbIo->UsbGetSupportedLanguages(UsbIo, &LangIDTable, &TableSize);
    for (UINT16 i = 0; i < TableSize; i++) {
      CHAR16    *ManufacturerStr;
      CHAR16    *ProductStr;
      CHAR16    *SerialNumberStr;
      Status = UsbIo->UsbGetStringDescriptor(UsbIo, LangIDTable[i], UsbDeviceDescriptor.StrManufacturer, &ManufacturerStr);
      Status = UsbIo->UsbGetStringDescriptor(UsbIo, LangIDTable[i], UsbDeviceDescriptor.StrProduct, &ProductStr);
      Status = UsbIo->UsbGetStringDescriptor(UsbIo, LangIDTable[i], UsbDeviceDescriptor.StrSerialNumber, &SerialNumberStr);
      Print(L"LangId 0x%x, ManufacturerStr:%s, ProductStr:%s, SerialNumberStr:%s\n", LangIDTable[i], ManufacturerStr, ProductStr, SerialNumberStr);
      FreePool(ManufacturerStr);
      FreePool(ProductStr);
      FreePool(SerialNumberStr);
    }

    EFI_USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    Status = UsbIo->UsbGetInterfaceDescriptor(UsbIo, &InterfaceDescriptor);
    AsciiPrint("Itf Dsc Length:%d DescriptorType:%d InterfaceNumber:%d AlternateSetting:%d NumEndpoints:%d InterfaceClass:%d.%d InterfaceProtocol:%d Interface:%d\n",
    InterfaceDescriptor.Length, InterfaceDescriptor.DescriptorType, InterfaceDescriptor.InterfaceNumber,
    InterfaceDescriptor.AlternateSetting, InterfaceDescriptor.NumEndpoints, InterfaceDescriptor.InterfaceClass, InterfaceDescriptor.InterfaceSubClass,
    InterfaceDescriptor.InterfaceProtocol, InterfaceDescriptor.Interface);

    for (UINT8 i = 0; i < InterfaceDescriptor.NumEndpoints; i++) {
      EFI_USB_ENDPOINT_DESCRIPTOR EndpointDescriptor;
      Status = UsbIo->UsbGetEndpointDescriptor(UsbIo, i, &EndpointDescriptor);
      AsciiPrint("EndPIndex %d, Length:%d DescriptorType:%d EndpointAddress:0x%x Attributes:%d MaxPacketSize:%d Interval:%d\n",
      i, EndpointDescriptor.Length, EndpointDescriptor.DescriptorType, EndpointDescriptor.EndpointAddress,
      EndpointDescriptor.Attributes, EndpointDescriptor.MaxPacketSize, EndpointDescriptor.Interval);
    }
    AsciiPrint("\n");
  }

  FreePool(UsbIoHandleList);
}

VOID ShowUsbFnIoInfo()
{
  EFI_STATUS                  Status;
  UINTN                       UsbFnIoNumber;
  EFI_HANDLE                  *UsbFnIoHandleList;

  Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiUsbFunctionIoProtocolGuid, NULL, &UsbFnIoNumber, &UsbFnIoHandleList);
  AsciiPrint("Status:%r, UsbFnIoNumber:%d\n", Status, UsbFnIoNumber);
  for (UINTN i = 0; i < UsbFnIoNumber; i++) {
    EFI_USBFN_IO_PROTOCOL     *UsbFnIo;
    Status = gBS->HandleProtocol(UsbFnIoHandleList[i], &gEfiUsbFunctionIoProtocolGuid, (VOID **)&UsbFnIo);
    EFI_USBFN_PORT_TYPE       PortType;
    Status = UsbFnIo->DetectPort(UsbFnIo, &PortType);
    AsciiPrint("UsbFnIo %d, Revision %d, PortType:%d\n", i, UsbFnIo->Revision, PortType);
    UINTN                     BufferSize = 40;
    CHAR16                    *SerialNumber;
    CHAR16                    *ManufacturerName;
    CHAR16                    *ProductName;
    Status = UsbFnIo->GetDeviceInfo(UsbFnIo, EfiUsbDeviceInfoSerialNumber, &BufferSize, SerialNumber);
    Status = UsbFnIo->GetDeviceInfo(UsbFnIo, EfiUsbDeviceInfoManufacturerName, &BufferSize, ManufacturerName);
    Status = UsbFnIo->GetDeviceInfo(UsbFnIo, EfiUsbDeviceInfoProductName, &BufferSize, ProductName);
    Print(L"SerialNumber:%s, ManufacturerName:%s, ProductName:%s\n", SerialNumber, ManufacturerName, ProductName);
    UINT16                    Vid, Pid;
    UINTN                     MaxTransSize;
    Status = UsbFnIo->GetVendorIdProductId(UsbFnIo, &Vid, &Pid);
    Status = UsbFnIo->GetMaxTransferSize(UsbFnIo, &MaxTransSize);
    AsciiPrint("Vid:0x%x, Pid:0x%x, MaxTransSize:%d\n", Vid, Pid, MaxTransSize);
  }
  FreePool(UsbFnIoHandleList);
}

VOID ShowFileSystemInfo()
{
  EFI_STATUS                Status;
  UINTN                     FsNumber;
  EFI_HANDLE                *FsHandleList;

  Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &FsNumber, &FsHandleList);
  for (UINTN i = 0; i < FsNumber; i++) {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *FsProtocol;
    EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
    CHAR16                            *DevicePathString;
    Status = gBS->HandleProtocol(FsHandleList[i], &gEfiSimpleFileSystemProtocolGuid, (VOID **)&FsProtocol);
    DevicePath = DevicePathFromHandle(FsHandleList[i]);
    DevicePathString = ConvertDevicePathToText (DevicePath, TRUE, FALSE);
    AsciiPrint("FsHandle :0x%x, DevPath :%s\n", FsHandleList[i], DevicePathString);
  }
  FreePool(FsHandleList);
}

EFI_STATUS
UsbTestEntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status = EFI_SUCCESS;
  EFI_USB2_HC_PROTOCOL      *Usb2Hc = NULL;
  UINT8                     MaxSpeed = 0;
  UINT8                     PortNumber = 0;
  UINT8                     Is64BitCapable = 0;
  EFI_USB_HC_STATE          HcState;

  Status = gBS->LocateProtocol(&gEfiUsb2HcProtocolGuid, NULL, (VOID **)&Usb2Hc);
  Status = Usb2Hc->GetCapability(Usb2Hc, &MaxSpeed, &PortNumber, &Is64BitCapable);
  Status = Usb2Hc->GetState(Usb2Hc, &HcState);
  AsciiPrint("Usb2Hc Version:%d.%d\n", Usb2Hc->MajorRevision, Usb2Hc->MinorRevision);
  AsciiPrint("Usb2Hc MaxSpeed:%d, Port:%d, Is64BitCapable:%d, State:%d\n\n", MaxSpeed, PortNumber, Is64BitCapable, HcState);

  ShowUsbIoInfo();
  ShowUsbFnIoInfo();
  ShowFileSystemInfo();
  
  return EFI_SUCCESS;
}