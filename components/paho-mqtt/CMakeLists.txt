set(incs paho-mqtt)

unset(c_compile_options)
# MQTT communication options
list(APPEND c_compile_options "-DMQTT_COMM_ENABLED")
list(APPEND c_compile_options "-DMQTT_DIRECT")
list(APPEND c_compile_options "-DIOTX_WITHOUT_TLS")
list(APPEND c_compile_options "-DOTA_SIGNAL_CHANNEL=1")

# Paho MQTT specific options
#list(APPEND c_compile_options "-DMQTT_TASK")
list(APPEND c_compile_options "-DMAX_PACKET_ID=65535")

if (CONFIG_PAHO_MQTT)
    # Include directories
    list(APPEND incs
        client
        client/src
        packet/src
        mqtt_ui
        mqtt_ui/tcp_mqtt
        mqtt_ui/ssl_mqtt
    )

    # Core MQTT client source files
    list(APPEND srcs
        client/src/MQTTClient.c
        client/src/MQTTFreeRTOS.c
        client/paho_mqtt_udp.c
    )

    # MQTT UI (user interface) source files
    list(APPEND srcs
        mqtt_ui/mqtt_client_core.c
        mqtt_ui/mqtt_client_com_port.c
        mqtt_ui/tcp_mqtt/tcp_mqtt_client_port.c
    )

    # MQTT packet handling source files
    list(APPEND srcs
        packet/src/MQTTConnectClient.c
        packet/src/MQTTDeserializePublish.c
        packet/src/MQTTPacket.c
        packet/src/MQTTSerializePublish.c
        packet/src/MQTTSubscribeClient.c
        packet/src/MQTTUnsubscribeClient.c
        packet/src/MQTTFormat.c
    )

    # Optional SSL/TLS support (if enabled)
    if (CONFIG_PAHO_MQTT_TLS)
        list(APPEND srcs
            mqtt_ui/ssl_mqtt/ssl_mqtt_client_port.c
        )
        list(APPEND c_compile_options "-DMQTT_USING_TLS")
        # Ensure sl_tls.h can be found from mbedtls component
        list(APPEND incs
            ${ARMINO_PATH}/components/psa_mbedtls/mbedtls_ui
            ${ARMINO_PATH}/components/psa_mbedtls/mbedtls_port/inc
            ${ARMINO_PATH}/components/psa_mbedtls/mbedtls/include
        )
    endif()
    # Test files (optional, for development)
    if (CONFIG_PAHO_MQTT_TEST)
        list(APPEND srcs
            client/test/test1.c
            packet/test/test1.c
        )
    endif()
endif()

# Register the component
# Collect private requirements
set(priv_reqs bk_common http lwip_intf_v2_1 bk_rtos)
set(pub_reqs)
if (CONFIG_PAHO_MQTT_TLS)
    list(APPEND pub_reqs psa_mbedtls)
endif()
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" REQUIRES ${pub_reqs} PRIV_REQUIRES ${priv_reqs})

# Set CFLAGS
armino_build_set_property(C_COMPILE_OPTIONS "${c_compile_options}" APPEND)
