markdown 编辑工具之图片插入说明

markdown作为文字记录文本，对图片插入的考虑是相对弱一点的。对于君正的markdown文件管理如下，尽量保持一致：

1,  研发人员所写的markdown存放在doc/.Markdown/，对客户发布使用markdown转成的pdf文件，存放在doc/开发使用说明/ 或者doc/芯片手册之下，供客户参考，但是不能改动。

2, 客户支持人员所写的markdown存放在doc/.Markdown/客户支持/， 对客户不开放，供后续内部修改补充使用。其中每一个xxx.md对应wiki上一个页面对客户开放。每一个xxx.md对应一个pdf格式的文件提交在doc/FAE文档/下。所以，客户支持人员提交md文件要一式两份，一份是doc/.Markdown/客户支持/xxx.md，另一份是doc/FAE文档/xxx.pdf。

3, 上述doc目录在git管理之下，有利于追溯。

4, markdown文档编辑工具建议使用typera，在linux和windows下都有软件支持。单对图片插入做如下说明：

![1](markdown 编辑工具之图片插入说明.assets/1.png)

如此，会在本md文件的同级目录自动产生一个同名的xxx.assert目录，专门存放本md文件内插入的图片。

之后如果需要拷贝这个md文件的话，直接拷贝这个md文件及同目录的同名.assert目录就可以了。方便拷贝、修改、保存。

5, 每次在doc/.Markdown/客户支持/ 下提交了md文件之后，将这个md文件导出的pdf文件存放在doc/FAE文档/ 下提交，作为供客户参考的pdf发布版本使用。

