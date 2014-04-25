#include "hw/llcp.h"
#include "hw/goldfish_nfc.h"
#include "hw/nfc-re.h"
#include "hw/nfc.h"
#include "hw/nfc-nci.h"

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
        size_t i;
        struct nfc_ntf_param param = NFC_NTF_PARAM_INIT(client);
        /* read remote-endpoint index */
        p = strsep(&args, " ");
        if (!p) {
            control_write(client, "KO: no remote endpoint given\r\n");
            return -1;
        }
        errno = 0;
        i = strtoul(p, NULL, 0);
        if (errno) {
            control_write(client,
                          "KO: invalid remote endpoint '%s', error %d(%s)\r\n",
                          p, errno, strerror(errno));
            return -1;
        }
        if (!(i < sizeof(nfc_res)/sizeof(nfc_res[0])) ) {
            control_write(client, "KO: unknown remote endpoint %zu\r\n", i);
            return -1;
        }

        /* read discover notification type */
        p = strsep(&args, " ");
        if (!p) {
            control_write(client, "KO: no discover notification type given\r\n");
            return -1;
        }
        errno = 0;
        param.ntype = strtoul(p, NULL, 0);
        if (errno) {
            control_write(client,
                          "KO: invalid discover notification type '%s', error %d(%s)\r\n",
                          p, errno, strerror(errno));
            return -1;
        }
        if (!(param.ntype < NUMBER_OF_NCI_NOTIFICATION_TYPES)) {
            control_write(client, "KO: unknown discover notification type %zu\r\n", param.ntype);
            return -1;
        }
        param.re = nfc_res + i;
        /* generate RF_DISCOVER_NTF */
        if (goldfish_nfc_send_ntf(nfc_rf_discovery_ntf_cb, &param) < 0) {
            /* error message generated in create function */
            return -1;
        }
    } else if (!strcmp(p, "rf_intf_activated_ntf")) {
        struct nfc_ntf_param param = NFC_NTF_PARAM_INIT(client);
        /* read remote-endpoint index */
        p = strsep(&args, " ");
        if (p) {
            size_t i;
            errno = 0;
            i = strtoul(p, NULL, 0);
            if (errno) {
                control_write(client,
                              "KO: invalid remote endpoint '%s', error %d(%s)\r\n",
                              p, errno, strerror(errno));
                return -1;
            }
            if (!(i < sizeof(nfc_res)/sizeof(nfc_res[0]))) {
                control_write(client, "KO: unknown remote endpoint %zu\r\n", i);
                return -1;
            }
            param.re = nfc_res + i;

            /* read rf interface index */
            p = strsep(&args, " ");
            if (!p) {
                param.rf = -1;
            } else {
                errno = 0;
                param.rf = strtol(p, NULL, 0);
                if (errno) {
                    control_write(client,
                                  "KO: invalid rf index '%s', error %d(%s)\r\n",
                                  p, errno, strerror(errno));
                    return -1;
                }
                if (param.rf < -1 ||
                    param.rf >= NUMBER_OF_SUPPORTED_NCI_RF_INTERFACES) {
                    control_write(client, "KO: unknown rf index %d\r\n", param.rf);
                    return -1;
                }
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
