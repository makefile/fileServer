
启动程序后将成为守护进程。
程序会读取配置文件/etc/my_httpd.conf,目前这个文件在~/my_httpd/下，用户可以修改这个文件。make install时因该将此文件复制到适当位置，/etc/下需要管理员权限。

可以通过netstat -ln|grep myHttpd来查看是否在监听（-l只显示正在监听的，-n以数字形式ip显示而不是主机名称）

停止服务目前可以pkill myHttpd
或ps,top等查找进程id,再kill [id]
后续增加stop等参数来关闭。
