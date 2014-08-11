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
#include "nfc-debug.h"
#include "nfc-rf.h"

void
nfc_rf_init(struct nfc_rf* rf, enum nci_rf_interface iface, enum nci_rf_tech_mode mode)
{
    assert(rf);

    rf->iface = iface;
    rf->mode = mode;
}

enum nfc_rfst
nfc_rf_state_transition(enum nfc_rfst* rf_state, unsigned long bits,
                        enum nfc_rfst state)
{
    enum nfc_rfst rfst;

    assert(rf_state);

    rfst = *rf_state;

    if (!((1<<rfst) & bits)) {
        return NUMBER_OF_NFC_RFSTS;
    }

    NFC_D("rf_state from %d to %d\n", *rf_state, state);

    *rf_state = state;

    return rfst;
}
