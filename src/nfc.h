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

#ifndef nfc_h
#define nfc_h

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <nfcemu/types.h>
#include "nfc-rf.h"

struct nfc_re;
union nci_packet;

enum {
    NUMBER_OF_SUPPORTED_NCI_RF_INTERFACES = 8
};

enum nfc_fsm_state {
    NFC_FSM_STATE_IDLE = 0,
    NFC_FSM_STATE_RESET,
    NFC_FSM_STATE_INITIALIZED,
    NUMBER_OF_NFC_FSM_STATES
};

struct nfc_device {
    enum nfc_fsm_state state;
    enum nfc_rfst rf_state;

    /* Support Frame and NFC-DEP interface */
    struct nfc_rf rf[NUMBER_OF_SUPPORTED_NCI_RF_INTERFACES];
    uint8_t id;

    struct nfc_re* active_re;
    struct nfc_rf* active_rf;

    /* stores all config options */
    uint8_t config_id_value[128];
};

void
nfc_device_init(struct nfc_device* nfc);

void
nfc_device_set(struct nfc_device* nfc, size_t off, size_t len,
               const void* value);

void
nfc_device_get(const struct nfc_device* nfc, size_t off, size_t len,
               void* value);

uint8_t
nfc_device_incr_id(struct nfc_device* nfc);

struct nfc_rf*
nfc_find_rf_by_protocol_and_mode(struct nfc_device* nfc,
                                 enum nci_rf_protocol proto, enum nci_rf_tech_mode mode);

void
nfc_delivery_cb_setup(struct nfc_delivery_cb* cb, enum nfc_buf_type type,
                      void* data, ssize_t (*func)(void*, union nci_packet*));

#endif
