#include "hw/llcp.h"
#include "hw/goldfish_nfc.h"
#include "hw/ndef.h"
#include "hw/nfc-re.h"
#include "hw/nfc.h"
#include "hw/nfc-nci.h"
#include "hw/snep.h"

struct nfc_ndef_record_param {
    unsigned long flags;
    enum ndef_tnf tnf;
    const char* type;
    const char* payload;
    const char* id;
};

#define NFC_NDEF_PARAM_RECORD_INIT(_rec) \
    _rec = { \
        .flags = 0, \
        .tnf = 0, \
        .type = NULL, \
        .payload = NULL, \
        .id = NULL \
    }

ssize_t
build_ndef_msg(ControlClient client,
               const struct nfc_ndef_record_param* record, size_t nrecords,
               uint8_t* buf, size_t len)
{
    size_t off;
    size_t i;

    assert(record || !nrecords);
    assert(buf || !len);

    off = 0;

    for (i = 0; i < nrecords; ++i, ++record) {
        size_t idlen;
        uint8_t flags;
        struct ndef_rec* ndef;
        ssize_t res;

        idlen = strlen(record->id);

        flags = record->flags |
                ( NDEF_FLAG_MB * (!i) ) |
                ( NDEF_FLAG_ME * (i+1 == nrecords) ) |
                ( NDEF_FLAG_IL * (!!idlen) );

        ndef = (struct ndef_rec*)(buf + off);
        off += ndef_create_rec(ndef, flags, record->tnf, 0, 0, 0);

        /* decode type */
        res = decode_base64(record->type, strlen(record->type),
                            buf+off, len-off);
        if (res < 0) {
            return -1;
        }
        ndef_rec_set_type_len(ndef, res);
        off += res;

        /* decode payload */
        res = decode_base64(record->payload, strlen(record->payload),
                            buf+off, len-off);
        if (res < 0) {
            return -1;
        } else if ((res > 255) && (flags & NDEF_FLAG_SR)) {
            control_write(client,
                          "KO: NDEF flag SR set for long payload of %zu bytes",
                          res);
            return -1;
        }
        ndef_rec_set_payload_len(ndef, res);
        off += res;

        if (flags & NDEF_FLAG_IL) {
            /* decode id */
            res = decode_base64(record->id, strlen(record->id),
                                buf+off, len-off);
            if (res < 0) {
                return -1;
            }
            ndef_rec_set_id_len(ndef, res);
            off += res;
        }
    }
    return off;
}

struct nfc_snep_param {
    ControlClient client;
    long dsap;
    long ssap;
    size_t nrecords;
    struct nfc_ndef_record_param record[4];
};

#define NFC_SNEP_PARAM_INIT(_client) \
    { \
        .client = (_client), \
        .dsap = LLCP_SAP_LM, \
        .ssap = LLCP_SAP_LM, \
        .nrecords = 0, \
        .record = { \
            NFC_NDEF_PARAM_RECORD_INIT([0]), \
            NFC_NDEF_PARAM_RECORD_INIT([1]), \
            NFC_NDEF_PARAM_RECORD_INIT([2]), \
            NFC_NDEF_PARAM_RECORD_INIT([3]) \
        } \
    }

static ssize_t
create_snep_cp(void *data, size_t len, struct snep* snep)
{
    const struct nfc_snep_param* param;
    ssize_t res;

    param = data;
    assert(param);

    res = build_ndef_msg(param->client, param->record, param->nrecords,
                         snep->info, len-sizeof(*snep));
    if (res < 0) {
        return -1;
    }
    return snep_create_req_put(snep, res);
}

static ssize_t
nfc_send_snep_put_cb(void* data,
                     struct nfc_device* nfc,
                     size_t maxlen, union nci_packet* ntf)
{
    struct nfc_snep_param* param;
    ssize_t res;

    param = data;
    assert(param);

    if (!nfc->active_re) {
        control_write(param->client, "KO: no active remote endpoint\n");
        return -1;
    }
    if ((param->dsap < 0) && (param->ssap < 0)) {
        param->dsap = nfc->active_re->last_dsap;
        param->ssap = nfc->active_re->last_ssap;
    }
    res = nfc_re_send_snep_put(nfc->active_re, param->dsap, param->ssap,
                               create_snep_cp, data);
    if (res < 0) {
        control_write(param->client, "KO: 'snep put' failed\r\n");
        return -1;
    }
    return res;
}

