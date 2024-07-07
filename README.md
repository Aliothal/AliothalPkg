# 准备

-   搭建edk2环境和最好加上edk2-libc (一般用的最新edk2版本)

# 模块

## Lvgl

目前移植的是v9.1  
已经porting了显示,键鼠,定时器;但不保证完全管用,文件系统直接用的libc,尝试了一部分OK;  

### 关于Lvgl

-   [Lvgl](https://github.com/lvgl/lvgl)
-   [中文文档](https://lvgl.100ask.net/master/index.html)
-   [可视化工具eez studio](https://github.com/eez-open/studio)

lvgl对于事件,状态支持有限,有些功能仅依靠官方代码不能实现  
lvgl theme用的是simple,有点简陋;默认是default,不好看

#### Lvgl/NetAssist
目前以实现udp4大部分功能  
为什么要用两个udp protocol;因为UEFI的udp只有本机ip和子网掩码为0时才能接受广播(不好评价)  
正常来说需要先有ip,才能通讯;但是UEFI未通过ifconfig设置ip的时候,也能通讯;设置成相同ip甚至能拦截发向这个ip的包(试过UDP4)  
且只有真正设置了ip,才能切换config的ip,否则只能使用第一次config的ip

#### 网络工具
-   [NetAssist](https://www.cmsoft.cn/resource/102.html)
-   [Wireshark](https://www.wireshark.org/)  

。。。