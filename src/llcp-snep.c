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
#include <limits.h>
#include "ptr.h"
#include "nfc-debug.h"
#include "llcp.h"
#include "snep.h"
#include "llcp-snep.h"

static size_t
process_req_put(const struct snep* snep, struct llcp_data_link* dl,
                      struct snep* rsp)
{
    uint32_t sneplen;

    NFC_D("SNEP Put");

    sneplen = be32_to_cpu(snep->len);

    if (!(sneplen < sizeof(dl->rbuf))) {
        NFC_D("SNEP responding 'Excess Data'");
        return snep_create_rsp_excess_data(rsp);
    }

    /* copy information field into dl->rbuf */
    llcp_dl_write_rbuf(dl, sneplen, snep->info);

    return snep_create_rsp_success(rsp, 0);
}

static size_t
process_rsp_success(const struct snep* snep, struct llcp_data_link* dl,
                          struct snep* rsp)
{
    NFC_D("SNEP Success");

    return 0;
}

static int
version_is_supported(unsigned char major, unsigned char minor)
{
    return ((major << 4) | minor) <=
           ((SNEP_VERSION_MAJOR << 4) | SNEP_VERSION_MINOR);
}

static size_t
process_msg(const struct snep* snep, struct llcp_data_link* dl,
                  struct snep* rsp)
{
    static size_t (*const process[])(const struct snep*,
                                           struct llcp_data_link*,
                                           struct snep*) = {
        [SNEP_REQ_PUT]     = process_req_put,
        [SNEP_RSP_SUCCESS] = process_rsp_success
    };

    if (!version_is_supported(snep->ver.major, snep->ver.minor)) {
        NFC_D("SNEP responding 'Unsupported Version'");
        return snep_create_rsp_unsupported_version(rsp);
    }
    if ((snep->msg >= ARRAY_SIZE(process)) || !process[snep->msg]) {
        NFC_D("SNEP responding 'Not Implemented'");
        return snep_create_rsp_not_implemented(rsp);
    }
    return process[snep->msg](snep, dl, rsp);
}

size_t
llcp_sap_snep(struct llcp_data_link* dl, const uint8_t* info, size_t len,
              struct snep* rsp)
{
    const struct snep* snep;
    uint32_t sneplen;

    assert(dl);
    assert(info);
    assert(len >= 6);
    assert(rsp);

    if (len < sizeof(*snep)) {
        NFC_D("SNEP responding 'Bad Request'");
        return snep_create_rsp_bad_request(rsp);
    }

    snep = (struct snep*)info;
    sneplen = be32_to_cpu(snep->len);

    if (!(sneplen < (UINT32_MAX-sizeof(*snep))) ||
         (sneplen+sizeof(*snep) != len)) {
        NFC_D("SNEP responding 'Excess Data'");
        return snep_create_rsp_excess_data(rsp);
    }
    return process_msg(snep, dl, rsp);
}
