#include "hw/llcp.h"
#include "hw/goldfish_nfc.h"
#include "hw/nfc-re.h"
#include "hw/nfc.h"
#include "hw/nfc-nci.h"

struct nfc_ntf_param {
    ControlClient client;
    struct nfc_re* re;
};

#define NFC_NTF_PARAM_INIT(_client) \
    { \
      .client = (_client), \
      .re = NULL \
    }

static ssize_t
nfc_rf_discovery_ntf_cb(void* data,
                        struct nfc_device* nfc, size_t maxlen,
                        union nci_packet* ntf)
{
    ssize_t res;
    const struct nfc_ntf_param* param = data;
    res = nfc_create_rf_discovery_ntf(param->re, nfc, ntf);
    if (res < 0) {
        control_write(param->client, "KO: rf_discover failed\r\n");
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
            control_write(param->client, "KO: no active remote endpoint\n");
            return -1;
        }
        param->re = nfc->active_re;
    }
    res = nfc_create_rf_intf_activated_ntf(param->re, nfc, ntf);
    if (res < 0) {
        control_write(param->client, "KO: rf_intf_activated failed\r\n");
        return -1;
    }
    return res;
}

static int
do_nfc_ntf( ControlClient  client, char*  args )
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
    if (!strcmp(p, "rf_discover")) {
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
        param.re = nfc_res + i;
        /* generate RF_DISCOVER_NTF */
        if (goldfish_nfc_send_ntf(nfc_rf_discovery_ntf_cb, &param) < 0) {
            /* error message generated in create function */
            return -1;
        }
    } else if (!strcmp(p, "rf_intf_activated")) {
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
        } else {
            param.re = NULL;
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