static ssize_t
nfc_recv_process_ndef_cb(void* data, size_t len, const struct ndef_rec* ndef)
{
    const struct nfc_snep_param* param;
    ssize_t remain;
    char base64[3][512];

    param = data;
    assert(param);

    remain = len;

    control_write(param->client, "[");

    while (remain) {
        size_t tlen, plen, ilen, reclen;

        if (remain < sizeof(*ndef)) {
            return -1; /* too short */
        }
        tlen = encode_base64(ndef_rec_const_type(ndef),
                             ndef_rec_type_len(ndef),
                             base64[0], sizeof(base64[0]));
        plen = encode_base64(ndef_rec_const_payload(ndef),
                             ndef_rec_payload_len(ndef),
                             base64[1], sizeof(base64[1]));
        ilen = encode_base64(ndef_rec_const_id(ndef), ndef_rec_id_len(ndef),
                             base64[2], sizeof(base64[2]));

        /* print NDEF message in JSON format */
        control_write(param->client,
                      "{\"tnf\": %d,"
                      " \"type\": \"%.*s\","
                      " \"payload\": \"%.*s\","
                      " \"id\": \"%.*s\"}",
                      ndef->flags & NDEF_TNF_BITS,
                      tlen, base64[0], plen, base64[1], ilen, base64[2]);

        /* advance record */
        reclen = ndef_rec_len(ndef);
        remain -= reclen;
        ndef = (const struct ndef_rec*)(((const unsigned char*)ndef) + reclen);
        if (remain) {
          control_write(param->client, ","); /* more to come */
        }
    }
    control_write(param->client, "]\r\n");
    return 0;
}

static ssize_t
nfc_recv_snep_put_cb(void* data,  struct nfc_device* nfc)
{
    struct nfc_snep_param* param;
    ssize_t res;

    param = data;
    assert(param);

    if (!nfc->active_re) {
        control_write(param->client, "KO: no active remote endpoint\r\n");
        return -1;
    }
    if ((param->dsap < 0) && (param->ssap < 0)) {
        param->dsap = nfc->active_re->last_dsap;
        param->ssap = nfc->active_re->last_ssap;
    }
    res = nfc_re_recv_snep_put(nfc->active_re, param->dsap, param->ssap,
                               nfc_recv_process_ndef_cb, data);
    if (res < 0) {
        control_write(param->client, "KO: 'snep put' failed\r\n");
        return -1;
    }
    return 0;
}

static int
parse_sap(ControlClient client, const char* field,
          char** args, long* sap, int can_autodetect)
{
    const char* p;

    assert(args);
    assert(sap);

    p = strsep(args, " ");
    if (!p) {
        control_write(client, "KO: no %s given\r\n", field);
        return -1;
    }
    errno = 0;
    *sap = strtol(p, NULL, 0);
    if (errno) {
        control_write(client,
                      "KO: invalid %s '%s', error %d(%s)\r\n",
                      field, p, errno, strerror(errno));
        return -1;
    }
    if (((*sap == -1) && !can_autodetect) ||
         (*sap < -1) || !(*sap < LLCP_NUMBER_OF_SAPS)) {
        control_write(client, "KO: invalid %s '%ld'\r\n",
                      field, *sap);
        return -1;
    }
    return 0;
}

/* Each record is given by its flag bits, TNF value, type,
 * payload, and id. Id is optional. Type, payload, and id
 * are given in base64url encoding.
 */
static int
parse_ndef_rec(ControlClient client, char** args,
               struct nfc_ndef_record_param* record)
{
    const char* p;

    assert(args);
    assert(record);

