

# 一 开发环境

请最好使用Ubuntu,Deepin 64位系统。

> **推荐在 X86_64 Ubuntu 16.04 或mint-20.3系统环境下进行开发, 不要使用X86_64 Ubuntu 20.04以上的版本,以下操作以mint-20.3系统为例.** 

# 二 授权开通

下载我们的开发资料需要我们开通授权以后才能下载，开通步骤：

## 2.1 生成SSH KEY

```shell
ingenic@ingenic:~$ ssh-keygen 
```

输入ssh-keygen后，使用默认配置，一路回车直到完成即可

```shell
Generating public/private rsa key pair.

Enter file in which to save the key (/home/chenwy/.ssh/id_rsa): 

Created directory '/home/chenwy/.ssh'.

Enter passphrase (empty for no passphrase): 

Enter same passphrase again: 

Your identification has been saved in /home/chenwy/.ssh/id_rsa.

Your public key has been saved in /home/chenwy/.ssh/id_rsa.pub.

The key fingerprint is:

0a:be:c9:55:e9:ec:59:57:ce:d8:14:e7:83:b4:a9:33 chenwy@ubuntu

The key's randomart image is:

+--[ RSA 2048]----+

|         |

|         |

|       .. .|

|     .  . ++ |

|   .  S   +o..|

|  . . =   .B  .|

|   . o o .Eo +  |

|  . + . o .o   |

|   +  o     |

+-----------------+

 
```

##  2.2 授权开通

将生成好的**id_rsa.pub**发送给我们,其所在位置：

```c
ingenic@ingenic:~$ cd ~/.ssh
ingenic@ingenic:~/.ssh$ cat id_rsa.pub
```

# 三 SSH本地配置

## 3.1 配置用户config

```c
ingenic@ingenic:~/.ssh$ cd ~/.ssh
ingenic@ingenic:~/.ssh$ vim config
ingenic@ingenic:~/.ssh$ sudo apt install vim       //系统未安装vim先安装vim
ingenic@ingenic:~/.ssh$ vim config 
```

在config中加入以下信息：(如果当前目录下没有config，则新建一个)

```c
Host *

    KexAlgorithms +diffie-hellman-group1-sha1
```

## 3.2 配置ssh_config

```c
ingenic@ingenic:~$ sudo vim /etc/ssh/ssh_config
```

找到如下一行，将注释去掉：

```c
#Ciphers aes128-cbc,aes192-cbc,aes256-cbc,aes128-ctr,aes192-ctr,aes256-ctr,3des-cbc,arcfour128,arcfour256,arcfour,blowfish-cbc,cast128-cbc
```

# 四 Git环境安装

## 4.1 安装Git及Gitk

```c
ingenic@ingenic:~$ sudo apt install git
ingenic@ingenic:~$ sudo apt install gitk
```

## 4.2  配置Git

请将双引号里的内容替换成你自己

```shell
ingenic@ingenic:~$git config --global user.email "you@example.com"
ingenic@ingenic:~$git config --global user.name "Your Name"
```

> **请等待我们开通权限后再进行下面的操作**

# 五 代码下载

## 5.1 repo工具下载

```c
ingenic@ingenic:~$ mkdir linux                                   //创建工程目录
ingenic@ingenic:~$ cd linux/
ingenic@ingenic:~/linux$ wget http://git.ingenic.com.cn:8082/bj/repo 
ingenic@ingenic:~/linux$ chmod +x repo
```

## 5.2 代码同步

```c
ingenic@ingenic:~/linux$./repo init -u ssh://sz_halley2@119.136.25.25:29418/mirror/linux/manifest
ingenic@ingenic:~/linux$./repo sync
```

> **请直接复制代码，不要修改sz_halley2的用户名**

ssh同步如若报错可使用http同步方式：

```c
ingenic@ingenic:~/linux$./repo init -u http://sz_halley2@119.136.25.25:8089/mirror/linux/manifest
```

可能遇到的问题,当为本机配一个固定用户名远程登录某主机时,配置了一个config文件,但是在执行ssh免密码登陆时报如下的错误：

```c
Bad owner or permissions on .ssh/config 
```

解决方法:

```c
sudo chmod 600 .ssh/config
```

## 5.3 编译依赖环境

```c
sudo apt install python2                        //编译需要python2.x的版本,python3.x的版本会报错
sudo ln -s /usr/bin/python2 /usr/bin/python

sudo apt-get install libc6-dev            
sudo apt-get install g++
sudo apt-get install u-boot-tools
sudo apt-get install libncurses5-dev
```

配置5.1的kernel需要添加环境变量

```c
sudo apt install gedit
gedit ~/.bashrc
```

添加工程目录的工具链

```c
export PATH=/home/ingenic/linux/tools/toolchains/mips-gcc720-glibc229/bin/:$PATH   // /home/ingenic/linux是前面自己创建的工程目录
```

