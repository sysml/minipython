import lwip
lwip.reset()
eth = lwip.ether('172.64.0.100', '255.255.255.0', '0.0.0.0')

while 1: eth.poll()
