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

#include <assert.h>
#include <string.h>
#include "bswap.h"
#include "nfc-debug.h"
#include "nfc.h"
#include "nfc-hci.h"

static size_t
create_evt(union hci_answer* rsp, enum hci_command cmd, unsigned char l)
{
    assert(rsp);

    rsp->evt.common.cmd = cmd;
    rsp->evt.common.l = l;

    return sizeof(rsp->evt.common) + l;
}

static size_t
create_evt_cmd_complete(union hci_answer* rsp,
                        enum hci_service service,
                        enum hci_command cmd,
                        enum hci_status_code status)
{
    assert(rsp);

    /* TODO: the Nexus 4 returns '1' for the first value. We should
     * figure out why this doesn't work for us.
     */
    rsp->evt.cmd_complete.npackets = HCI_BCM2079x_CMD_WRITE_SLEEP_MODE;
    rsp->evt.cmd_complete.service = service;
    rsp->evt.cmd_complete.cmd = cmd;
    rsp->evt.cmd_complete.status = status;

    return create_evt(rsp, HCI_BCM2079x_EVT_CMD_COMPLETE, 4);
}

/* IDLE state */

static size_t
idle_process_bcm2079x_write_sleep_mode_cmd(const union hci_packet* cmd,
                                           struct nfc_device* nfc,
                                           union hci_answer* rsp)
{
    assert(rsp);

    return create_evt_cmd_complete(rsp,
                                   cmd->control.service,
                                   cmd->control.cmd, HCI_STATUS_OK);
}

static size_t
idle_process_cmd(const union hci_packet* cmd, struct nfc_device* nfc,
                 union hci_answer* rsp)
{
    size_t len = 0;

    assert(cmd);

    if (cmd->control.service == HCI_SERVICE_BCM2079x) {
        switch (cmd->control.cmd) {
            case HCI_MESSAGE_RFU|HCI_BCM2079x_CMD_WRITE_SLEEP_MODE:
                len = idle_process_bcm2079x_write_sleep_mode_cmd(cmd, nfc, rsp);
                break;
            default:
                NFC_D("unknown command");
                return 0;
        }
    }
    NFC_D("result length=%ld", (long)len);

    return len;
}

/* RESET state */

static size_t
reset_process_cmd(const union hci_packet* cmd, struct nfc_device* nfc,
                  union hci_answer* rsp)
{
    assert(0);

    return 0;
}

/* INITIALIZED state */

static size_t
init_process_bcm2079x_write_sleep_mode_cmd(const union hci_packet* cmd,
                                           struct nfc_device* nfc,
                                           union hci_answer* rsp)
{
    assert(rsp);

    return create_evt_cmd_complete(rsp,
                                   cmd->control.service,
                                   cmd->control.cmd, HCI_STATUS_OK);
}

static size_t
init_process_cmd(const union hci_packet* cmd, struct nfc_device* nfc,
                 union hci_answer* rsp)
{
    size_t len = 0;

    assert(cmd);

    if (cmd->control.service == HCI_SERVICE_BCM2079x) {
        switch (cmd->control.cmd & ~HCI_MESSAGE_RFU) {
            case HCI_BCM2079x_CMD_WRITE_SLEEP_MODE:
                len = init_process_bcm2079x_write_sleep_mode_cmd(cmd, nfc, rsp);
                break;
            default:
                NFC_D("unknown command");
                return 0;
        }
    }
    NFC_D("result length=%ld", (long)len);

    return len;
}

int
nfc_process_hci_cmd(const union hci_packet* cmd, struct nfc_device* nfc,
                    union hci_answer* rsp)
{
    static size_t (* const process[NUMBER_OF_NFC_FSM_STATES])
      (const union hci_packet*, struct nfc_device*, union hci_answer*) = {
        [NFC_FSM_STATE_IDLE] = idle_process_cmd,
        [NFC_FSM_STATE_RESET] = reset_process_cmd,
        [NFC_FSM_STATE_INITIALIZED] = init_process_cmd
    };

    NFC_D("HCI service=%x cmd=%x; NFC state=%d",
          cmd->common.service, cmd->common.cmd, nfc->state);

    assert(nfc->state < NUMBER_OF_NFC_FSM_STATES);
    assert(process[nfc->state]);

    return process[nfc->state](cmd, nfc, rsp);
}
