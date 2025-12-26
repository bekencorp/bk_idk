/*
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#include <stddef.h>

#if 0
#define ENTRUST_ROOT_CA                                                 \
"-----BEGIN CERTIFICATE-----\r\n"                                       \
"MIIEPjCCAyagAwIBAgIESlOMKDANBgkqhkiG9w0BAQsFADCBvjELMAkGA1UEBhMC\r\n"  \
"VVMxFjAUBgNVBAoTDUVudHJ1c3QsIEluYy4xKDAmBgNVBAsTH1NlZSB3d3cuZW50\r\n"  \
"cnVzdC5uZXQvbGVnYWwtdGVybXMxOTA3BgNVBAsTMChjKSAyMDA5IEVudHJ1c3Qs\r\n"  \
"IEluYy4gLSBmb3IgYXV0aG9yaXplZCB1c2Ugb25seTEyMDAGA1UEAxMpRW50cnVz\r\n"  \
"dCBSb290IENlcnRpZmljYXRpb24gQXV0aG9yaXR5IC0gRzIwHhcNMDkwNzA3MTcy\r\n"  \
"NTU0WhcNMzAxMjA3MTc1NTU0WjCBvjELMAkGA1UEBhMCVVMxFjAUBgNVBAoTDUVu\r\n"  \
"dHJ1c3QsIEluYy4xKDAmBgNVBAsTH1NlZSB3d3cuZW50cnVzdC5uZXQvbGVnYWwt\r\n"  \
"dGVybXMxOTA3BgNVBAsTMChjKSAyMDA5IEVudHJ1c3QsIEluYy4gLSBmb3IgYXV0\r\n"  \
"aG9yaXplZCB1c2Ugb25seTEyMDAGA1UEAxMpRW50cnVzdCBSb290IENlcnRpZmlj\r\n"  \
"YXRpb24gQXV0aG9yaXR5IC0gRzIwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\r\n"  \
"AoIBAQC6hLZy254Ma+KZ6TABp3bqMriVQRrJ2mFOWHLP/vaCeb9zYQYKpSfYs1/T\r\n"  \
"RU4cctZOMvJyig/3gxnQaoCAAEUesMfnmr8SVycco2gvCoe9amsOXmXzHHfV1IWN\r\n"  \
"cCG0szLni6LVhjkCsbjSR87kyUnEO6fe+1R9V77w6G7CebI6C1XiUJgWMhNcL3hW\r\n"  \
"wcKUs/Ja5CeanyTXxuzQmyWC48zCxEXFjJd6BmsqEZ+pCm5IO2/b1BEZQvePB7/1\r\n"  \
"U1+cPvQXLOZprE4yTGJ36rfo5bs0vBmLrpxR57d+tVOxMyLlbc9wPBr64ptntoP0\r\n"  \
"jaWvYkxN4FisZDQSA/i2jZRjJKRxAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAP\r\n"  \
"BgNVHRMBAf8EBTADAQH/MB0GA1UdDgQWBBRqciZ60B7vfec7aVHUbI2fkBJmqzAN\r\n"  \
"BgkqhkiG9w0BAQsFAAOCAQEAeZ8dlsa2eT8ijYfThwMEYGprmi5ZiXMRrEPR9RP/\r\n"  \
"jTkrwPK9T3CMqS/qF8QLVJ7UG5aYMzyorWKiAHarWWluBh1+xLlEjZivEtRh2woZ\r\n"  \
"Rkfz6/djwUAFQKXSt/S1mja/qYh2iARVBCuch38aNzx+LaUa2NSJXsq9rD1s2G2v\r\n"  \
"1fN2D807iDginWyTmsQ9v4IbZT+mD12q/OWyFcq1rca8PdCE6OoGcrBNOTJ4vz4R\r\n"  \
"nAuknZoh8/CbCzB428Hch0P+vGOaysXCHMnHjf87ElgI5rY97HosTvuDls4MPGmH\r\n"  \
"VHOkc8KT/1EQrBVUAdj8BbGJoX90g5pJ19xOe4pIb4tF9g==\r\n"                  \
"-----END CERTIFICATE-----\r\n"


const char mbedtls_root_certificate[] = 
	ENTRUST_ROOT_CA
	;
#elif 0
/////www.baidu.com
#define GLOBALSIGN_ROOT_CA             \
"-----BEGIN CERTIFICATE-----\r\n"                                       \
"MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG\r\n"  \
"A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv\r\n"  \
"b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw\r\n"  \
"MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i\r\n"  \
"YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT\r\n"  \
"aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ\r\n"  \
"jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp\r\n"  \
"xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp\r\n"  \
"1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG\r\n"  \
"snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ\r\n"  \
"U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8\r\n"  \
"9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E\r\n"  \
"BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B\r\n"  \
"AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz\r\n"  \
"yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE\r\n"  \
"38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP\r\n"  \
"AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad\r\n"  \
"DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME\r\n"  \
"HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\r\n"                              \
"-----END CERTIFICATE-----"

const char mbedtls_root_certificate[] = 
	GLOBALSIGN_ROOT_CA
;
#elif 1
/* 
 * Root CA Certificate Selection
 * 
 * Available options:
 * 1. GLOBALSIGN_ROOT_CA - default 
 * 2. DIGI_GLOBAL_ROOT_CA - DigiCert Global Root CA for connecting to broker.emqx.io   
 * 3. MOSQUITTO_ROOT_CA - Self-signed certificate for Mosquitto MQTT broker
 * 
 * To change CA, modify the #if conditions below and update the mbedtls_root_certificate assignment:
 * - Set #if 1 for the desired CA
 * - Set #if 0 for other CAs
 * - Update mbedtls_root_certificate to use the selected CA macro
 */
