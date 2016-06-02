try:
    import usocket as socket
except:
    import socket


CONTENT = b"""\
HTTP/1.1 200 OK
Content-Length: 23

Hello from MicroPython!
"""

def main(use_stream=False):
    s = socket.socket()

    # Binding to all interfaces - server will be accessible to other hosts!
    ai = socket.getaddrinfo("172.64.0.100", 8080)
    print("Bind address info:", ai)
    addr = ai[0][-1]

    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(addr)
    s.listen(5)
    print("Listening, connect your browser to http://172.64.0.100:8080/")

    counter = 0
    while True:
        res = s.accept()
        client_s = res[0]
        client_addr = res[1]
        print("Client address:", client_addr)
        print("Client socket:", client_s)
        print("Request:")
        if use_stream:
            # MicroPython socket objects support stream (aka file) interface
            # directly.
            print(client_s.read(4096))
            client_s.write(CONTENT)
        else:
            print(client_s.recv(4096))
            client_s.send(CONTENT)
        client_s.close()
        counter += 1
        print()


main()
