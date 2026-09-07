sudo sysctl -w net.core.wmem_max=24862979
sudo ifconfig enp6s0f1 mtu 9000
sudo sysctl -w net.core.rmem_max=24862979
