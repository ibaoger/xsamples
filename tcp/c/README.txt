##使用说明

1）模拟客户端连接超时
原理：使用防火墙 iptables 模拟 SYN 包丢失。
设置：
iptables -A OUTPUT -p tcp -m tcp --tcp-flags SYN SYN --sport 32015 -j DROP
查看：
iptables -L
启动防火墙：
service iptables start
查看防火墙状态：
service iptables status
恢复：
iptables -D OUTPUT -p tcp -m tcp --tcp-flags SYN SYN --sport 32015 -j DROP


2）模拟客户端发送超时
原理：使用防火墙 iptables 模拟 PSH 包丢失。
设置：
iptables -A OUTPUT -p tcp -m tcp --tcp-flags PSH PSH --sport 32015 -j DROP
查看：
iptables -L
恢复：
iptables -D OUTPUT -p tcp -m tcp --tcp-flags PSH PSH --sport 32015 -j DROP




参考文档：
简单几招模拟网络超时情况 http://www.xiaozhangwx.com/blog/archives/73
（收录 doc/简单几招模拟网络超时情况.mht）
