/*
 * wpa_supplicant/hostapd / Empty OS specific functions
 * Copyright (c) 2005-2006, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This file can be used as a starting point when adding a new OS target. The
 * functions here do not really work as-is since they are just empty or only
 * return an error value. os_internal.c can be used as another starting point
 * or reference since it has example implementation of many of these functions.
 */

#include <common/bk_include.h>
#include "bk_fake_clock.h"

#include "includes.h"
#include "utils/os.h"
#include <components/system.h>
#ifdef IEEE8021X_EAPOL
#include "xfile.h"
#endif

#ifdef IEEE8021X_EAPOL_DEMO
// ~/demoCA/certs/client.cer @ BSW-MINI
const char *client_cer =
"-----BEGIN CERTIFICATE-----\n"
"MIIDZTCCAk0CFC/zE6H7DRbvkAIsbx5xIEBzVIYlMA0GCSqGSIb3DQEBBQUAMG8x\n"
"CzAJBgNVBAYTAkNOMRMwEQYDVQQIDApteXByb3ZpbmNlMQ8wDQYDVQQHDAZteWNp\n"
"dHkxFzAVBgNVBAoMDm15b3JnYW5pemF0aW9uMRAwDgYDVQQLDAdteWdyb3VwMQ8w\n"
"DQYDVQQDDAZteW5hbWUwHhcNMjEwNzE1MDIyMTE3WhcNMjIwNzE1MDIyMTE3WjBv\n"
"MQswCQYDVQQGEwJDTjETMBEGA1UECAwKbXlwcm92aW5jZTEPMA0GA1UEBwwGbXlj\n"
"aXR5MRcwFQYDVQQKDA5teW9yZ2FuaXphdGlvbjEQMA4GA1UECwwHbXlncm91cDEP\n"
"MA0GA1UEAwwGbXluYW1lMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA\n"
"tr7PbWNXEvklyMS54zbqqy4egIgXQ5PrYk1jkeLxwAuRY8ZINjK60fl/7vk5yA0U\n"
"XdKCQf2oxncN2gRKyVriJ53a+9w6DfHpkofDsPDUf+jK32WKNtgoE1VLkrV/vK/t\n"
"Fah1x08syjo6I4MtftokU33zV2GfBoz0kiR+H22k3lMLt6JaaksDG53Pk41NdeEN\n"
"/ve7D2btWLa9lIz2xyIvZhRRqyllZvRcH3MmM2l5kiBbsLAqnMR+It3b9B3gN4HL\n"
"BlyKvP9576/NbgA4p7BTlDiqBTr8lO8wksrt4rwvjfrxY7zgn4mivfe10yR1zq+C\n"
"44qMq/BxVmKtWlB9l5VX+QIDAQABMA0GCSqGSIb3DQEBBQUAA4IBAQALUh27eH+h\n"
"SzUMeYmxn0/A/wyUrdWR47FJMYnID6piVnLoCTIWgJnjo2WK12Ec7RWB+hjsh+DD\n"
"LBgshhXsQ1590ngqOtgalq6AJChv2geg9AL41JT5uK3BaOkcGh1iCzmsS+Fd36Yv\n"
"xY3a7hFIoJQP4JCAxmAqn0Xa2b4aGUU3Fy4q6aSMXUmufc7uwr/IWh2raypSqzkM\n"
"vWBCGlWS08p+zHTj1AZPQ4T+mCl3P56AVr0/qzsx6JZRico9HEAPd981w/Yw9edX\n"
"WGrFpKIeHjrn6G0O62v/DsGus9O+nOT6MvK989SzcKyqsbYdwYnSXhICnUxZc80c\n"
"ESuSZaM6W+bb\n"
"-----END CERTIFICATE-----\n";

