#include<time.h>
// int m_recvfrom(int sockfd, void *buf, size_t len, int flags,
//                struct sockaddr *src_addr, socklen_t *addrlen) {
//     // Check if socket type is SOCK_MTP (not implemented here)
//     if (sockfd < 0 || buf == NULL || len == 0) {
//         mtp_errno = EINVAL;
//         return -1;
//     }

//     // Assuming shared_memory is accessible here
//     // and is an array of MTPSocket structures

//     // Find the corresponding entry in shared memory
//     int entry_index = -1;
//     for (int i = 0; i < num_mtp_sockets; i++) {
//         if (shared_memory[i].udp_socket_id == sockfd) {
//             entry_index = i;
//             break;
//         }
//     }

//     // Check if entry found
//     if (entry_index == -1) {
//         // Entry not found in shared memory
//         mtp_errno = EINVAL;
//         return -1;
//     }

//     // Check if there is any message in the receiver-side buffer
//     if (strlen(shared_memory[entry_index].recv_buffer) == 0) {
//         // No message available
//         mtp_errno = ENOMSG;
//         return -1;
//     }

//     // Copy the first message from the receiver-side buffer to the provided buffer
//     strcpy(buf, shared_memory[entry_index].recv_buffer);

//     // Clear the receiver-side buffer (delete the message)
//     memset(shared_memory[entry_index].recv_buffer, 0, MAX_BUFFER_SIZE);

//     // Return the length of the copied message
//     return strlen(buf);
// }


// // m_socket function

int m_close(int sockfd) {
    // Check if socket type is SOCK_MTP
    if (sockfd < 0) {
        mtp_errno = EINVAL;
        return -1;
    }

    // Find the corresponding entry in shared memory
    int entry_index = -1;
    for (int i = 0; i < num_mtp_sockets; i++) {
        if (shared_memory[i].udp_socket_id == sockfd) {
            entry_index = i;
            break;
        }
    }

    // Check if entry found
    if (entry_index == -1) {
        // Entry not found in shared memory
        mtp_errno = EINVAL;
        return -1;
    }

    // Close the UDP socket
    close(sockfd);

    // Clean up the corresponding socket entry in shared memory
    shared_memory[entry_index].is_free = 1;
    shared_memory[entry_index].pid = 0;
    shared_memory[entry_index].udp_socket_id = -1;
    memset(shared_memory[entry_index].other_end_ip, 0, sizeof(shared_memory[entry_index].other_end_ip));
    shared_memory[entry_index].other_end_port = 0;
    memset(shared_memory[entry_index].send_buffer, 0, sizeof(shared_memory[entry_index].send_buffer));
    memset(shared_memory[entry_index].recv_buffer, 0, sizeof(shared_memory[entry_index].recv_buffer));
    memset(shared_memory[entry_index].swnd, 0, sizeof(shared_memory[entry_index].swnd));
    memset(shared_memory[entry_index].rwnd, 0, sizeof(shared_memory[entry_index].rwnd));
    memset(&shared_memory[entry_index].destination_addr, 0, sizeof(shared_memory[entry_index].destination_addr));

    return 0;
}


