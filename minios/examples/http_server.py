try:
    import usocket as socket
except:
    import socket

def readfile(filename):
    f = open(filename, 'r')
    s = f.read()
    f.close()    
    return s

def main():  
    s = socket.socket()

    ai = socket.getaddrinfo("0.0.0.0", 8080)
    addr = ai[0][-1]

    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(addr)
    s.listen(5)
    print("Listening, connect your browser to http://172.64.0.100:8080/")

    while True:
        f = readfile("index.html")        
        res = s.accept()
        client_s = res[0]
        client_addr = res[1]
        client_s.recv(4096)
        client_s.send(f)
        client_s.close()

main()