    /* read opening bracket */
    p = strsep(args, "[");
    if (!p) {
        control_write(client, "KO: no NDEF record given\r\n");
        return -1;
    }
    /* read flags */
    p = strsep(args, " ,");
    if (!p) {
        control_write(client, "KO: no NDEF flags given\r\n");
        return -1;
    }
    errno = 0;
    record->flags = strtoul(p, NULL, 0);
    if (errno) {
        control_write(client,
                      "KO: invalid NDEF flags '%s', error %d(%s)\r\n",
                      p, errno, strerror(errno));
        return -1;
    }
    if (record->flags & ~NDEF_FLAG_BITS) {
        control_write(client, "KO: invalid NDEF flags '%u'\r\n",
                      record->flags);
        return -1;
    }
    /* read TNF */
    p = strsep(args, " ,");
    if (!p) {
        control_write(client, "KO: no NDEF TNF given\r\n");
        return -1;
    }
    errno = 0;
    record->tnf = strtoul(p, NULL, 0);
    if (errno) {
        control_write(client,
                      "KO: invalid NDEF TNF '%s', error %d(%s)\r\n",
                      p, errno, strerror(errno));
        return -1;
    }
    if (!(record->tnf < NDEF_NUMBER_OF_TNFS)) {
        control_write(client, "KO: invalid NDEF TNF '%u'\r\n",
                      record->tnf);
        return -1;
    }
    /* read type */
    record->type = strsep(args, " ,");
    if (!record->type) {
        control_write(client, "KO: no NDEF type given\r\n");
        return -1;
    }
    if (!strlen(record->type)) {
        control_write(client, "KO: empty NDEF type\r\n");
        return -1;
    }
    /* read payload */
    record->payload = strsep(args, " ,");
    if (!record->payload) {
        control_write(client, "KO: no NDEF payload given\r\n");
        return -1;
    }
    if (!strlen(record->payload)) {
        control_write(client, "KO: empty NDEF payload\r\n");
        return -1;
    }
    /* read id; might by empty */
    record->id = strsep(args, "]");
    if (!record->id) {
        control_write(client, "KO: no NDEF ID given\r\n");
        return -1;
    }
    return 0;
}

static ssize_t
parse_ndef_msg(ControlClient client, char** args, size_t nrecs,
               struct nfc_ndef_record_param* rec)
{
    size_t i;

    assert(args);

    for (i = 0; i < nrecs && *args && strlen(*args); ++i) {
        if (parse_ndef_rec(client, args, rec+i) < 0) {
          return -1;
        }
    }
    if (*args && strlen(*args)) {
        control_write(client,
                      "KO: invalid characters near EOL: %s\r\n",
                      *args);
        return -1;
    }
    return i;
}

static int
parse_re_index(ControlClient client, char** args, unsigned long nres,
               unsigned long* i)
{
    const char* p;

    assert(args);
    assert(i);

    p = strsep(args, " ");
    if (!p) {
        control_write(client, "KO: no remote endpoint given\r\n");
        return -1;
    }
    errno = 0;
    *i = strtoul(p, NULL, 0);
    if (errno) {
        control_write(client,
                      "KO: invalid remote endpoint '%s', error %d(%s)\r\n",
                      p, errno, strerror(errno));
        return -1;
    }
    if (!(*i < nres)) {
        control_write(client, "KO: unknown remote endpoint %zu\r\n", *i);
        return -1;
    }
    return 0;
}

static int
parse_nci_ntf_type(ControlClient client, char** args, unsigned long* ntype)
{
    const char* p;

    assert(args);

    p = strsep(args, " ");
    if (!p) {
        control_write(client, "KO: no discover notification type given\r\n");
        return -1;
    }
    errno = 0;
    *ntype = strtoul(p, NULL, 0);
    if (errno) {
        control_write(client,
                      "KO: invalid discover notification type '%s', error %d(%s)\r\n",
                      p, errno, strerror(errno));
        return -1;
    }
    if (!(*ntype < NUMBER_OF_NCI_NOTIFICATION_TYPES)) {
        control_write(client, "KO: unknown discover notification type %zu\r\n", *ntype);
        return -1;
    }
    return 0;
}

static int
parse_rf_index(ControlClient client, char** args, unsigned long* rf)
{
    const char* p;

    assert(client);
    assert(args);
    assert(rf);

    p = strsep(args, " ");
    errno = 0;
    *rf = strtol(p, NULL, 0);
    if (errno) {
        control_write(client,
                      "KO: invalid rf index '%s', error %d(%s)\r\n",
                      p, errno, strerror(errno));
        return -1;
    }
    if (*rf < -1 || *rf >= NUMBER_OF_SUPPORTED_NCI_RF_INTERFACES) {
        control_write(client, "KO: unknown rf index %d\r\n", *rf);
        return -1;
    }
    return 0;
}

