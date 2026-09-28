在获取源码之前，都需要把你开发电脑上的相应的 public key发送给我们网管进行添加，从而得到获取源码的权限。本地电脑获取 public key的方法如下： 

如果本机还没有 ssh public key,可以通过以下方法生成, 打开一个终端输入下列命令: 

```
jiangwen@uws:~/work/x1000/zk_external_git_test$ ssh-keygen
```


//输入 ssh-keygen 后,使用 
默认配置,一路回车直到完成即可 

```
jiangwen@uws:~/work/x1000/zk_external_git_test$ cat ~/.ssh/id_rsa.pub
```


把这个的内容发送给我们的网管。 
不同的芯片平台获取源码的方式如下 ： 
Linux平台获取源码的方法  doc/FAE文档/Linux平台源码获取.pdf
Rtos平台获取源码的方法    doc/FAE文档/FreeRTOS源码获取.pdf
代码同步注意事项：
比如客户同步代码的地址如下： 

```
./repo init -u ssh://sz_halley2@119.136.25.25:29418/mirror/linux/manifest
```



如若报错可使用http同步方式：

```
./repo init -u http://sz_halley2@119.136.25.25:8089/mirror/linux/manifest
```


对于外部客户来说，这里的帐号不用修改，直接使用sz_halley2就可以。 


 可能遇到的问题：

Bad owner or permissions on .ssh/config 解决方法如下：

当为本机配一个固定用户名远程登陆某主机时，配置了一个config文件，但是在执行ssh免密码登陆时报如下的错误：

Bad owner or permissinos on .ssh/config的解决。

解决方法如下：

sudo chmod 600 .ssh/config