// ~/demoCA/private/client-key.pem @ BSW-MINI
const char *client_key_pem =
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpAIBAAKCAQEAtr7PbWNXEvklyMS54zbqqy4egIgXQ5PrYk1jkeLxwAuRY8ZI\n"
"NjK60fl/7vk5yA0UXdKCQf2oxncN2gRKyVriJ53a+9w6DfHpkofDsPDUf+jK32WK\n"
"NtgoE1VLkrV/vK/tFah1x08syjo6I4MtftokU33zV2GfBoz0kiR+H22k3lMLt6Ja\n"
"aksDG53Pk41NdeEN/ve7D2btWLa9lIz2xyIvZhRRqyllZvRcH3MmM2l5kiBbsLAq\n"
"nMR+It3b9B3gN4HLBlyKvP9576/NbgA4p7BTlDiqBTr8lO8wksrt4rwvjfrxY7zg\n"
"n4mivfe10yR1zq+C44qMq/BxVmKtWlB9l5VX+QIDAQABAoIBAD76CM/AsUw3kj7b\n"
"FNhpn3t4fEh9Hc9LFY4d71diJz+wfN0ugsoyt1EPM3huaOw5HbCrj/of6V7jvmFI\n"
"o2E6sb9HViT/ov4VRRIbOWQryn9+ARP1kSkPWtg2x8F6TNgApKqXEGesMFpMsKZ2\n"
"WWJi2KvxNsABQ2VlhA8tBk3cYjm/PaV5cgfGXmjn2y1DGeND+IQIAawI+zHHDbVp\n"
"4LxCvyHIC9umL5R95fagqt2fHERA5XCrtQ/oGiED3WRwlMT+ljrW5J1XJ18aiIny\n"
"YgGWMYM/Iq565CbIIzgtU3pYwb0xCDilRL40e38fwsZ/nUe6A0a1c1mfMuLlVlNX\n"
"XWTwrIECgYEA3c1mq3Y9XV9e7jJdzY20lVZ6RWtjdp28WVOGoBzuZPcWBy0VVcXm\n"
"ZyYCakMwUnOxRWVlvXE4bbIt9sPXoguZl2ZFdVrhqndZ+Eh5M998oux9rzhf1vpN\n"
"rd/a8oC/4iM1Y/ssdOG3IPaLEkOCgCphJOFcwZEnjcIW7V4F0GnGz9ECgYEA0uvR\n"
"qW5DJXRbN4M7jAf7MH0McSmzsnT+RiSfwWeR6WrXu8nxVugv8f+n9rrBrBuZLWCC\n"
"EqF/L/M1jW4M2gJiD61oSDH6wTJJTNmBNzkSuhS1aj/3pauKFMfD0ESuma3jeNPs\n"
"XZn3nviR4BkfCxVs+Lj1SbccN8NPjlSpnBf8d6kCgYAQ+d+Zz/ryYpJJeOFlEit4\n"
"Ip5JIecC9R+tVo8/QfedEqxE8FZpsjPoQSh+KZZROTmgLhqTXU8MVk7eTC784BZp\n"
"WBKI4OPUwD3KmwFrzvcjISadepsbQR0g6blARkppbHhxpo/Igl4Zz8TlKPWcCaJV\n"
"yEnpJsr4Npq6oC+hu/bTgQKBgQDEe8+9cSz16DXNOeAv64NxQRxlEr8GgHwa5aU4\n"
"6v1/HvvHQTEkj3mSi2Fxs7oVVURVhDDHoFA2X+9ER/+K22t0RUr+7ig0715fS8NW\n"
"qUErvXzedKaDW5BvGUTtb/3ixZ1BAtKOw45sNgzWYTCNB7GS5eICgKS1wakL4wx5\n"
"j9KtuQKBgQCf3hkQ1wUDKT0DLKVm034DobvNH45PkNfkUjFb0XWVRiSbbl5vCqsl\n"
"giYPX3/+YXTSpcSOxbPXlc13JSgbh1BRoe8k2q/InaFbgIMEL56/dHh2ch/5SBfm\n"
"s2j8V1c5NNsEmxZZDaTuyjVHM/xRxScUI7jTBXAjGABGwquTeC4xEA==\n"
"-----END RSA PRIVATE KEY-----\n";