////myca for mqtt
#define GLOBALSIGN_ROOT_CA   \
"-----BEGIN CERTIFICATE-----\r\n"  \
"MIID3zCCAsegAwIBAgIJAODxoOUE+Ia8MA0GCSqGSIb3DQEBCwUAMIGFMQswCQYD\r\n"  \
"VQQGEwJDTjEQMA4GA1UECAwHc2hhZ2hhaTERMA8GA1UEBwwIc2hhbmdoYWkxDjAM\r\n"  \
"BgNVBAoMBUJFS0VOMRAwDgYDVQQLDAd3aWZpLWdlMRIwEAYDVQQDDAl3aWZpLXRl\r\n"  \
"YW0xGzAZBgkqhkiG9w0BCQEWDGN3bGluMTYzLmNvbTAeFw0yMDA4MTEwNTQ3Mjha\r\n"  \
"Fw0zMDA4MDkwNTQ3MjhaMIGFMQswCQYDVQQGEwJDTjEQMA4GA1UECAwHc2hhZ2hh\r\n"  \
"aTERMA8GA1UEBwwIc2hhbmdoYWkxDjAMBgNVBAoMBUJFS0VOMRAwDgYDVQQLDAd3\r\n"  \
"aWZpLWdlMRIwEAYDVQQDDAl3aWZpLXRlYW0xGzAZBgkqhkiG9w0BCQEWDGN3bGlu\r\n"  \
"MTYzLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALwa6fM/5xuR\r\n"  \
"nAHQZ+pDDOAK6EJf04yO28G9NGPYP2mQKhkBhcYC9G9xXLglvuCfXFKpsXxMMOYI\r\n"  \
"LjQWL8iN0M6tyt+t7kEbSlBwac0IuqjB62LvK623XvX7k7an/E87oCZih6TYzCy7\r\n"  \
"5HGIELhODa/8sHG5HT8gkqSCJscKbpCocwj13PPs6vwloCmuG0LxB5xb3CkWQ8Kv\r\n"  \
"DCIF2jBTNbnYixlLYZ/P0SQMlHGLcOqTPvWEnowA1VqgNBCDVyln26XM87AV+qQB\r\n"  \
"5dBcLOZmnX0cpg+SFG+wrNUFN4WKI8Pl2suUiD4jeIixAWS2pyOsm8SvrM1YipM0\r\n"  \
"6IdqUQIeCLsCAwEAAaNQME4wHQYDVR0OBBYEFG7AMq4Tgm/sPp9Nl81gL1/YOcrQ\r\n"  \
"MB8GA1UdIwQYMBaAFG7AMq4Tgm/sPp9Nl81gL1/YOcrQMAwGA1UdEwQFMAMBAf8w\r\n"  \
"DQYJKoZIhvcNAQELBQADggEBACPRNVhHXi+8o5hSSX6dBqdUljeAvN2lDYFgVWPr\r\n"  \
"E3teVQADS3kd/60ACLc34ZP+OojlPMz3zbDosH5gRntwFGJUGqkJgQt7xUS2wgyo\r\n"  \
"fuxLNrnz2dIkVfDA/q4GpPVBZwlFKjEc1N6P7ENiJ/DJLkvYi78NDneComid08q2\r\n"  \
"OAbAJjTcUSOYbEygS2FPgFRR2O4s9nrphDgQIzWLgeHIhT7feSyQ7d0emMUv1iEQ\r\n"  \
"LXJU9sbgYU2G02D51tdz+tFObmtGxiy99gEunK6S9AeUnh8xM5eTJW2ebe0CdFmo\r\n"  \
"JkLASY80XmUo6WQVq7iN6mO3zsgIZmwmwSVnTcSPcHARoZ4=\r\n"  \
"-----END CERTIFICATE-----\r\n"

