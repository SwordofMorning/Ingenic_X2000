

# RSA使用及说明文档

## 1.配置

打开IConfigTool并且选择对应板级配置，此处以**x2580**为例进行说明。

<img src="img/1.png"  />

进入 [模块化驱动] 界面，点击进入 **[x2580驱动列表]** 并且勾选配置如下配置：

<img src="img/2.png"  />



## 2.编译、烧录

**编译、烧录，根据各种型号设备，请参考 <u>/doc/开发使⽤说明/1_Linux工程编译说明.pdf</u> 工程编译流程章节以及 <u>/doc/开发使⽤说明/2_Linux工程烧录介绍.pdf</u> 文档。**



## 3.使用

**1.在已烧录的情况下直接按回车进入命令行，输入 "lsmod" 查看当前是否已安装 rsa 驱动，且输入 "ls /dev/jz_rsa" 可查看到该设备节点**

```c
lsmod                   //查看驱动是否安装

ls /dev/jz_rsa          //是否存在设备节点文件
```

<img src="img/3.png"  />

**2.输入 cmd_rsa 查看该命令具体用法**

<img src="img/4.png" />

`若提示没有该命令请查看 libhardware2片上外设接口/命令 中rsa是否已勾选`

**3.命令使用参考**

```c
  # 生成密钥长度为1024位, 格式为PEM的私钥文件
openssl genrsa -out pri.pem 1024
  # 将PEM格式文件转换为DER格式文件
openssl rsa -in pri.pem -outform DER -out pri.der

  # openssl使用私钥文件对文件1加密
  # 加密后的数据保存在文件1_out
openssl rsautl -pkcs -encrypt -inkey pri.pem -in 1 -out 1_out
  # openssl使用私钥文件对文件1_out解密
  # 解密后的数据保存在文件1_out_out
openssl rsautl -pkcs -decrypt -inkey pri.pem -in 1_out -out 1_out_out

  # cmd_rsa设置bits为1024位, 使用DER格式的私钥文件
  # 对文件1加密, 加密后的数据保存在文件1_out
cmd_rsa encrypt bits=1024 key=pri.der in=1
  # cmd_rsa设置bits为1024位, 使用DER格式的私钥文件
  # 对文件1_out解密, 解密后的数据保存在文件1_out_out
cmd_rsa decrypt bits=1024 key=pri.der in=1_out
```

<img src="img/5.png"  />