static int
do_nfc_snep( ControlClient  client, char*  args )
{
    char *p;

    if (!args) {
        control_write(client, "KO: no arguments given\r\n");
        return -1;
    }

    p = strsep(&args, " ");
    if (!p) {
        control_write(client, "KO: no operation given\r\n");
        return -1;
    }
    if (!strcmp(p, "put")) {
        ssize_t nrecords;
        struct nfc_snep_param param = NFC_SNEP_PARAM_INIT(client);

        /* read DSAP */
        if (parse_sap(client, "DSAP", &args, &param.dsap, 1) < 0) {
            return -1;
        }
        /* read SSAP */
        if (parse_sap(client, "SSAP", &args, &param.ssap, 1) < 0) {
            return -1;
        }
        /* The emulator supports up to 4 records per NDEF
         * message. If no records are given, the emulator
         * will print the current content of the LLCP data-
         * link buffer.
         */
        nrecords = parse_ndef_msg(client, &args, ARRAY_SIZE(param.record),
                                  param.record);
        if (nrecords < 0) {
            return -1;
        }
        param.nrecords = nrecords;
        if (param.nrecords) {
            /* put SNEP request onto SNEP server */
            if (goldfish_nfc_send_dta(nfc_send_snep_put_cb, &param) < 0) {
                /* error message generated in create function */
                return -1;
            }
        } else {
            /* put SNEP request onto SNEP server */
            if (goldfish_nfc_recv_dta(nfc_recv_snep_put_cb, &param) < 0) {
                /* error message generated in create function */
                return -1;
            }
        }
    } else {
        control_write(client, "KO: invalid operation '%s'\r\n", p);
        return -1;
    }

    return 0;
}

struct nfc_ntf_param {
    ControlClient client;
    struct nfc_re* re;
    unsigned long ntype;
    long rf;
};

#define NFC_NTF_PARAM_INIT(_client) \
    { \
      .client = (_client), \
      .re = NULL, \
      .ntype = 0, \
      .rf = -1 \
    }

static ssize_t
nfc_rf_discovery_ntf_cb(void* data,
                        struct nfc_device* nfc, size_t maxlen,
                        union nci_packet* ntf)
{
    ssize_t res;
    const struct nfc_ntf_param* param = data;
    res = nfc_create_rf_discovery_ntf(param->re, param->ntype, nfc, ntf);
    if (res < 0) {
        control_write(param->client, "KO: rf_discover_ntf failed\r\n");
        return -1;
    }
    return res;
}

static ssize_t
nfc_rf_intf_activated_ntf_cb(void* data,
                             struct nfc_device* nfc, size_t maxlen,
                             union nci_packet* ntf)
{
    size_t res;
    struct nfc_ntf_param* param = data;
    if (!param->re) {
        if (!nfc->active_re) {
            control_write(param->client, "KO: no active remote-endpoint\n");
            return -1;
        }
        param->re = nfc->active_re;
    }
    nfc_clear_re(param->re);
    if (nfc->active_rf) {
        // Already select an active rf interface,so do nothing.
    } else if (param->rf == -1) {
        // Auto select active rf interface based on remote-endpoint protocol.
        enum nci_rf_interface iface;

        switch(param->re->rfproto) {
            case NCI_RF_PROTOCOL_T1T:
            case NCI_RF_PROTOCOL_T2T:
            case NCI_RF_PROTOCOL_T3T:
                iface = NCI_RF_INTERFACE_FRAME;
                break;
            case NCI_RF_PROTOCOL_NFC_DEP:
                iface = NCI_RF_INTERFACE_NFC_DEP;
                break;
            case NCI_RF_PROTOCOL_ISO_DEP:
                iface = NCI_RF_INTERFACE_ISO_DEP;
                break;
            default:
                control_write(param->client,
                              "KO: invalid remote-endpoint protocol '%d'\n",
                              param->re->rfproto);
                return -1;
        }

        nfc->active_rf = nfc_find_rf_by_rf_interface(nfc, iface);
        if (!nfc->active_rf) {
            control_write(param->client, "KO: no active rf interface\r\n");
            return -1;
        }
    } else {
        nfc->active_rf = nfc->rf + param->rf;
    }
    res = nfc_create_rf_intf_activated_ntf(param->re, nfc, ntf);
    if (res < 0) {
        control_write(param->client, "KO: rf_intf_activated_ntf failed\r\n");
        return -1;
    }
    return res;
}

