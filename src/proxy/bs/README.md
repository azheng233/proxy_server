# urlserver
BS主服务程序


## 安装步骤
依赖 cmake。


### 克隆仓库
 git clone --recursive git@gitlab.idc.safecenter.cn:BS/urlserver.git

 如果克隆时没有加 --recursive 参数，在克隆后需要在urlserver目录中执行：

  git submodule init

   git submodule update


### 编译安装
    make -e G="Unix Makefiles" config

     make

      make install


### 说明
      - 默认生成 Release 版本，若想生成 Debug 版本，在 make config 时指定环境变量 DEBUG=true
      - 若想要安装在当前目录以外的路径，在 make config 时指定环境变量 PREFIX="/path/to/install"。
      - 若依赖的库没有安装在系统目录下，在 make config 时指定环境变量 LIBPATH="/path/of/libs"。
      - 使用 make depclean 来清除 cmake 产生的临时文件。

### 历史版本支持操作系统
#### 32位：
     - Red Hat Enterprise Linux Server release 5.5 (Tikanga)
       2.6.18-194.el5 #1 SMP Tue Mar 16 21:52:43 EDT 2010 i686 i686 i386 GNU/Linux

#### 64位：
     - CentOS release 5.8 (Final)
       2.6.18-308.el5 #1 SMP Tue Feb 21 20:06:06 EST 2012 x86_64 x86_64 x86_64 GNU/Linux
