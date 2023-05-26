<center><font size = 4>MySQL 存储引擎开发</font></center>

推荐使用 Linux 进行开发，这样可以最大程度的避免因环境配置而造成的问题

1. 下载 MySQL 8.0 源码

   前往 [MySQL 官方下载地址](https://dev.mysql.com/downloads/mysql/)，Operating System 选择 Source Code，拉到最下面，选择 **All Operating Systems (Generic) (Architecture Independent), Compressed TAR Archive** ，是否带 Boost 可选，下载下来的文件为 `mysql-8.0.*.tar.gz`，在 Linux 系统中使用 `tar -xzvf mysql-8.0.*.tar.gz` 进行解压

2. 需要的编译条件(以 Ubuntu/Debian 为例子)

   ```shell
   sudo apt-get update
   sudo apt-get upgrade -y
   sudo apt-get install cmake build-essential bison libssl-dev openssl graphviz pkg-config -y
   ```

   之后进去刚才解压好的 `mysql` 源码目录，如果你没有下载 `boost` 库的话，

   ```shell
   cmake -DCMAKE_BUILD_TYPE=Debug -DFORCE_INSOURCE_BUILD=1 -DDOWNLOAD_BOOST=1 -DWITH_BOOST=/path/to/where/you/want/to/download
   ```

   为方便管理，推荐首先使用

   ```shell
   echo $HOME/boost
   ```

   得到一个在根目录下的目录然后将它替换命令中的 `/path/to/where/you/want/to/download` 

   经过一段时间后完成 `Makefile` 的生成，然后编译 `MySQL` 服务端和客户端，回到 `MySQL` 源码根目录，输入 `make -j$(nproc)` ，之后等待编译完成，这步耗时较长

   等待编译完成后，输入 `sudo make install` 进行安装，安装完成后，进入到 `/usr/local/mysql/bin` 目录下 (`cd /usr/local/mysql/bin`) ，输入 `./mysqld --initialize --datadir=/path/where/data/store`，这里推荐 `datadir` 为 `$HOME/data` ，同样的，为了方便管理和观察，你也可以自行选择目录，同时这里会输出一个 `MySQL` 的初始密码，需要记录下，后面需要使用

   之后需要两个终端，一个作为服务端，一个作为客户端，都进入到 `/usr/local/mysql/bin` 目录，服务端使用 `./mysqld --datadir=/path/where/data/store` 这里使用上面初始化的相同路径，而另一个客户端先使用 `./mysqladmin password --user=root --password` 输入初始密码，之后更改密码，到这里 `MySQL` 的开发环境搭建完成，接下来进入到插件的开发

3. 创建一个插件

   压缩包中提供了一个 `ldb` 文件夹，将它复制到 `MySQL` 源码目录下的 `/storage` 下，之后回到 `MySQL` 源码根目录，再次输入 

   ```shell
   cmake -DCMAKE_BUILD_TYPE=Debug -DFORCE_INSOURCE_BUILD=1 -DDOWNLOAD_BOOST=1 -DWITH_BOOST=/path/to/where/you/want/to/download
   ```

   记得做路径替换，之后进入到源码目录下的 `/storage/ldb` 中，观察是否有 `Makefile` 生成，若有可以进行一下测试，进入 `storage/ldb` 然后输入 `make` 编译得到动态链接库，回到源码根目录，输入 `sudo ln -s /path/to/plugin_output_directory/ha_ldb.so /usr/local/mysql/lib/plugin` 创建一个软链接 (这步仅第一次需要，需要使用绝对路径) 之后重启服务端 (关闭服务端，并再次执行启动服务端的命令即可)，在客户端中输入

   ```sql
   install plugin lengine soname 'ha_ldb.so';
   ```

   之后可以使用 

   ```sql
   show engines;
   ```

   来验证是否成功

4. 完善代码后再次使用 `make` 重新编译插件即可，之后重启服务端便可以验证了

   (以上步骤为配置后依照记忆重写，可能会有地方需要修改)