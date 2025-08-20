//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include <syscall.h>

#include "app/eapp_utils.h"
#include "app/syscall.h"
#include "edge/edge_common.h"

#define OCALL_PRINT_STRING 1
#define OCALL_QUIC_SERVER_LISTEN 2

unsigned long ocall_print_string(char *string);

unsigned long ocall_quic_server_listen();

int main() {
    while (1) {
        unsigned long retval = ocall_quic_server_listen();
        if(!retval) {
            ocall_print_string("Enclave recieved msg");
        } else {
            break;
        }
    }

    EAPP_RETURN(0);
}

unsigned long ocall_print_string(char *string) {
    unsigned long retval;
    ocall(OCALL_PRINT_STRING, string, strlen(string) + 1, &retval, sizeof(unsigned long));
    return retval;
}

unsigned long ocall_quic_server_listen() {
    unsigned long retval;
    ocall(OCALL_QUIC_SERVER_LISTEN, NULL, 0, &retval, sizeof(unsigned long));
    return retval;
}