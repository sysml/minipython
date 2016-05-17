import lwip

lwip.reset()
lwip.netifadd('172.64.0.100', '255.255.255.0', '0.0.0.0')

while 1: lwip.poll()

print('done!!!!')