////////////////// Aptilo demo key pair ///////////////////////////
const char *beken_iot_1_key =
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpAIBAAKCAQEA0tIJrvzDItZly3zdktL0LQTabQi02RwECXr0csKwqa9T2Iw4\n"
"nAnpfMsnHX7YVpUwKCfPq99DziiwaHmTteitN9y8PmLg3ULhyR+6RLfYBm0MhgIu\n"
"JBOuVHH8cBV+Sc32nPzP+X/fqFaJkiGcy1Zo0C9lQlxIovGelPb0WiyRrYRCmp+k\n"
"e49GdLWS4OfA/gRTlQKwIcPtrA4VT8fzo/ma6j35ZqIbYf6MlJmOeQGnqyGQfjBh\n"
"79O4vrAUOO3ZU+v31ADwoUwxt2kCot+iwm75bBkcH7rHHoTHPerGutPJ/Zqg9iZQ\n"
"twmb3uB6F2GkmfitMDWHJCHxWy29A4aBXe33aQIDAQABAoIBAQCUFgvNSil8CCPb\n"
"CyvNHbd9BAK2Qj5enypKyn55Oj7EAq8f1mdRXDkRetZQBrDKKON54XUj4qGLZ9N6\n"
"n6PdTMKYEcp8/BG9+mQFVe8OpCDvapw4acGlf9md4a+Tv9VE3JNVdNRWCre9VIQw\n"
"DMJJprnbOt+i/JXdN2HQe6Pqz5HI0m8eB82u+NzTa2PsXGY5sJLIYkeaJW6+IOFU\n"
"H0mkLK+6IVP2b9eUrla2xzGcsdddcChVCX6W7jfG6T/uYIanrrIaAf3VylZZ4Fzj\n"
"Wtbf0eUSouEthXbr/Rq4Ejl0NcU3eZ8iDmySZSvLvSxlMpIusn3zBp8Oht8lr6db\n"
"E+pGs0KxAoGBAPHy/3GptKI2b4JjJt1h/WD3maMUFGprGF17mzvAJXcmi2Lnx8Gz\n"
"Mp/lXS39sO4goO3zaqya8Kaa57+PUj5Pg90sYWs8zt79xxZTcv2C5QYukTmVTKpH\n"
"OA0NjZTRTAaTi/XI0cOv+8XdHEz3oa93y+VFUCVempKTU+8WLiqNNxrdAoGBAN8Q\n"
"QZR19hd7ZK5p2r6SpC0pQ2RepGnhpB0Wuw8C6D5DeNprMLwNwaVJTx2AD9grXLqa\n"
"d2O0f4OfQi8VXbeWoX6kmPF7jjjhzRVNRhhwu44ELjy/s2fYVufQ2rs3U8hNEC71\n"
"IidbZ45MaV7Dz2uoq0/gD19aC+PuCJxp4cEy9+f9AoGALSO72rXhuH3VcgsTniwA\n"
"/M1VAaLmaDBQ3GgKPyyLEN7FFI/PqejH4iMFrmXtU5AM1jgax9ckXMD7ybZwjqWB\n"
"IYt9JWez5NCOCzFIe9mm5u9bMtYS9pXQ9EZ6CrC9sWBntVRhJlQ38eNdu1Jb/QOp\n"
"q72xxTcQOZZ5C1slA9VS2ZUCgYBDubHPm4keVA6bjfQmTGugcpjNjpaotyOW2WHu\n"
"CtnsnZvB5S2kyyBySLHGqS0mWvQKi/+XvCPDGMWFeh08MnXR97OgDf1+c8BKqW2Q\n"
"2hl34vkO5L1Z2Ct6ssTWdtPvSsaRG0hFXtEwOS784MLHUyzAUa0ZLEPT7l+a7eLn\n"
"P8xZGQKBgQDGQrCf80OD2xhPfngpqCmGNDbOMCtKKkD55f202cVQzm25i/Ye4WcF\n"
"WxE4xhpSGLvDLITkaXXkRqtAQUjJrFXt8SjJ+24BVRlvtHby9yuEZZKwmzjhRcln\n"
"c62/pov7m2Ni8HOc/ANvejW9aMOrdNdACGMhKYAOIvKPS+GmA0Nl4Q==\n"
"-----END RSA PRIVATE KEY-----\n";

