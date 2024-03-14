int main() {
    // Create and initialize shared memory (not shown here)
    init();
    // Example usage of m_bind
    int mtp_socket = m_socket(AF_INET, SOCK_DGRAM, 0);
    if (mtp_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(12345); // Example source port

    if (m_bind(mtp_socket, (struct sockaddr *)&local_addr, sizeof(local_addr), "destination_ip", 54321) == -1) {
        perror("m_bind");
        exit(EXIT_FAILURE);
    }

    // Use the MTP socket
    // ...

    return 0;
}