/* DIGI_GLOBAL_ROOT_CA - DigiCert Global Root CA for connecting to broker.emqx.io */
#define DIGI_GLOBAL_ROOT_CA   \
"-----BEGIN CERTIFICATE-----\r\n"  \
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\r\n"  \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"  \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\r\n"  \
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\r\n"  \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n"  \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\r\n"  \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\r\n"  \
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\r\n"  \
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\r\n"  \
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\r\n"  \
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\r\n"  \
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\r\n"  \
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\r\n"  \
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\r\n"  \
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\r\n"  \
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\r\n"  \
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\r\n"  \
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\r\n"  \
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\r\n"  \
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\r\n"  \
"-----END CERTIFICATE-----\r\n"  \

/* MOSQUITTO_ROOT_CA - Self-signed certificate for Mosquitto MQTT broker */
#define MOSQUITTO_ROOT_CA   \
"-----BEGIN CERTIFICATE-----\r\n"  \
"MIIDSzCCAjOgAwIBAgIUBsIrK3nP7iAr2qqJ9QzIN6xkvQ8wDQYJKoZIhvcNAQEL\r\n"  \
"BQAwNTELMAkGA1UEBhMCQ04xDjAMBgNVBAoMBU15T3JnMRYwFAYDVQQDDA1NeU9y\r\n"  \
"ZyBSb290IENBMB4XDTI1MTAzMDA5NTUzM1oXDTM1MTAyODA5NTUzM1owNTELMAkG\r\n"  \
"A1UEBhMCQ04xDjAMBgNVBAoMBU15T3JnMRYwFAYDVQQDDA1NeU9yZyBSb290IENB\r\n"  \
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2h+9NDj2OnI1QTVybhiC\r\n"  \
"Nkr9y6L+lxfz+J4wSwaHCRMoJg2t8F8067we9EQcvJ7me++hXUN3ms0HOvOgb0r7\r\n"  \
"6m8ZApfwei1EMIWIycbPSNO3WXNknlNX20dzH3mLooKckJZ6JFUlBjFBkDbuMgg7\r\n"  \
"g/4IYWVx58oC1cX6cDhG6x8Lwdse+u0CSJoMJGgNJE78+2ekf551+QBN2qaag1if\r\n"  \
"KVWTe9K2bLdKT5Cp3XCOpI4alC0X7O+sTnmER9vfTKPiSvwuCs9tXLxdmIlJOrBq\r\n"  \
"WXbCIe6fbAlUCKE+fC2aSb6Rq3drhLP4LBI/KsNRIoXtrMflCt3T5LRgeAsFJfm5\r\n"  \
"YQIDAQABo1MwUTAdBgNVHQ4EFgQUUa1iLLBGGRJcs3KyAxerC5hfx4cwHwYDVR0j\r\n"  \
"BBgwFoAUUa1iLLBGGRJcs3KyAxerC5hfx4cwDwYDVR0TAQH/BAUwAwEB/zANBgkq\r\n"  \
"hkiG9w0BAQsFAAOCAQEAbFqe3jDyz/tfHj+xNeHSo7SRyarMqMkIJMi9CtnXAz/t\r\n"  \
"k4yXljELg54O5O3t/2g/DhVo59d4UWFuylQgqMImW9ra42sJCItRax4BKVx47mEa\r\n"  \
"+Jp0l4tXZTR1kzR4vcFNOKSf/IlTzcEWGiz+xd1fCTmOw9/AHATeGN74NqrQruIg\r\n"  \
"aqnukyNTv4uvLIeWZh5hiibMYWS368FIYwdHcoOHruHpWwp9Iye1LoogWNGky26D\r\n"  \
"NczVFyvJ/sY6W182Xn4Uj9tEda7aEvEXEdBDSLF1V6YbDErbGbSa3BClfByVidyk\r\n"  \
"593bwbd2lAkpkdtBSH8+tDSWb60uuHtgbuaipBzeEQ==\r\n"  \
"-----END CERTIFICATE-----\r\n"  \

