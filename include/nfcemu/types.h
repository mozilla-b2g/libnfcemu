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

#ifndef nfcemu_types_h
#define nfcemu_types_h

#include <stdint.h>

union nci_packet;

typedef void nfcemu_timeout;

/* the buffer-type for the delivery callback */
enum nfc_buf_type {
  NO_BUF = 0,
  NTFN_BUF,
  DATA_BUF
};

/* supplied to process_{nci,hci}_message */
struct nfc_delivery_cb {
  enum nfc_buf_type type;
  void* data;
  ssize_t (*func)(void* /*user_data*/, union nci_packet*);
};

enum {
  MAX_NCI_PAYLOAD_LENGTH = 256
};

#endif
