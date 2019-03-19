/*
 * Copyright (C) 2011-2018 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * File: app.cpp
 *
 * Description: Sample application to
 * demonstrate the usage of quote generation.
 */

#include <assert.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#if defined(_MSC_VER)
#include <Windows.h>
#include <tchar.h>
#endif

#include "sgx_urts.h"
#include "sgx_report.h"
#include "sgx_dcap_ql_wrapper.h"
#include "sgx_pce.h"
#include "sgx_error.h"

#include "Enclave_u.h"

#include "ecdsa-aesmd-messages.pb-c.h"

#if defined(_MSC_VER)
#define ENCLAVE_PATH _T("enclave.signed.dll")
#else
#define ENCLAVE_PATH "enclave.signed.so"
#endif

static void print_byte_array(FILE* f, uint8_t* data, int size) {
    for (int i = 0; i < size; ++i) {
        fprintf(f, "%02X", data[i]);
    }
}

bool create_app_enclave_report(sgx_target_info_t qe_target_info, sgx_report_t *app_report)
{
        bool ret = true;
        uint32_t retval = 0;
        sgx_status_t sgx_status = SGX_SUCCESS;
        sgx_enclave_id_t eid = 0;
        int launch_token_updated = 0;
        sgx_launch_token_t launch_token = { 0 };

        sgx_status = sgx_create_enclave(ENCLAVE_PATH,
                SGX_DEBUG_FLAG,
                &launch_token,
                &launch_token_updated,
                &eid,
                NULL);
        if (SGX_SUCCESS != sgx_status) {
                printf("Error, call sgx_create_enclave fail [%s], SGXError:%04x.\n", __FUNCTION__, sgx_status);
                ret = false;
                goto CLEANUP;
        }


        sgx_status = enclave_create_report(eid,
                &retval,
                &qe_target_info,
                app_report);
        if ((SGX_SUCCESS != sgx_status) || (0 != retval)) {
                printf("\nCall to get_app_enclave_report() failed\n");
                ret = false;
                goto CLEANUP;
        }

CLEANUP:
        sgx_destroy_enclave(eid);
        return ret;
}

static void process_initquoterequest(int fd, Quoteservice__Message__Request* msg) {
    Quoteservice__Message__Response__InitQuoteResponse response =
        QUOTESERVICE__MESSAGE__RESPONSE__INIT_QUOTE_RESPONSE__INIT;
    Quoteservice__Message__Response wrapper_msg =
        QUOTESERVICE__MESSAGE__RESPONSE__INIT;
    wrapper_msg.initquoteresponse = &response;

    sgx_target_info_t qe_target_info = {0, };
    quote3_error_t qe3_ret = sgx_qe_get_target_info(&qe_target_info);
    assert(qe3_ret == SGX_QL_SUCCESS);
    response.targetinfo.data = (uint8_t*) &qe_target_info;
    response.targetinfo.len = sizeof(qe_target_info);
    response.has_targetinfo = 1;

    qe3_ret = sgx_qe_get_quote_size(&response.quote_size);
    assert(SGX_QL_SUCCESS == qe3_ret);
    
    uint32_t payload_len = quoteservice__message__response__get_packed_size(&wrapper_msg);
    uint32_t total_len = payload_len + sizeof(payload_len);
    uint8_t buf[1024];
    assert(total_len <= sizeof(buf));
    memcpy(buf, (uint8_t*)&payload_len, sizeof(payload_len));
    quoteservice__message__response__pack(&wrapper_msg, buf + sizeof(payload_len));

    int rc = send(fd, buf, total_len, 0);
    assert(rc == total_len);
}

static void process_getquoterequest(int fd, Quoteservice__Message__Request* msg) {
    assert(NULL != msg->getquoterequest);
    
    Quoteservice__Message__Request__GetQuoteRequest* request =
        (Quoteservice__Message__Request__GetQuoteRequest*) msg->getquoterequest;
    Quoteservice__Message__Response__GetQuoteResponse response =
        QUOTESERVICE__MESSAGE__RESPONSE__GET_QUOTE_RESPONSE__INIT;
    Quoteservice__Message__Response wrapper_msg =
        QUOTESERVICE__MESSAGE__RESPONSE__INIT;
    wrapper_msg.getquoteresponse = &response;

    uint32_t quote_size = 0;
    quote3_error_t qe3_ret = sgx_qe_get_quote_size(&quote_size);
    assert(SGX_QL_SUCCESS == qe3_ret);

    uint8_t* quote_buffer = (uint8_t*) calloc(1, quote_size);
    assert(NULL != quote_buffer);

    print_byte_array(stdout, request->report.data, request->report.len);
    printf("\n");
    
    qe3_ret = sgx_qe_get_quote((sgx_report_t*) request->report.data,
                               quote_size, quote_buffer);
    assert(SGX_QL_SUCCESS == qe3_ret);
    response.quote.data = quote_buffer;
    response.quote.len  = quote_size;
    response.has_quote  = 1;
    
    uint32_t payload_len = quoteservice__message__response__get_packed_size(&wrapper_msg);
    uint32_t total_len = payload_len + sizeof(payload_len);
    uint8_t buf[4096];
    assert(total_len <= sizeof(buf));
    memcpy(buf, (uint8_t*)&payload_len, sizeof(payload_len));
    quoteservice__message__response__pack(&wrapper_msg, buf + sizeof(payload_len));

    int rc = send(fd, buf, total_len, 0);
    assert(rc == total_len);
}

static void accept_loop(void) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(-1 != sockfd);
    struct sockaddr_in srvaddr = { 0, };
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &srvaddr.sin_addr);
    int enable = 1;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    assert(ret != -1);
    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
    assert(ret != -1);
    ret = bind(sockfd, (struct sockaddr*)&srvaddr, sizeof(srvaddr));
    assert(-1 != ret);
    ret = listen(sockfd, 0);
    assert(-1 != ret);

    do {
        struct sockaddr_in clientaddr;
        socklen_t client_len = sizeof(clientaddr);
        const int clientfd = accept(sockfd, (struct sockaddr*) &clientaddr, &client_len);
        assert(-1 != clientfd);

        do {
            uint32_t msglen;
            ret = read(clientfd, &msglen, sizeof(msglen));
            uint8_t buf[512];
            assert(msglen <= sizeof(buf));
            ret = read(clientfd, buf, msglen);
            if (ret == 0) break; // EOF, i.e., client hung up
            assert(ret == msglen);

            Quoteservice__Message__Request* msg =
                quoteservice__message__request__unpack(NULL, msglen, buf);

            if (msg->initquoterequest) {
                process_initquoterequest(clientfd, msg);
            } else if (msg->getquoterequest) {
                process_getquoterequest(clientfd, msg);
            } else {
                assert(0);
            }
        } while (1);
        
        close(clientfd);
    } while (1);
}

int main(int argc, char* argv[])
{
    (void)(argc);
    (void)(argv);

    accept_loop();
    
    return 0;
}