const char mbedtls_root_certificate[] = 
	MOSQUITTO_ROOT_CA
;
#endif


const size_t mbedtls_root_certificate_len = sizeof(mbedtls_root_certificate);

#if 1
/* Client certificate and private key for testing with Mosquitto MQTT broker */
#define MOSQUITTO_CLIENT_CRT   \
"-----BEGIN CERTIFICATE-----\r\n"  \
"MIICzzCCAbcCFHya+WQBvBpkKIK7WZlLV2HC/yKlMA0GCSqGSIb3DQEBCwUAMDUx\r\n"  \
"CzAJBgNVBAYTAkNOMQ4wDAYDVQQKDAVNeU9yZzEWMBQGA1UEAwwNTXlPcmcgUm9v\r\n"  \
"dCBDQTAeFw0yNTEwMzAxMDMwMTBaFw0yNjEwMzAxMDMwMTBaMBMxETAPBgNVBAMM\r\n"  \
"CG15Y2xpZW50MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA3GjaOs8m\r\n"  \
"Ufrp1CijOp68DZHoVZ36dO91uVBBZoaucry0hWXYi7flWYTFwlJp4NibV6ZFPJol\r\n"  \
"Nr8MbfJFu4wky6ktrO3Vx40D1udp3yypGyqitImexW6QyX0+D0q1jQLki30L35z7\r\n"  \
"QsCptTg2mDzN8p0D+OQA201+ZaR6kx7ajCfkl91+7/Anah1D6cTmqOytepHL09cc\r\n"  \
"u9HTvMmfP8PUdQ1NNJbJnLT20R5aZZDREW80V491DgOaJzTcMaS0kYjz0KKG8l/x\r\n"  \
"/Q830P3xjxADwBo6CibZ+Z75Da+pNHnDjt4NkcePL2fA/wJODAWOUQJ+gMsRmXWu\r\n"  \
"Rh7/9KfBtgcO1wIDAQABMA0GCSqGSIb3DQEBCwUAA4IBAQCuYr445aTTUV0b/+P8\r\n"  \
"ExaTdn+svCvtbNtt9uqBouk25DL+thLn71I2U0bjFWKfNY/a1Bm0TFT1JkiNX8cc\r\n"  \
"OeJHi84k8QLmsdKlns7J54VZpO9Qc/lu7aJPmdEA/qOAvJ0IB+xdgrBNgSe3meoE\r\n"  \
"SBQMc6nXML2s+mY9+Y3tGtovqhSl9gON7+9tGnmbZ2x1QCOTY53uP271oft3FbBc\r\n"  \
"5EmSWF6xVXKHabX9YboM5hJKJ0bsfP007pAKoRY22fVdCCTuzR04zXR1f1Ltbx+C\r\n"  \
"8lGP85+waTn4Y9ae2Cl2JQIBtfkS8je7DrjEbjIvMVxR4Q83tcn00lj+y2YWzh8S\r\n"  \
"DCF1\r\n"  \
"-----END CERTIFICATE-----\r\n"  \

