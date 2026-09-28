# NFS文件系统挂载

NFS即网络文件系统，可以通过网络让不同机器，系统之间实现文件共享



## 1.服务器端配置

1.安装 NFS 软件包

>   sudo apt-get install nfs-kernel-serve   # 安装 NFS服务器端



2.添加 NFS 共享目录

>   sudo vim /etc/exports

若需要把 “/nfsroot” 目录设置为 NFS 共享目录，请在该文件末尾添加下面的一行

/nfsroot *(rw,sync,no_root_squash)    #表示允许任何网段 IP 的系统访问该 NFS 目录

![1](img/1.png)

`注意：需要挂载的目录必须存在`



3.启动 NFS 服务

>   sudo /etc/init.d/nfs-kernel-server start    或者
>
>   sudo /etc/init.d/nfs-kernel-server restart

`注意：在 NFS 服务已经启动的情况下，若修改了“/etc/exports” 文件，需要重启 NFS 服务，以刷新 NFS 的共享目录`



## 2.测试 NFS 服务器

>   sudo apt-get install nfs-common         # 安装 NFS客户端
>
>   sudo mkdir nfs
>
>   sudo mount -t nfs 194.169.3.136:/nfsroot /nfs

194.169.3.136为主机IP，/nfsroot为主机共享目录，/nfs 为创建的设备挂载目录，如果指令运行没有出错，则 NFS 挂载成功，在主机的 /nfs 目录下应该可以看到 /nfsroot 目录下的内容（可先在 nfsroot 目录下新建测试目录），如需卸载使用

>   sudo umount /nfs



## 3.开发板挂载NFS

1.将开发板联网，使其与服务器端处于同一网段

将开发板连接电脑的网口 或者 将开发板接在电脑所连的路由器或交换机上

2.确定开发板与主机服务器端可以 ping 通

>   ping 194.169.3.136

3.添加NFS支持

进入buildroot/buildroot/

>   make menuconfig

将 Target packages -> FileSystem and flash utilities -> nfs-utils 选上

![2](img/2.png)

4.挂载

>   cd /tmp
>
>   mkdir nfs
>
>   mount -t nfs 194.169.3.136:/nfsroot /tmp/nfs

194.169.3.136为服务器端IP，/nfsroot为服务器端提供的共享目录，/tmp/nfs是开发板挂载目录，若NFS挂载成功，在开发板的 /tmp/nfs 目录下应该可以看到主机端/nfsroot目录下的内容，同理卸载使用

>   umount /tmp/nfs