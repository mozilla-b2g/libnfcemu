/*
 * Copyright (C) 2013-2014  Mozilla Foundation
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

#ifndef goldfish_nfc_h
#define goldfish_nfc_h

#include <stddef.h>
#include <sys/types.h>

struct nfc_device;
union nci_packet;

int
goldfish_nfc_send_dta(ssize_t (*create)(void*, struct nfc_device*, size_t,
                                        union nci_packet*), void* data);

int
goldfish_nfc_send_ntf(ssize_t (*create)(void*, struct nfc_device*, size_t,
                                        union nci_packet*), void* data);

int
goldfish_nfc_recv_dta(ssize_t (*recv)(void*, struct nfc_device*), void* data);

#endif
