#include<time.h>

void *receive_thread(void *arg) {
    // Cast argument to the set of file descriptors
    fd_set readfds;
    int max_fd = 0;

    while (1) {
        FD_ZERO(&readfds);
        
        // Add UDP sockets to the set
        for (int i = 0; i < MAX_SOCKETS; i++) {
            if (!shared_memory[i].is_free) {
                FD_SET(shared_memory[i].udp_socket_id, &readfds);
                if (shared_memory[i].udp_socket_id > max_fd) {
                    max_fd = shared_memory[i].udp_socket_id;
                }
            }
        }

        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 5; // 5 seconds
        timeout.tv_usec = 0;

        // Wait for incoming messages or timeout
        int activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
        if (activity < 0) {
            perror("select");
            continue;
        }

        if (activity == 0) {
            // Timeout occurred
            // Implement handling of timeout
            // Check for nospace flag and send duplicate ACK if necessary
            // ...
        } else {
            // Loop through each MTP socket to handle incoming messages
            for (int i = 0; i < MAX_SOCKETS; i++) {
                if (!shared_memory[i].is_free && FD_ISSET(shared_memory[i].udp_socket_id, &readfds)) {
                    // Receive message from UDP socket
                    char recv_buf[MAX_BUFFER_SIZE];
                    struct sockaddr_in sender_addr;
                    socklen_t sender_len = sizeof(sender_addr);
                    ssize_t bytes_received = recvfrom(shared_memory[i].udp_socket_id, recv_buf, sizeof(recv_buf), 0,
                                                      (struct sockaddr *)&sender_addr, &sender_len);
                    if (bytes_received == -1) {
                        perror("recvfrom");
                        continue;
                    }

                    // Process the received message
                    // Assuming the message format includes type (data or ACK) and sequence number
                    int message_type = recv_buf[0];
                    int sequence_number = recv_buf[1];

                    if (message_type == DATA_MESSAGE) {
                        // Store data message in the receiver-side message buffer
                        strcpy(shared_memory[i].recv_buffer, recv_buf + 2); // Assuming the data starts from index 2

                        // Send ACK message to the sender
                        // Assuming the ACK message format includes type (ACK) and sequence number
                        char ack_message[2];
                        ack_message[0] = ACK_MESSAGE;
                        ack_message[1] = sequence_number;
                        sendto(shared_memory[i].udp_socket_id, ack_message, sizeof(ack_message), 0,
                               (struct sockaddr *)&sender_addr, sender_len);

                        // Set nospace flag if receive buffer is full
                        // if (strlen(shared_memory[i].recv_buffer) == 0) {
                        //     shared_memory[i].nospace = 1;
                        // }
                    } else if (message_type == ACK_MESSAGE) {
                        // Update sender window size and remove message from sender-side buffer
                        // Assuming the ACK message format includes type (ACK) and sequence number
                        int acked_sequence_number = recv_buf[1];

                        // Update sender window size
                        // Assuming swnd array holds sequence numbers of sent but unacknowledged messages
                        for (int j = 0; j < MAX_WINDOW_SIZE; j++) {
                            if (shared_memory[i].swnd[j] == acked_sequence_number) {
                                shared_memory[i].swnd[j] = -1; // Mark as acknowledged
                                break;
                            }
                        }
                        // Remove message from sender-side buffer
                        // Assuming the sender buffer is cleared for simplicity
                    }
                }
            }
        }
    }
    return NULL;
}

// Thread function for sending messages, timeouts, and retransmissions
void *send_thread(void *arg) {
    while (1) {
        // Sleep for some time (< T/2)
        usleep(100000); // 100 milliseconds, adjust as needed

        // Get current time
        struct timeval current_time;
        gettimeofday(&current_time, NULL);

        // Loop through each MTP socket to handle sending messages
        for (int i = 0; i < num_mtp_sockets; i++) {
            if (!shared_memory[i].is_free) {
                // Check if message timeout period (T) is over
                long int time_diff = current_time.tv_sec - shared_memory[i].last_send_time.tv_sec;
                if (time_diff >= TIMEOUT_PERIOD) { // Define TIMEOUT_PERIOD as needed
                    // Retransmit all messages within the current swnd
                    for (int j = 0; j < MAX_WINDOW_SIZE; j++) {
                        if (shared_memory[i].swnd[j] != -1) {
                            // Retransmit message
                            // Assuming the sender buffer contains the message to be retransmitted
                            sendto(shared_memory[i].udp_socket_id, shared_memory[i].send_buffer, strlen(shared_memory[i].send_buffer), 0,
                                   (struct sockaddr *)&shared_memory[i].destination_addr, sizeof(shared_memory[i].destination_addr));

                            // Update last send time
                            gettimeofday(&shared_memory[i].last_send_time, NULL);
                        }
                    }
                }

                // Check if there is a pending message from the sender-side buffer that can be sent
                if (strlen(shared_memory[i].send_buffer) > 0) {
                    // Send the message through the UDP socket
                    sendto(shared_memory[i].udp_socket_id, shared_memory[i].send_buffer, strlen(shared_memory[i].send_buffer), 0,
                           (struct sockaddr *)&shared_memory[i].destination_addr, sizeof(shared_memory[i].destination_addr));

                    // Update last send time
                    gettimeofday(&shared_memory[i].last_send_time, NULL);

                    // Clear sender-side buffer
                    memset(shared_memory[i].send_buffer, 0, sizeof(shared_memory[i].send_buffer));
                }
            }
        }
    }
    return NULL;
}


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


