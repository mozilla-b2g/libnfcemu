#include "hw/llcp.h"
#include "hw/goldfish_nfc.h"
#include "hw/nfc-re.h"
#include "hw/nfc.h"
#include "hw/nfc-nci.h"

static size_t
nfc_rf_discovery_ntf_cb(void* data,
                        struct nfc_device* nfc,
                        union nci_packet* ntf)
{
    return nfc_create_rf_discovery_ntf(data, nfc, ntf);
}

static size_t
nfc_rf_intf_activated_ntf_cb(void* data,
                             struct nfc_device* nfc,
                             union nci_packet* ntf)
{
    return nfc_create_rf_intf_activated_ntf(data, nfc, ntf);
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
        /* read remote-endpoint index */
        p = strsep(&args, " ");
        if (!p) {
            control_write(client, "KO: no remote endpoint given\r\n");
            return -1;
        }
        errno = 0;
        size_t i = strtoul(p, NULL, 0);
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

        goldfish_nfc_send_ntf(nfc_rf_discovery_ntf_cb, nfc_res+i);
    } else if (!strcmp(p, "rf_intf_activated")) {
        struct nfc_re *re = NULL;

        /* read remote-endpoint index */
        p = strsep(&args, " ");
        if (p) {
            errno = 0;
            size_t i = strtoul(p, NULL, 0);
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
            re = nfc_res + i;
        }

        /* if re == NULL, active RE will be used */
        goldfish_nfc_send_ntf(nfc_rf_intf_activated_ntf_cb, re);
    } else {
        control_write(client, "KO: invalid operation '%s'\r\n", p);
        return -1;
    }

    return 0;
}

struct nfc_dta_write_param {
  uint8_t connid;
  size_t len;
  const void* data;
};

static size_t
nfc_dta_write_cb(void* data, struct nfc_device* nfc,
                 union nci_packet* packet)
{
  const struct nfc_dta_write_param* param;

  param = data;

  return nfc_create_dta(param->data, param->len, nfc, packet);
}

static int
do_nfc_dta( ControlClient  client, char*  args )
{
    char *p;

    if (!args) {
        return -1;
    }

    /* read notification type */
    p = strsep(&args, " ");

    if (!p) {
        return -1;
    }

    if (!strcmp(p, "s") || !strcmp(p, "send") ||
        !strcmp(p, "w") || !strcmp(p, "wr") || !strcmp(p, "write")) {

        struct nfc_dta_write_param param;

        /* read connection id */
        p = strsep(&args, " ");

        if (!p) {
            return -1;
        }

        errno = 0;
        param.connid = strtoul(p, NULL, 0);

        if (errno || !param.connid || (param.connid > 254)) {
            return -1;
        }

        param.len = strlen(args);
        param.data = args;

        goldfish_nfc_send_dta(nfc_dta_write_cb, &param);
    }

    return 0;
}
