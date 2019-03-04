#

#==============================================================================
# Global defines
#==============================================================================
GLOBAL_DEFINES += STDIO_BUFFER_SIZE=256

NAME := App_ebisu_demo

$(NAME)_DEFINES += KII_PUSH_KEEP_ALIVE_INTERVAL_SECONDS=60 \
                    KII_JSON_FIXED_TOKEN_NUM=64

$(NAME)_SOURCES := ./ebisu_sample.c \
                   ./ebisu_environment_impl.c \
                   ./ebisu/khc/src/khc.c \
                   ./ebisu/khc/src/khc_state_impl.c \
                   ./ebisu/khc/src/khc_slist.c \
                   ./ebisu/khc/src/khc_impl.c \
                    ./ebisu/kii/kii.c \
                    ./ebisu/kii/kii_api_call.c \
                    ./ebisu/kii/kii_json_wrapper.c \
                    ./ebisu/kii/kii_mqtt_task.c \
                    ./ebisu/kii/kii_object.c \
                    ./ebisu/kii/kii_object_impl.c \
                    ./ebisu/kii/kii_push.c \
                    ./ebisu/kii/kii_push_impl.c \
                    ./ebisu/kii/kii_req_impl.c \
                    ./ebisu/kii/kii_server_code.c \
                    ./ebisu/kii/kii_thing.c \
                    ./ebisu/kii/kii_thing_impl.c \
                    ./ebisu/kii/kii_ti.c \
                    ./ebisu/kii/kii_ti_impl.c \
                    ./ebisu/tio/command_parser.c \
                    ./ebisu/tio/tio.c \
                    ./ebisu/tio/tio_impl.c \
                    ./ebisu/jkii/src/jkii.c \
                    ./ebisu/jkii/libs/jsmn/jsmn.c

$(NAME)_INCLUDES := ./ebisu/ \
					./ebisu/khc/include/ \
					./ebisu/kii/include/ \
					./ebisu/tio/include/ \
					./ebisu/jkii/include/ \
					./ebisu/jkii/libs/jsmn/

$(NAME)_COMPONENTS := protocols/MQTT \
                      utilities/wiced_log \
                      utilities/command_console \
                      utilities/command_console/wps \
                      utilities/command_console/wifi \
                      utilities/command_console/thread \
                      utilities/command_console/ping \
                      utilities/command_console/platform \
                      utilities/command_console/tracex \
                      utilities/command_console/mallinfo

WIFI_CONFIG_DCT_H := wifi_config_dct.h

VALID_PLATFORMS := BCM94343W_AVN
