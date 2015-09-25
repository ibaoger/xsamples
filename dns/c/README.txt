##使用说明

部署DNS服务器
dnspod-sr https://github.com/DNSPod/dnspod-sr （至少2核2G，否则报 killed 错误）


1）模拟客户端接收超时
原理：使用防火墙 iptables 模拟 PSH 包丢失。
设置：
iptables -A OUTPUT -p udp --sport 53 -j DROP
查看：
iptables -L
启动防火墙：
service iptables start
查看防火墙状态：
service iptables status
恢复：
iptables -D OUTPUT -p udp --sport 53 -j DROP


2）模拟客户端发送超时
原理：使用防火墙 iptables 模拟 PSH 包丢失。
设置：
iptables -A INPUT -p udp --sport 53 -j DROP


4）TCP超时测试用例
环境：ECS1运行 server，ECS2运行 client；
4.1 无任何设置：双向通信正常
4.2 ECS1模拟客户端连接超时：当client不强制关闭时，超时10秒后断开；当client强制3秒关闭时，第3秒准时断开；
4.3 ECS1模拟客户端接收超时：当client不强制关闭时，超时10秒后断开；当client强制3秒关闭时，第3秒准时断开；
4.4 ECS1模拟客户端发送超时：当client不强制关闭时，超时10秒后断开；当client强制3秒关闭时，第3秒准时断开；


参考文档：
简单几招模拟网络超时情况 http://www.xiaozhangwx.com/blog/archives/73
（收录 doc/简单几招模拟网络超时情况.mht）
