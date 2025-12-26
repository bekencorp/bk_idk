#ifndef TLS_CONFIG_H
#define TLS_CONFIG_H

#include <os/os.h>
#include <os/mem.h>

#define tls_malloc  os_malloc
#define tls_free    os_free
#define TLS_NULL    0U

#endif /* TLS_CONFIG_H */