#define MOSQUITTO_CLIENT_KEY   \
"-----BEGIN RSA PRIVATE KEY-----\r\n"  \
"MIIEogIBAAKCAQEA3GjaOs8mUfrp1CijOp68DZHoVZ36dO91uVBBZoaucry0hWXY\r\n"  \
"i7flWYTFwlJp4NibV6ZFPJolNr8MbfJFu4wky6ktrO3Vx40D1udp3yypGyqitIme\r\n"  \
"xW6QyX0+D0q1jQLki30L35z7QsCptTg2mDzN8p0D+OQA201+ZaR6kx7ajCfkl91+\r\n"  \
"7/Anah1D6cTmqOytepHL09ccu9HTvMmfP8PUdQ1NNJbJnLT20R5aZZDREW80V491\r\n"  \
"DgOaJzTcMaS0kYjz0KKG8l/x/Q830P3xjxADwBo6CibZ+Z75Da+pNHnDjt4NkceP\r\n"  \
"L2fA/wJODAWOUQJ+gMsRmXWuRh7/9KfBtgcO1wIDAQABAoIBABHa2Jrc/xCzD9Ak\r\n"  \
"/pwYEcnToQuWgZyJbXL2omWo0WbwDHul9XXUDttCmzaTsIoLYgImsQMoxYz6ywn5\r\n"  \
"D1cTEkQBT3bryV6h+Mam9neiyYwu1wFjCJ6bx1TbNXTNq4lhy5vVJGoX8G7G5riM\r\n"  \
"dje87T4TJCg8aClbzLp1KYzTegbSafZPz/i6Gynoiu/0cZlY6V+PsE38YqIdbifW\r\n"  \
"j9JQHcURFSzIBx6O1FmxjeHk/WqeuRo/HgljCE+qBrI5aUsq1zrtEGjxDll+MXxw\r\n"  \
"Lo9P+cu5do9/O++8q5lCRKKAoE4/hrlwxUOdiiLaifpxXsbMN2QAh00SeUfQYeDP\r\n"  \
"EXSsSpkCgYEA+NCcrAARC1zZXOGokOfXs0Xk3vTpYEtuEPHS9t+KS2X3h891Jz2J\r\n"  \
"rQaDrSotMMUSoIPc4rNux4Qi0dEkQY+J/kqk+d/lT6HUSepnz7XbZqnS/rx0TTlW\r\n"  \
"qXpOEmpJ3fMCNdiFlRTWld/oOnRBSEMeI8+bjOMMocxRSCYVuA/l0mMCgYEA4sZA\r\n"  \
"YFcbV3VKOgrjmf7gF+9Irq7FllIzbnFuve+uqGSiasvUiQcriKUkUSiKkqu76/l9\r\n"  \
"ziuo8fJHmHgf1vChWEoJ7WpclD/pV5GK2U0+yGctzlQGMMwE1booEVPwZoHHo8rV\r\n"  \
"i18954vah9omGfkcCg7I1+lLFkx+N9Jh5sAJQf0CgYAR1CDl0jtmbQjGNwU1HOe/\r\n"  \
"2MpFj5cJZ15DqJBNUEdAj9XkzWC/pxEubMESr2r9i6GCDvDM7b6KXVWBY1MNv5NL\r\n"  \
"vV2E6h4sTpQ/l6RIpedKu/B6gFZ1Eh67lh/yAdu5I6iM4y7vN8cIhjrFtX7YBrcq\r\n"  \
"Kb4jokFNdq58VP5Jaho1sQKBgElA5Ta5rJlZx/pr3g22qUjSANZ8mlLuhrKcbbtp\r\n"  \
"GzBGIbkB0svYxxVC0zJsOCcc1n1pgFwC+nX9X2c/FnnmDRhqAj7w5qr04jlpSELd\r\n"  \
"kvRFcCSAO+ezX7Ryh9LhHHzgW07rjIOS5npPUO4lZ71oHMia8gHc2GaBmxwJF5rk\r\n"  \
"WZk5AoGAUUh94awlQAoovUDx2koivHm5lTdkPESfsFULEW4BMP5O/NwZm6LBEbVJ\r\n"  \
"HYarrJLNUwNl6c9qk1yIRSagu54VYj+rN9dqFBAYjZuWqkNQlparYsvhjnoej+8b\r\n"  \
"PcdxfY56gPOJ1i4z8FQZ2zx772Fu9N+HzrdJFCaKI58gy9ii17w=\r\n"  \
"-----END RSA PRIVATE KEY-----\r\n"  \