const char *beken_iot_1_pem =
"Certificate:\n"
"    Data:\n"
"        Version: 3 (0x2)\n"
"        Serial Number: 2 (0x2)\n"
"        Signature Algorithm: sha256WithRSAEncryption\n"
"        Issuer: C=IN, ST=MH, L=CITY, O=ORG, OU=UNIT, CN=CA/emailAddress=cacert@example.com\n"
"        Validity\n"
"            Not Before: Jun 15 05:46:54 2022 GMT\n"
"            Not After : Jun 12 05:46:54 2032 GMT\n"
"        Subject: C=IN, ST=MH, L=CITY, O=ORG1, OU=UNIT1, CN=CLIENT/emailAddress=client@example.com\n"
"        Subject Public Key Info:\n"
"            Public Key Algorithm: rsaEncryption\n"
"                RSA Public-Key: (2048 bit)\n"
"                Modulus:\n"
"                    00:d2:d2:09:ae:fc:c3:22:d6:65:cb:7c:dd:92:d2:\n"
"                    f4:2d:04:da:6d:08:b4:d9:1c:04:09:7a:f4:72:c2:\n"
"                    b0:a9:af:53:d8:8c:38:9c:09:e9:7c:cb:27:1d:7e:\n"
"                    d8:56:95:30:28:27:cf:ab:df:43:ce:28:b0:68:79:\n"
"                    93:b5:e8:ad:37:dc:bc:3e:62:e0:dd:42:e1:c9:1f:\n"
"                    ba:44:b7:d8:06:6d:0c:86:02:2e:24:13:ae:54:71:\n"
"                    fc:70:15:7e:49:cd:f6:9c:fc:cf:f9:7f:df:a8:56:\n"
"                    89:92:21:9c:cb:56:68:d0:2f:65:42:5c:48:a2:f1:\n"
"                    9e:94:f6:f4:5a:2c:91:ad:84:42:9a:9f:a4:7b:8f:\n"
"                    46:74:b5:92:e0:e7:c0:fe:04:53:95:02:b0:21:c3:\n"
"                    ed:ac:0e:15:4f:c7:f3:a3:f9:9a:ea:3d:f9:66:a2:\n"
"                    1b:61:fe:8c:94:99:8e:79:01:a7:ab:21:90:7e:30:\n"
"                    61:ef:d3:b8:be:b0:14:38:ed:d9:53:eb:f7:d4:00:\n"
"                    f0:a1:4c:31:b7:69:02:a2:df:a2:c2:6e:f9:6c:19:\n"
"                    1c:1f:ba:c7:1e:84:c7:3d:ea:c6:ba:d3:c9:fd:9a:\n"
"                    a0:f6:26:50:b7:09:9b:de:e0:7a:17:61:a4:99:f8:\n"
"                    ad:30:35:87:24:21:f1:5b:2d:bd:03:86:81:5d:ed:\n"
"                    f7:69\n"
"                Exponent: 65537 (0x10001)\n"
"        X509v3 extensions:\n"
"            X509v3 Basic Constraints:\n"
"                CA:FALSE\n"
"            Netscape Comment:\n"
"                OpenSSL Generated Certificate\n"
"            X509v3 Subject Key Identifier:\n"
"                15:55:BB:D8:0B:BF:93:51:14:91:AC:06:38:AC:17:39:54:44:64:FD\n"
"            X509v3 Authority Key Identifier:\n"
"                keyid:7B:90:47:BD:5F:E8:A5:38:2A:DB:C5:7F:E0:D0:B6:17:25:B1:1B:6C\n"

