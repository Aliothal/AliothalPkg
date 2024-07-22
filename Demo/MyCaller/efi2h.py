import os
import time

efi_name = "\\MyPrint.efi"
struct_line = 32

efi_path = os.path.dirname(os.path.abspath(__file__)) + efi_name
header_path = os.path.dirname(os.path.abspath(__file__)) + '/EfiBinary.h'
data_bytes = ''
content0 = """#ifndef __EFI_BINARY_H__
#define __EFI_BINARY_H__

"""
declaration = """EFI_STATUS
EfiBinaryExecute (
  IN CONST EFI_DEVICE_PATH_PROTOCOL   *DevicePath OPTIONAL,
  IN CONST CHAR16                     *CommandLine,
  VOID                                *SourceBuffer OPTIONAL,
  UINTN                               SourceSize OPTIONAL,
  OUT EFI_STATUS                      *StartImageStatus OPTIONAL
  );

"""
# repr把""变成了''
# content1 = f"""CONST CHAR16  *EfiName = L{repr(efi_name)};

# """
content1 = """CONST CHAR16  *EfiName = L"{}";

""".format(efi_name)
content2 = """UINT8     EfiBinary[] = {
"""
content3 = """};

#endif
"""

if __name__ == "__main__":
    # if os.path.exists(header_path):
    #     exit()
    with open(efi_path, 'rb') as f:
        buffer = f.read()
        # t = time.time()
        for index,byte in enumerate(buffer):
            data_bytes += (' 0x'+ hex(byte)[2:].zfill(2) + ',')
            # data_bytes += ' ' + '0x%02x' % (buffer[2]) + ','
            if (index + 1) % struct_line == 0:
                data_bytes += ('\n')
        # t = time.time() - t
        # print(data_bytes)
            
    with open(header_path, 'w') as f:
        f.write(content0)
        f.write(declaration)
        f.write(content1)
        f.write(content2)
        f.write(data_bytes)
        f.write(content3)
        