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
#include "bswap.h"
#include "snep.h"

size_t
snep_create_msg(struct snep* snep, enum snep_msg msg, uint32_t len)
{
    assert(snep);

    snep->ver.major = SNEP_VERSION_MAJOR;
    snep->ver.minor = SNEP_VERSION_MINOR;
    snep->msg = msg;
    snep->len = cpu_to_be32(len);

    return sizeof(*snep) + len;
}

size_t
snep_create_req_put(struct snep* snep, uint32_t len)
{
    return snep_create_msg(snep, SNEP_REQ_PUT, len);
}

size_t
snep_create_rsp_success(struct snep* snep, uint32_t len)
{
    return snep_create_msg(snep, SNEP_RSP_SUCCESS, len);
}

size_t
snep_create_rsp_excess_data(struct snep* snep)
{
    return snep_create_msg(snep, SNEP_RSP_EXCESS_DATA, 0);
}

size_t
snep_create_rsp_bad_request(struct snep* snep)
{
    return snep_create_msg(snep, SNEP_RSP_BAD_REQUEST, 0);
}

size_t
snep_create_rsp_not_implemented(struct snep* snep)
{
    return snep_create_msg(snep, SNEP_RSP_NOT_IMPLEMENTED, 0);
}

size_t
snep_create_rsp_unsupported_version(struct snep* snep)
{
    return snep_create_msg(snep, SNEP_RSP_UNSUPPORTED_VERSION, 0);
}
