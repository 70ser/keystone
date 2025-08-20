//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include "app/eapp_utils.h"
#include "app/syscall.h"
#include "edge/edge_common.h"

#define OCALL_PRINT_STRING 1
#define OCALL_QUIC_CLIENT_SEND 2

unsigned long ocall_print_string(char* string);
unsigned long ocall_quic_client_send(char* msg);

int main() {
  char msg[1024] = "Test Client";
  ocall_quic_client_send(msg);
  ocall_print_string("Client sent msg");

  EAPP_RETURN(0);
}

unsigned long ocall_print_string(char* string) {
  unsigned long retval;
  ocall(OCALL_PRINT_STRING, string, strlen(string) + 1, &retval, sizeof(unsigned long));
  return retval;
}

unsigned long ocall_quic_client_send(char* msg) {
  unsigned long retval;
  ocall(OCALL_QUIC_CLIENT_SEND, msg, sizeof(msg) + 1, &retval, sizeof(unsigned long));
  return retval;
}