"    Signature Algorithm: sha256WithRSAEncryption\n"
"         a4:46:c2:13:21:c7:10:39:b7:c3:95:f3:81:e3:e9:2e:7c:5d:\n"
"         bf:19:d2:ed:c1:7d:ce:11:14:c4:7d:38:72:55:ba:2a:c4:24:\n"
"         f5:1e:1a:6a:0b:37:57:d6:22:cd:31:5b:33:1d:7e:27:b6:b3:\n"
"         d9:c0:b5:30:4b:18:0e:89:d5:df:cf:e1:5c:2f:23:f0:1d:0a:\n"
"         1b:3e:88:03:e3:0a:fc:c2:f4:f8:3a:a6:17:42:ea:59:6b:77:\n"
"         dc:2a:64:c0:07:0f:94:61:4c:59:84:34:75:8e:2b:97:63:ab:\n"
"         11:b6:16:6e:83:fd:e7:ea:80:bd:23:58:b0:10:7f:d1:5d:f3:\n"
"         af:f2:3d:60:14:6d:d0:51:8c:70:2d:5a:cf:75:f0:82:81:64:\n"
"         32:75:c7:b6:3d:ec:82:82:dd:73:1b:8a:a2:fc:a7:89:69:5b:\n"
"         bc:71:94:2b:5c:95:b3:c4:3e:e0:e3:b4:9e:67:8f:9f:b9:d1:\n"
"         dd:68:32:c1:33:2b:9b:e9:95:a7:54:6f:96:f3:f1:e2:b7:6a:\n"
"         01:d9:48:44:d5:1c:3d:40:f1:98:41:1d:88:8e:96:c9:81:c6:\n"
"         16:96:51:8a:3e:dc:59:ea:a0:9b:91:0a:d2:b2:e9:45:fb:7c:\n"
"         81:ae:9a:82:86:20:d9:9f:af:94:a9:97:c3:25:1d:2e:63:cf:\n"
"         10:53:7c:fb\n"
"-----BEGIN CERTIFICATE-----\n"
"MIID6DCCAtCgAwIBAgIBAjANBgkqhkiG9w0BAQsFADB2MQswCQYDVQQGEwJJTjEL\n"
"MAkGA1UECAwCTUgxDTALBgNVBAcMBENJVFkxDDAKBgNVBAoMA09SRzENMAsGA1UE\n"
"CwwEVU5JVDELMAkGA1UEAwwCQ0ExITAfBgkqhkiG9w0BCQEWEmNhY2VydEBleGFt\n"
"cGxlLmNvbTAeFw0yMjA2MTUwNTQ2NTRaFw0zMjA2MTIwNTQ2NTRaMHwxCzAJBgNV\n"
"BAYTAklOMQswCQYDVQQIDAJNSDENMAsGA1UEBwwEQ0lUWTENMAsGA1UECgwET1JH\n"
"MTEOMAwGA1UECwwFVU5JVDExDzANBgNVBAMMBkNMSUVOVDEhMB8GCSqGSIb3DQEJ\n"
"ARYSY2xpZW50QGV4YW1wbGUuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB\n"
"CgKCAQEA0tIJrvzDItZly3zdktL0LQTabQi02RwECXr0csKwqa9T2Iw4nAnpfMsn\n"
"HX7YVpUwKCfPq99DziiwaHmTteitN9y8PmLg3ULhyR+6RLfYBm0MhgIuJBOuVHH8\n"
"cBV+Sc32nPzP+X/fqFaJkiGcy1Zo0C9lQlxIovGelPb0WiyRrYRCmp+ke49GdLWS\n"
"4OfA/gRTlQKwIcPtrA4VT8fzo/ma6j35ZqIbYf6MlJmOeQGnqyGQfjBh79O4vrAU\n"
"OO3ZU+v31ADwoUwxt2kCot+iwm75bBkcH7rHHoTHPerGutPJ/Zqg9iZQtwmb3uB6\n"
"F2GkmfitMDWHJCHxWy29A4aBXe33aQIDAQABo3sweTAJBgNVHRMEAjAAMCwGCWCG\n"
"SAGG+EIBDQQfFh1PcGVuU1NMIEdlbmVyYXRlZCBDZXJ0aWZpY2F0ZTAdBgNVHQ4E\n"
"FgQUFVW72Au/k1EUkawGOKwXOVREZP0wHwYDVR0jBBgwFoAUe5BHvV/opTgq28V/\n"
"4NC2FyWxG2wwDQYJKoZIhvcNAQELBQADggEBAKRGwhMhxxA5t8OV84Hj6S58Xb8Z\n"
"0u3Bfc4RFMR9OHJVuirEJPUeGmoLN1fWIs0xWzMdfie2s9nAtTBLGA6J1d/P4Vwv\n"
"I/AdChs+iAPjCvzC9Pg6phdC6llrd9wqZMAHD5RhTFmENHWOK5djqxG2Fm6D/efq\n"
"gL0jWLAQf9Fd86/yPWAUbdBRjHAtWs918IKBZDJ1x7Y97IKC3XMbiqL8p4lpW7xx\n"
"lCtclbPEPuDjtJ5nj5+50d1oMsEzK5vpladUb5bz8eK3agHZSETVHD1A8ZhBHYiO\n"
"lsmBxhaWUYo+3FnqoJuRCtKy6UX7fIGumoKGINmfr5Spl8MlHS5jzxBTfPs=\n"
"-----END CERTIFICATE-----\n";

