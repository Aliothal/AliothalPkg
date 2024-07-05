[Defines]
  PLATFORM_NAME                     = AliothalPkg
  PLATFORM_GUID                     = a938d727-775b-4736-b71e-b0d281c87f65
  PLATFORM_VERSION                  = 0.01
  DSC_SPECIFICATION                 = 0x00010006
  OUTPUT_DIRECTORY                  = Build/AliothalPkg
  SUPPORTED_ARCHITECTURES           = IA32|X64|MIPS64EL|AARCH64
  BUILD_TARGETS                     = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER                  = DEFAULT

#DEBUG Output Contorl
  DEFINE DEBUG_ENABLE_OUTPUT        = FALSE
  DEFINE DEBUG_PRINT_ERROR_LEVEL    = 0x80000040
  DEFINE DEBUG_PROPERTY_MASK        = 0x02

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|$(DEBUG_PROPERTY_MASK)
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|$(DEBUG_PRINT_ERROR_LEVEL)

[LibraryClasses]
  #
  # Entry Point Libraries
  #
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  ShellCEntryLib|ShellPkg/Library/UefiShellCEntryLib/UefiShellCEntryLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  #
  # Common Libraries
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  !if $(DEBUG_ENABLE_OUTPUT)
    DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
    DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  !else   ## DEBUG_ENABLE_OUTPUT
    DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  !endif  ## DEBUG_ENABLE_OUTPUT
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf  
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
  FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  BmpSupportLib|MdeModulePkg/Library/BaseBmpSupportLib/BaseBmpSupportLib.inf
  SmbusLib|MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  NetLib|NetworkPkg/Library/DxeNetLib/DxeNetLib.inf
  
  #CommandParameters
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  ShellCommandLib|ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
  HandleParsingLib|ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
  OrderedCollectionLib|MdePkg/Library/BaseOrderedCollectionRedBlackTreeLib/BaseOrderedCollectionRedBlackTreeLib.inf

  #lbdebug
  VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf

  #Time
  TimeBaseLib|EmbeddedPkg/Library/TimeBaseLib/TimeBaseLib.inf
  # TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf
  # LocalApicLib|UefiCpuPkg/Library/BaseXApicX2ApicLib/BaseXApicX2ApicLib.inf
  # TimerLib|UefiCpuPkg/Library/SecPeiDxeTimerLibUefiCpu/SecPeiDxeTimerLibUefiCpu.inf
  TimerLib|UefiCpuPkg/Library/CpuTimerLib/BaseCpuTimerLib.inf

  #Json
  Ucs2Utf8Lib|RedfishPkg/Library/BaseUcs2Utf8Lib/BaseUcs2Utf8Lib.inf
  RedfishCrtLib|RedfishPkg/PrivateLibrary/RedfishCrtLib/RedfishCrtLib.inf
  JsonLib|RedfishPkg/Library/JsonLib/JsonLib.inf

  #Crypt
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf

  #MyLib
  cJSONLib|AliothalPkg/Library/cJsonLib/cJSON.inf
  Udp4SocketLib|AliothalPkg/Library/Udp4SocketLib/Udp4SocketLib.inf
  
[Components]
  AliothalPkg/Test/Test.inf
  AliothalPkg/Demo/SerialPort/SerialPort.inf
  AliothalPkg/Demo/NetStackTest/NetStackTest.inf
  AliothalPkg/Demo/Udp4Receive/Udp4Receive.inf
  AliothalPkg/Demo/MemVddq/MemVddq.inf
  AliothalPkg/Demo/Tcp4Receive/Tcp4Receive.inf
  AliothalPkg/Demo/GraphicsOutputTest/GraphicsOutputTest.inf
  AliothalPkg/Demo/MultiProcessors/MultiProcessors.inf
  AliothalPkg/Demo/SpdInfo/SpdInfo.inf
  AliothalPkg/Demo/Tcp4Client/Tcp4Client.inf
  AliothalPkg/Demo/EchoTcp4/EchoTcp4.inf
  AliothalPkg/Demo/UsbTest/UsbTest.inf
  AliothalPkg/Demo/DumpBootLogo/DumpBootLogo.inf
  AliothalPkg/Demo/SimpleText/SimpleText.inf
  AliothalPkg/App/ShowGif/ShowGif.inf
  AliothalPkg/Demo/PerfMonitor/PerfMonitor.inf
  AliothalPkg/App/Func/Func.inf
  AliothalPkg/CppMain/CppMain.inf
  AliothalPkg/Demo/MyPrint/MyPrint.inf
  AliothalPkg/Demo/MyCaller/MyCaller.inf
  AliothalPkg/Demo/UsbRS/UsbRS.inf
  AliothalPkg/Template/Template.inf
  AliothalPkg/Demo/JsonTest/JsonTest.inf
  AliothalPkg/Demo/Udp4Socket/Udp4Socket.inf
  AliothalPkg/Lvgl/Lvgl.inf
  AliothalPkg/Demo/GetTable/GetTable.inf

!include StdLib/StdLib.inc