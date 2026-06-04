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
       "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
       "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
       "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
       "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
       "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
       "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
       "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
       "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
       "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
       "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
       "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
       "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
       "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
       "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
       "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
       "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
       "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
       "rqXRfboQnoZsG4q5WTP468SQvvG5\n"
       "-----END CERTIFICATE-----\"");

    /*
     * Store AWS IoT device certificate.
     *
     * Use your file:
     * nrf-try-1.cert.pem
     */
    run_at("AT%CMNG=0,42,1,\"-----BEGIN CERTIFICATE-----\n"
           "MIIDWTCCAkGgAwIBAgIUeYkIV7g5pZCvzI7o3O5b6SVhgKowDQYJKoZIhvcNAQEL\n"
           "BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n"
           "SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI2MDYwMzIzMzMz\n"
           "MFoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n"
           "ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMLwoeiUWsyD9BOYDPrR\n"
           "wJWLStPNBd+ktkn5D1XEEaZPWO10gT7FLnxr5LbT2hLh+4ey3JLccIITS7y0immo\n"
           "rlE+jq7Ik1Ot0VK2xRBwfpJ7hd1QBn5jHEupQbiFDSVLCpwUO6GMu+SZ+P/Chru5\n"
           "qyVai6BvLyUWLOmgarVQ0Xj0rhYl+/BG0fbwP2SO3KDruH5pD/g/Og+n4mPkoh4Y\n"
           "m739vrts0m7ed5q204V3bFI92HQBGZlhXrslU7fZfYVYK1iCCE4J+Hk/ipfuOewb\n"
           "iSBogro4a8f0TGtjYqQheQABmd+Qy0BSKyNodhjJcsCJjM0BhU5ZNXS9jNzAFuiK\n"
           "0iUCAwEAAaNgMF4wHwYDVR0jBBgwFoAUu6sotRQcGzjpB/ec/YJgYwQA0AwwHQYD\n"
           "VR0OBBYEFDHkig++371MNuouJV+I6UmjU6+/MAwGA1UdEwEB/wQCMAAwDgYDVR0P\n"
           "AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQB8hf5wyyy8EQ45NyvhEUqZ+Nt0\n"
           "LClc+SiX0RNrQI+fNkDnGg6xiyG2EKa7RJ1csLoVZbho3Tzetqg3EfpakQ7Ee+qi\n"
           "iYQzseBmuNELz68J62KyHRNYwSqYUNWpwUbe+FtYKxpgpzxUMMQweIFWshrcXEqk\n"
           "J7uGCgdAl5BAOwf7HsKDHV+JtMyOE1CZlMRX13o/gsL9TT7JN8p+ew6V16wnmyke\n"
           "64Xu/Bhke6Bkay5SScijLvJdX1fQ3ORwhGdFp+2AgKz/g5HJrICFGO6xnhm7C/e0\n"
           "4UO7z2QISbaPy4nfF4SIcJ64K28MDGSFBC7NgnauTzKeY5lnz+Wq/gVIeNky\n"
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
       "MIIEowIBAAKCAQEAwvCh6JRazIP0E5gM+tHAlYtK080F36S2SfkPVcQRpk9Y7XSB\n"
       "PsUufGvkttPaEuH7h7LcktxwghNLvLSKaaiuUT6OrsiTU63RUrbFEHB+knuF3VAG\n"
       "fmMcS6lBuIUNJUsKnBQ7oYy75Jn4/8KGu7mrJVqLoG8vJRYs6aBqtVDRePSuFiX7\n"
       "8EbR9vA/ZI7coOu4fmkP+D86D6fiY+SiHhibvf2+u2zSbt53mrbThXdsUj3YdAEZ\n"
       "mWFeuyVTt9l9hVgrWIIITgn4eT+Kl+457BuJIGiCujhrx/RMa2NipCF5AAGZ35DL\n"
       "QFIrI2h2GMlywImMzQGFTlk1dL2M3MAW6IrSJQIDAQABAoIBABoefcMY5ZWTdIeP\n"
       "juWoZwJieHMqfEf73MTKjyhxrd+3r/WY75xGdTUMi8eHWSfAiaAj8fPnf1eeWD5/\n"
       "fjAr/rc6B4oZq8vBdVN806ihOgRMcqI1j7ktdGQa2Ihto1ZE2LYS6+54WdCGZNt3\n"
       "5++NMAZAPIcRfqKcdwYSjVmf64EmI64X8iAfG5i6WSkllRFTJzkMPzTSV9ohf7g6\n"
       "Ze2/D9MZmb0J88F3Dzk2Wu0DWPaC49pixTlDjYYI3lUEs5DLsaDZyV63AZnERVVv\n"
       "AuIr9I97Nj9wU1xpeOJIrxqKGxisWFDzd2lJEsCG+JvtMhYCA5uzPZVv0kPrzyzt\n"
       "IGslU8ECgYEA5bIUmyi1HuxLCma7UM2xY1usY+4Xwisnh34z3b4slEPixj7XBfb1\n"
       "bHXO3o/Oig9o6lozP10x3GXGam47DDV4zd+mQVGyTddmX6lxPqGCUhdsXVhhXaLc\n"
       "/0etpMkzKx56Cn5CDSxr1Vo71Gyxgupn66me0rIkWPGXnRBkDWx/nnECgYEA2UOh\n"
       "a71M2KAtVGAy7m0YcJe/M3UTEfT2jJVWyzFGrllS82UvAhltlgZz3LjWOAG00jFI\n"
       "sGX4Nd/8Y8Qmzq1j+UEiJfRQbVuA+W/c6u5F3zYBUGxn/CKtFdAFlS3WnN1+UZC3\n"
       "1OIet7I1jHOC9dYR9LOVvOG6BAX1mMgfl09jMPUCgYBjMKS/YDPe17GMG1L2kOzy\n"
       "eCsYuD+gRNx8IV6qei5sFGjArXBTjhLrtHASIk3RxtfKpAsPaOJYR8cIPPPY0NqL\n"
       "JPJ5sJrsDIGT++sAXeSWKLFOGhVpxyPiRYTR7WTgUPfowh8p6y3h42aE1C9P1oPr\n"
       "+8KvP+h8VE/hrqtWNK1jsQKBgG/WD1aNR+xOb2b6ad0vTSBQDHzpa9qXdP4itV28\n"
       "zLxcgIWHdS4odq9+PM/GWYbU4gY8lkUvCBh7ZsjYJH3I5Shd7b0JyQixzG8ZtOc9\n"
       "pbvjacPDObehBeWvgeAri0iN/0LvMqGjj9pRIp1crHHtMqr6cj6bFwyRIL33bcOO\n"
       "eumtAoGBAJ/b9rxSGXMDXQmssfjAi1IGfwsP0ZbkkuIOCNTG2lbDth0LHUh2e6oS\n"
       "ScOWTCD9p7yUwKznvZpCkKqYFMjs0HMwBxx904K93o/I+81OHa4VZAqDcInhQoFH\n"
       "ELzns14os4BZ9fEjUIZtVJFAY44yTsL4D/1qt0n+Y26Jpd/xY+Nw\n"
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