/*
 * Multiple ROOTCA: first is aplito, second is
 * USERTrustRSACertificationAuthority.crt
 */
const char *cacert_pem =
"-----BEGIN CERTIFICATE-----\n"
"MIIDzTCCArWgAwIBAgIUM6PdWvmY6SZ5uCkVm+5/cjojHSYwDQYJKoZIhvcNAQEL\n"
"BQAwdjELMAkGA1UEBhMCSU4xCzAJBgNVBAgMAk1IMQ0wCwYDVQQHDARDSVRZMQww\n"
"CgYDVQQKDANPUkcxDTALBgNVBAsMBFVOSVQxCzAJBgNVBAMMAkNBMSEwHwYJKoZI\n"
"hvcNAQkBFhJjYWNlcnRAZXhhbXBsZS5jb20wHhcNMjIwNjE1MDUzOTIxWhcNMzIw\n"
"NjEyMDUzOTIxWjB2MQswCQYDVQQGEwJJTjELMAkGA1UECAwCTUgxDTALBgNVBAcM\n"
"BENJVFkxDDAKBgNVBAoMA09SRzENMAsGA1UECwwEVU5JVDELMAkGA1UEAwwCQ0Ex\n"
"ITAfBgkqhkiG9w0BCQEWEmNhY2VydEBleGFtcGxlLmNvbTCCASIwDQYJKoZIhvcN\n"
"AQEBBQADggEPADCCAQoCggEBAMNZRWSX041X5OuorNKD9gPIcCnDZVHE4dZZnAH/\n"
"ZzXzVmAQDdq8vdg1oOMUi9TiqDD+bcIClaD93rt0FW6YVnGLgwo6lezyhfcrSReJ\n"
"SOtneWMXTl9EOKma9ThYDePL2SItBxIC7rVz5LJ6nxu8crW2XFehUrvGir7bJaCv\n"
"XHYwB82CuXM87U2y6Pa4r79KlxsAe615rx8h4lcj1tdqgs1v+waJZDBvxeOcz7Ay\n"
"GxbYyhStessRzxYpTRWRekpK8YDczQicHPStI6ndcL5RG6q0qHI9b2DZKXQYIO6U\n"
"cHAsw+GJf7FIP1XGx34qROpVNYsqiy165xEdV9ZR8CeKklcCAwEAAaNTMFEwHQYD\n"
"VR0OBBYEFHuQR71f6KU4KtvFf+DQthclsRtsMB8GA1UdIwQYMBaAFHuQR71f6KU4\n"
"KtvFf+DQthclsRtsMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggEB\n"
"AEDEb1Cf94fU5ndFxOsQcDRLCCFpUFPjdRjxlcbI5bJWgVqoCqcMQoE/UcrlSVez\n"
"Mdn4pXbN4jsbsLLR4Zbpx0LVjuCvC5iNpQSgreqkb7g5/NEIGt3HocYLUX2VRIGE\n"
"KpxDFsUFTKNVetVXiPnqlJEFpVhvhKmIVKlakL+iHNUaB89LXvHVKI1/wBcmJ1/a\n"
"TPMR7jdnzfK8SqiYjKajj5BJAbbsQ8q0dXATEcrm9FA3Jr4+11/uYoxxbxTwX+eK\n"
"vveMohWRY0TUoh0N89AKXb/msTy5aiDJG6YbrGtnn+ydLURCT9On5jFABUC50cDD\n"
"9l1muXNGsn7fHKbbuPyEgAA=\n"
"-----END CERTIFICATE-----\n";
#endif

void os_sleep(os_time_t sec, os_time_t usec)
{
	rtos_delay_milliseconds(sec * 1000 + usec/1000);
}

