# chatserver
基于muduo库实现的可以工作在nginx tcp负载均衡环境中的集群聊天服务器和客户端源码（redis消息队列、mysql））

编译方式
cd build
camke ..
make

nginx的tcp负载均衡
cd /usr/local/nginx
cd /sbin
./nginx
