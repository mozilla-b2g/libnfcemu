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

#include <assert.h>
#include "cb.h"
#include <nfcemu/nfcemu.h>

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
                            void* data))
{
  assert(new_timeout);
  assert(mod_timeout);
  assert(del_timeout);
  assert(timeout_is_pending);
  assert(send_ntf);
  assert(send_dta);
  assert(recv_dta);

  cb.log_msg = log_msg;
  cb.log_err = log_err;
  cb.new_timeout = new_timeout;
  cb.mod_timeout = mod_timeout;
  cb.del_timeout = del_timeout;
  cb.timeout_is_pending = timeout_is_pending;
  cb.send_ntf = send_ntf;
  cb.send_dta = send_dta;
  cb.recv_dta = recv_dta;

  return 0;
}

void
nfcemu_uninit()
{
  return;
}
