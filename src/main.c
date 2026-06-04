#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include <stddef.h>
#include <modem/nrf_modem_lib.h>

/* Manual declaration because your SDK setup did not expose the header */
int nrf_modem_at_cmd(void *buf, size_t len, const char *fmt, ...);

/*
 * TLS_SEC_TAG is NOT a folder and NOT a filename.
 * It is just an ID number inside the nRF9160 modem.
 *
 * Under tag 42 we store:
 *   42,0 = Amazon Root CA
 *   42,1 = Device certificate
 *   42,2 = Private key
 */
#define TLS_SEC_TAG 42

static void run_at(const char *cmd)
{
    char resp[512];
    int err;

    memset(resp, 0, sizeof(resp));
    err = nrf_modem_at_cmd(resp, sizeof(resp), "%s", cmd);

    if (err) {
        printk("\nCMD:\n%s\nERRCODE: %d\n", cmd, err);
        return;
    }

    printk("\nCMD:\n%s\nRESP:\n%s\n", cmd, resp);
}

static void provision_aws_certificates(void)
{
    printk("\n=== AWS Certificate Provisioning Start ===\n");

    /*
     * Put modem in offline mode before changing credentials.
     */
    run_at("AT+CFUN=4");
    k_sleep(K_SECONDS(2));

    /*
     * Optional cleanup:
     * Delete old credentials from slot 42.
     * If they do not exist, errors are okay.
     */
    run_at("AT%CMNG=3,42,0");  /* Delete CA certificate */
    run_at("AT%CMNG=3,42,1");  /* Delete device certificate */
    run_at("AT%CMNG=3,42,2");  /* Delete private key */

    /*
     * Store Amazon Root CA 1.
     *
     * Replace PASTE_AMAZON_ROOT_CA_BODY_HERE with the middle part of:
     *
     * -----BEGIN CERTIFICATE-----
     * ...
     * -----END CERTIFICATE-----
     */
    run_at("AT%CMNG=0,42,0,\"-----BEGIN CERTIFICATE-----\n"
       "PASTE CONTENT HERE\n"
       "-----END CERTIFICATE-----\"");

    /*
     * Store AWS IoT device certificate.
     *
     * Use your file:
     * nrf-try-1.cert.pem
     */
    run_at("AT%CMNG=0,42,1,\"-----BEGIN CERTIFICATE-----\n"
           "PASTE CONTENT HERE\n"
           "-----END CERTIFICATE-----\"");

    /*
     * Store AWS IoT private key.
     *
     * Use your file:
     * nrf-try-1.private.key
     *
     * IMPORTANT:
     * Your key may start with either:
     * -----BEGIN RSA PRIVATE KEY-----
     *
     * or:
     * -----BEGIN PRIVATE KEY-----
     *
     * Use exactly what your file has.
     */
    run_at("AT%CMNG=0,42,2,\"-----BEGIN RSA PRIVATE KEY-----\n"
       "PASTE CONTENT HERE\n"
       "-----END RSA PRIVATE KEY-----\"");

    /*
     * Verify credentials stored under tag 42.
     * You want to see entries for:
     *   42,0
     *   42,1
     *   42,2
     */
    run_at("AT%CMNG=1,42");

    printk("\n=== AWS Certificate Provisioning End ===\n");
}

int main(void)
{
    int err;

    printk("=== nRF9160 AWS Certificate Provisioning Test ===\n");

    err = nrf_modem_lib_init();
    if (err) {
        printk("nrf_modem_lib_init() failed: %d\n", err);
        return 0;
    }

    printk("Modem initialized.\n");

    run_at("AT");

    provision_aws_certificates();

    printk("\nDone. If AT%%CMNG=1,42 shows 42,0 / 42,1 / 42,2, provisioning worked.\n");
    printk("After this succeeds once, remove provision_aws_certificates() from main.\n");

    while (1) {
        k_sleep(K_SECONDS(5));
    }

    return 0;
}
