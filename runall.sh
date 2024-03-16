gcc initmsocket.c -o init
chmod +x init
gnome-terminal -- bash -c "./init; exec bash"
gcc user.c m_socket.c initmsocket.c
./a.out
