##ʹ��˵��

1��ģ��ͻ������ӳ�ʱ
ԭ��ʹ�÷���ǽ iptables ģ�� SYN ����ʧ��
���ã�
iptables -A OUTPUT -p tcp -m tcp --tcp-flags SYN SYN --sport 32015 -j DROP
�鿴��
iptables -L
��������ǽ��
service iptables start
�鿴����ǽ״̬��
service iptables status
�ָ���
iptables -D OUTPUT -p tcp -m tcp --tcp-flags SYN SYN --sport 32015 -j DROP


2��ģ��ͻ��˷��ͳ�ʱ
ԭ��ʹ�÷���ǽ iptables ģ�� PSH ����ʧ��
���ã�
iptables -A OUTPUT -p tcp -m tcp --tcp-flags PSH PSH --sport 32015 -j DROP
�鿴��
iptables -L
�ָ���
iptables -D OUTPUT -p tcp -m tcp --tcp-flags PSH PSH --sport 32015 -j DROP




�ο��ĵ���
�򵥼���ģ�����糬ʱ��� http://www.xiaozhangwx.com/blog/archives/73
����¼ doc/�򵥼���ģ�����糬ʱ���.mht��
