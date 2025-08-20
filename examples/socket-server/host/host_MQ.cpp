//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
// 消息队列版本
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "edge/edge_call.h"
#include "host/keystone.h"

#define OCALL_PRINT_STRING 1
#define OCALL_QUIC_SERVER_LISTEN 2
#define PORT 8080
#define BUFFER_SIZE 1024

typedef struct msg_node {
  char *msg;
  struct msg_node *next;
} MSG_NODE;
MSG_NODE* msg_list_head = (MSG_NODE*)malloc(sizeof(MSG_NODE));
MSG_NODE* msg_list_tail = (MSG_NODE*)malloc(sizeof(MSG_NODE));
int msg_num;
pthread_mutex_t msg_mutex = PTHREAD_MUTEX_INITIALIZER;
int server_fd, client_socket;

void print_string_wrapper(void* buffer) {
  /* Parse and validate the incoming call data */
  struct edge_call* edge_call = (struct edge_call*)buffer;
  uintptr_t call_args;
  unsigned long ret_val;
  size_t arg_len;
  if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
    return;
  }

  /* Pass the arguments from the eapp to the exported ocall function */
  ret_val = printf("Enclave said: \"%s\"\n", (char*)call_args);

  /* Setup return data from the ocall function */
  uintptr_t data_section = edge_call_data_ptr();
  memcpy((void*)data_section, &ret_val, sizeof(unsigned long));
  if (edge_call_setup_ret(
          edge_call, (void*)data_section, sizeof(unsigned long))) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
  } else {
    edge_call->return_data.call_status = CALL_STATUS_OK;
  }

  /* This will now eventually return control to the enclave */
  return;
}

void xquic_server_setup() {
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  char buffer[BUFFER_SIZE] = {0};

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
      perror("socket failed");
      exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
      perror("bind failed");
      close(server_fd);
      exit(EXIT_FAILURE);
  }

  if (listen(server_fd, 1) < 0) {
      perror("listen");
      close(server_fd);
      exit(EXIT_FAILURE);
  }

  printf("Server is listening on port %d...\n", PORT);
  if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
    perror("accept");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  printf("Connection from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
  
  while (1) {
    int n = read(client_socket, buffer, BUFFER_SIZE);
    if (n <= 0) {
        if (n == 0) {
            printf("Client disconnected\n");
        } else {
            perror("read");
        }
        break;
    }
    pthread_mutex_lock(&msg_mutex);
    printf("Received from client: %s", buffer);
    if(msg_num == 0) {
        strcpy(msg_list_head->msg, (char*)buffer);
        msg_list_tail = msg_list_head;
    } else {
        MSG_NODE* node = (MSG_NODE*)malloc(sizeof(MSG_NODE));
        strcpy(node->msg, (char*)buffer);
        msg_list_tail->next = node;
        msg_list_tail = msg_list_tail->next;
    }
    msg_num ++;
    pthread_mutex_unlock(&msg_mutex);
    memset(buffer, 0, BUFFER_SIZE);
  }
}

void xquic_server_send2enclave(void* buffer) {
  /* Parse and validate the incoming call data */
  struct edge_call* edge_call = (struct edge_call*)buffer;
  uintptr_t call_args;
  size_t arg_len;
  char* ret_val;
  if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
    return;
  }

  /* Pass the arguments from the eapp to the exported ocall function */
  while(msg_num == 0);
  pthread_mutex_lock(&msg_mutex);
  ret_val = msg_list_head->msg;
  free(msg_list_head);
  msg_list_head = msg_list_head->next;
  msg_num --;
  pthread_mutex_unlock(&msg_mutex);

  /* Setup return data from the ocall function */
  uintptr_t data_section = edge_call_data_ptr();
  memcpy((void*)data_section, &ret_val, sizeof(ret_val));
  if (edge_call_setup_ret(
          edge_call, (void*)data_section, sizeof(ret_val))) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
  } else {
    edge_call->return_data.call_status = CALL_STATUS_OK;
  }

  /* This will now eventually return control to the enclave */
  return;
}


int main(int argc, char** argv) {
  Keystone::Enclave enclave;
  Keystone::Params params;

  params.setFreeMemSize(1024 * 1024);
  // params.setUntrustedMem(DEFAULT_UNTRUSTED_PTR, 1024 * 1024);
  params.setUntrustedSize( 1024 * 1024);
  enclave.init(argv[1], argv[2], params);

  enclave.registerOcallDispatch(incoming_call_dispatch);

  /* We must specifically register functions we want to export to the
     enclave. */
  register_call(OCALL_PRINT_STRING, print_string_wrapper);
  register_call(OCALL_QUIC_SERVER_LISTEN, xquic_server_send2enclave);

  edge_call_init_internals(
      (uintptr_t)enclave.getSharedBuffer(), enclave.getSharedBufferSize());

  xquic_server_setup();
  enclave.run();

  return 0;
}

