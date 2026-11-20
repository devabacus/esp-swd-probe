set(COMPONENT_ADD_INCLUDEDIRS "${PROJECT_PATH}")
set(COMPONENT_SRCS "usb_serial.c" "usb_serial_esp.c"
    main.c timer.c tcp_server.c usbip_server.c DAP_handle.c
    uart_bridge.c wifi_handle.c)

if(CONFIG_USE_WEBSOCKET_DAP)
    list(APPEND COMPONENT_SRCS "websocket_server.c")
endif()

if(CONFIG_DAP_TRANSPORT_USB)
    list(APPEND COMPONENT_SRCS "usb_dap.c" "dap_pktring.c")
endif()

register_component()