time_t os_time(time_t *tm)
{
	uint64_t ticks = bk_get_tick();
	time_t t = ticks / bk_get_ticks_per_second();
	if (tm)
		*tm = t;

	return t;
}

int os_get_time(struct os_time *t)
{
	uint64_t ticks = bk_get_tick();
	if (t) {
		t->sec = ticks / bk_get_ticks_per_second();
		t->usec = ((ticks % bk_get_ticks_per_second()) * 1000000) / bk_get_ticks_per_second();
	}

	return 0;
}


__IRAM_SEC int os_get_reltime(struct os_reltime *t)
{
	uint64_t ticks = bk_get_tick();
	if (t) {
		t->sec = ticks / bk_get_ticks_per_second();
		t->usec = ((ticks % bk_get_ticks_per_second()) * 1000000) / bk_get_ticks_per_second();
	}

	return 0;
}


int os_mktime(int year, int month, int day, int hour, int min, int sec,
	      os_time_t *t)
{
#ifdef IEEE8021X_EAPOL_DEMO
	/* FIX: Implement */
	return 0;
#else
	return -1;
#endif
}

int os_gmtime(os_time_t t, struct os_tm *tm)
{
	return -1;
}


int os_daemonize(const char *pid_file)
{
	return -1;
}


void os_daemonize_terminate(const char *pid_file)
{
}

int os_get_random(unsigned char *buf, size_t sz)
{
	int r;
	int len;

	while (sz > 0) {
		len = sizeof(r) > sz ? sz : sizeof(r);
		r = rand();
		os_memcpy(buf, &r, len);
		buf += len;
		sz -= len;
	}

	return 0;
}

unsigned long os_random(void)
{
	return rand();
}


char * os_rel2abs_path(const char *rel_path)
{
	return NULL; /* strdup(rel_path) can be used here */
}


int os_program_init(void)
{
#ifdef IEEE8021X_EAPOL_DEMO
	static int inited = 0;

	if (!inited) {
		register_xfile("client.cer", (char *)client_cer, strlen(client_cer));
		register_xfile("client-key.pem", (char *)client_key_pem, strlen(client_key_pem));

		// starts of aptilo's IoT demo EAP-TLS key pair, RootCA
		register_xfile("beken-iot-1.key", (char *)beken_iot_1_key, strlen(beken_iot_1_key));
		register_xfile("beken-iot-1.pem", (char *)beken_iot_1_pem, strlen(beken_iot_1_pem));
		register_xfile("cacert.pem", (char *)cacert_pem, strlen(cacert_pem));
		inited = 1;
	}
#endif

	return 0;
}


void os_program_deinit(void)
{
}


int os_setenv(const char *name, const char *value, int overwrite)
{
	return -1;
}


int os_unsetenv(const char *name)
{
	return -1;
}


extern void wpa_dbg(void *ctx, int level, const char *fmt, ...);

char *os_readfile(const char *name, size_t *len)
{
#ifdef IEEE8021X_EAPOL_DEMO
	xFILE *f;
	char *buf;

	f = xfopen(name, "r");
	if (!f)
		return NULL;
	*len = xfsize(f);
	buf = (char *)os_zalloc(*len);
	if (!buf) {
		xfclose(f);
		return NULL;
	}

	if (xfread(buf, *len, 1, f) < 0) {
		os_free(buf);
		xfclose(f);
		return NULL;
	}

	xfclose(f);
	//wpa_dbg(NULL, 2, "|%s|\n", buf);
	return buf;
#else
	return NULL;
#endif
}


int os_fdatasync(FILE *stream)
{
	return 0;
}


int os_exec(const char *program, const char *arg, int wait_completion)
{
	return -1;
}

#if 0
int os_memcmp_const(const void *a, const void *b, size_t len)
{
	const u8 *aa = a;
	const u8 *bb = b;
	size_t i;
	u8 res;

	for (res = 0, i = 0; i < len; i++)
		res |= aa[i] ^ bb[i];

	return res;
}
#endif

void * os_memdup(const void *src, size_t len)
{
	void *r = os_malloc(len == 0 ? 1 : len);

	if (r && src)
		os_memcpy(r, src, len);
	return r;
}

// eof