static int
do_nfc_nci( ControlClient  client, char*  args )
{
    char *p;

    if (!args) {
        control_write(client, "KO: no arguments given\r\n");
        return -1;
    }

    /* read notification type */
    p = strsep(&args, " ");
    if (!p) {
        control_write(client, "KO: no operation given\r\n");
        return -1;
    }
    if (!strcmp(p, "rf_discover_ntf")) {
        unsigned long i;
        struct nfc_ntf_param param = NFC_NTF_PARAM_INIT(client);
        /* read remote-endpoint index */
        if (parse_re_index(client, &args, ARRAY_SIZE(nfc_res), &i) < 0) {
            return -1;
        }
        param.re = nfc_res + i;

        /* read discover notification type */
        if (parse_nci_ntf_type(client, &args, &param.ntype) < 0) {
            return -1;
        }

        /* generate RF_DISCOVER_NTF */
        if (goldfish_nfc_send_ntf(nfc_rf_discovery_ntf_cb, &param) < 0) {
            /* error message generated in create function */
            return -1;
        }
    } else if (!strcmp(p, "rf_intf_activated_ntf")) {
        struct nfc_ntf_param param = NFC_NTF_PARAM_INIT(client);
        if (args && *args) {
            unsigned long i;
            /* read remote-endpoint index */
            if (parse_re_index(client, &args, ARRAY_SIZE(nfc_res), &i) < 0) {
                return -1;
            }
            param.re = nfc_res + i;

            if (args && *args) {
                /* read rf interface index */
                if (parse_rf_index(client, &args, &param.rf) < 0) {
                    return -1;
                }
            } else {
                param.rf = -1;
            }
        } else {
            param.re = NULL;
            param.rf = -1;
        }
        /* generate RF_INTF_ACTIVATED_NTF; if param.re == NULL,
         * active RE will be used */
        if (goldfish_nfc_send_ntf(nfc_rf_intf_activated_ntf_cb, &param) < 0) {
            /* error message generated in create function */
            return -1;
        }
    } else {
        control_write(client, "KO: invalid operation '%s'\r\n", p);
        return -1;
    }

    return 0;
}

struct nfc_llcp_param {
    ControlClient client;
    long dsap;
    long ssap;
};

#define NFC_LLCP_PARAM_INIT(_client) \
    { \
        .client = (_client), \
        .dsap = 0, \
        .ssap = 0 \
    }

static ssize_t
nfc_llcp_connect_cb(void* data, struct nfc_device* nfc, size_t maxlen,
                    union nci_packet* packet)
{
    struct nfc_llcp_param* param = data;
    ssize_t res;

    if (!nfc->active_re) {
        control_write(param->client, "KO: no active remote endpoint\n");
        return -1;
    }
    if ((param->dsap < 0) && (param->ssap < 0)) {
        param->dsap = nfc->active_re->last_dsap;
        param->ssap = nfc->active_re->last_ssap;
    }
    if (!param->dsap) {
        control_write(param->client, "KO: DSAP is 0\r\n");
        return -1;
    }
    if (!param->ssap) {
        control_write(param->client, "KO: SSAP is 0\r\n");
        return -1;
    }
    res = nfc_re_send_llcp_connect(nfc->active_re, param->dsap, param->ssap);
    if (res < 0) {
        control_write(param->client, "KO: LLCP connect failed\r\n");
        return -1;
    }
    return 0;
}

static int
do_nfc_llcp( ControlClient  client, char*  args )
{
    char *p;

    if (!args) {
        control_write(client, "KO: no arguments given\r\n");
        return -1;
    }

    p = strsep(&args, " ");
    if (!p) {
        control_write(client, "KO: no operation given\r\n");
        return -1;
    }
    if (!strcmp(p, "connect")) {
        struct nfc_llcp_param param = NFC_LLCP_PARAM_INIT(client);

        /* read DSAP */
        if (parse_sap(client, "DSAP", &args, &param.dsap, 1) < 0) {
            return -1;
        }
        /* read SSAP */
        if (parse_sap(client, "SSAP", &args, &param.ssap, 1) < 0) {
            return -1;
        }
        if (goldfish_nfc_send_dta(nfc_llcp_connect_cb, &param) < 0) {
            /* error message generated in create function */
            return -1;
        }
    } else {
        control_write(client, "KO: invalid operation '%s'\r\n", p);
        return -1;
    }

    return 0;
}
