gcc -c initialize.c -o initialize.o
gcc -c initmsocket.c -o initmsocket.o
gcc initialize.o initmsocket.o -o run_threads -pthread
chmod +x run_threads
# gnome-terminal -- bash -c "./run_threads; exec bash"


gcc -c m_socket.c -o m_socket.o
gcc -c user.c -o user.o
gcc m_socket.o user.o initialize.o -o executable
chmod +x executable
gnome-terminal -- bash -c "./executable; exec bash"


