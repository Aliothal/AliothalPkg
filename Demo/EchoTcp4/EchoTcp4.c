/** @file
    A simple, basic, application showing how the Hello application could be
    built using the "Standard C Libraries" from StdLib.

    Robin's uefi framework application, 2020-3-20
**/
/**
 * 《UEFI编程实践》随书代码
 * 更多的UEFI探索，可以参考笔者的博客和专栏：
 * CSDN: https://blog.csdn.net/luobing4365
 * 知乎: https://www.zhihu.com/column/c_1233025362843209728
 * **/
//#include  <Uefi.h>
//#include  <Library/UefiLib.h>
//#include  <Library/ShellCEntryLib.h>
#include  <Uefi.h>
#include  <Library/UefiLib.h>
// #include  <Library/ShellCEntryLib.h>
#include  <Library/DebugLib.h>

#include <Library/BaseMemoryLib.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
// #include <errno.h>
// #include <netdb.h>
// #include <string.h>
#include <stdio.h>
// #include <unistd.h>
// #include <netinet/in.h>
// #include <sys/socket.h>
// #include <sys/endian.h>
// #include <stdlib.h>
// #include <wchar.h>
// #include <arpa/inet.h>

int
main (
  IN int Argc,
  IN char **Argv
  )
{
  // int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  // struct sockaddr_in addr;
  // addr.sin_family = AF_INET;
  // addr.sin_port = htons(54545);
  // addr.sin_addr.s_addr = inet_addr("192.168.1.101");
  // connect(fd, (struct sockaddr*)&addr, sizeof(addr));
  // char *buffer = "1234";
  // send(fd, buffer, 5, 0);
  // close(fd);

  double  a = 8/9.0;
  printf("%f\n",a);
  return 0;
}