const char mbedtls_client_certificate[] =
    MOSQUITTO_CLIENT_CRT
    "";
const size_t mbedtls_client_certificate_len = sizeof(mbedtls_client_certificate);

const char mbedtls_client_key[] =
    MOSQUITTO_CLIENT_KEY
    "";
const size_t mbedtls_client_key_len = sizeof(mbedtls_client_key);
#endif

#if 0
///client client.crt
#define GLOBALSIGN_Client_crt   \
"-----BEGIN CERTIFICATE-----\r\n"  \
"MIIDkDCCAngCCQCb4RI7B1uRIDANBgkqhkiG9w0BAQsFADCBhTELMAkGA1UEBhMC\r\n"  \
"Q04xEDAOBgNVBAgMB3NoYWdoYWkxETAPBgNVBAcMCHNoYW5naGFpMQ4wDAYDVQQK\r\n"  \
"DAVCRUtFTjEQMA4GA1UECwwHd2lmaS1nZTESMBAGA1UEAwwJd2lmaS10ZWFtMRsw\r\n"  \
"GQYJKoZIhvcNAQkBFgxjd2xpbjE2My5jb20wHhcNMjAwODExMDU1MzI1WhcNMzAw\r\n"  \
"ODA5MDU1MzI1WjCBjTELMAkGA1UEBhMCQ04xETAPBgNVBAgMCHNoYW5naGFpMREw\r\n"  \
"DwYDVQQHDAhzaGFuZ2hhaTEOMAwGA1UECgwFYmVrZW4xEzARBgNVBAsMCmJla2Vu\r\n"  \
"LXdpZmkxFjAUBgNVBAMMDTE5Mi4xNjguMTkuMjQxGzAZBgkqhkiG9w0BCQEWDGN3\r\n"  \
"bGluMTYzLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMSKa+Cn\r\n"  \
"9wVL7H1/zY02jJTHmq3Y6XfMwwkHsCE3Oag62snjNs60LQCYIQvGMXRmz9hIuWRW\r\n"  \
"hAA627TLIAvGhXG5mFkuG3HJJLQLdTGuWobjTL74/hrSAstmJM/eg9M1FHvWO9rq\r\n"  \
"bTNagybd9k2BjdZdah1QFlDagECtAbBKq89riKqvlwaIJP43Xb0dYXZ223iHsNDv\r\n"  \
"wYO9TU9Cl7U49UURUA7hw649cPtVA++uKQJSsqEpHF1/Q0fJcCnFqfPSZsqdhBmO\r\n"  \
"p4S1t+xqpautEiGWyBcdOZ1Og4a5gio4hBt4hDpLWXIF/mrjGMvImm4RlOkAhbil\r\n"  \
"6nZz+ZwXl2RD4ZkCAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAkY7PGZkxn4zoV+/a\r\n"  \
"EBcZ06V9PqCYtn+8/r4ciBW1GHjveOXX5plVkaKpkx1QFOS5RV96vk4N10HnKO9V\r\n"  \
"YjRqPN3maiQ+bn3Yve0zopMPbNiA+oTZqrP8jRlpTujCo7XE6fKXfQYpRXijXDHB\r\n"  \
"Ty4pURq8UkEmyZyFoSUYgib2aG0K656OOG5LSRRda4NSWJilhLB5Aheb7gfVcLut\r\n"  \
"eUq28qFSx0BC+Yf2IaMMnOIO1HD8mgdEgktGlX7eapAive3g/sFmaX9TPzV6hJ5U\r\n"  \
"s4OvpG2D3BzwJH5pmoi1ovOi2m0WauR8Z3W3m7kywLEUtfEbX0W+Q02CPMYw1jmi\r\n"  \
"D/qgUw==\r\n"  \
"-----END CERTIFICATE-----\r\n"


const char mbedtls_client_certificate[] =
	GLOBALSIGN_ROOT_CA
	GLOBALSIGN_Client_crt
;
const size_t mbedtls_client_certificate_len = sizeof(mbedtls_client_certificate);
#endif



