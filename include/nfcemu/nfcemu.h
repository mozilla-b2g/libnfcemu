/*
 * Copyright (C) 2014  Mozilla Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef nfcemu_nfcemu_h
#define nfcemu_nfcemu_h

#include <sys/types.h>
#include "types.h"

struct nfc_device;
union nci_packet;

int
nfcemu_init(void (*log_msg)(const char* fmtstr, ...),
            void (*log_err)(const char* fmtstr, ...),
            nfcemu_timeout* (*new_timeout)(void (*cb)(void*), void* data),
            void (*mod_timeout)(nfcemu_timeout* t, unsigned long ms),
            void (*del_timeout)(nfcemu_timeout* t),
            int (*timeout_is_pending)(nfcemu_timeout* t),
            int (*send_ntf)(ssize_t (*create)(void*, struct nfc_device*,
                                              size_t, union nci_packet*),
                            void* data),
            int (*send_dta)(ssize_t (*create)(void*, struct nfc_device*,
                                              size_t, union nci_packet*),
                            void* data),
            int (*recv_dta)(ssize_t (*handle)(void*, struct nfc_device*),
                            void* data));

void
nfcemu_uninit(void);

#endif
