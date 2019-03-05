#include "wiced.h"
#include "tio.h"
#include "ebisu_environment_impl.h"

#include "khc_socket_callback.h"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include "wiced_log.h"

#include <command_console_commands.h>


const char EX_APP_ID[] = "gd989tog1uuz";
const char EX_APP_KEY[] = "cc31a860f91e434784a0710ef5f11b17";
/* JP: "api-jp.kii.com" */
/* US: "api.kii.com" */
/* SG: "api-sg.kii.com" */
/* CN: "api-cn3.kii.com" */
const char EX_APP_SITE[] = "api-jp.kii.com";

#define EX_TO_RECV_SEC 20
#define EX_TO_SEND_SEC 20
#define EX_KEEP_ALIVE_SEC 600
#define EX_COMMAND_HANDLER_BUFF_SIZE 1024
#define EX_MQTT_BUFF_SIZE 1024

static wiced_mutex_t m_mutex;
static char line_buffer[MAX_LINE_LENGTH];
static char history_buffer_storage[MAX_LINE_LENGTH * MAX_HISTORY_LENGTH];

static int wiced_log_output_handler(WICED_LOG_LEVEL_T level, char *logmsg)
{
    write(STDOUT_FILENO, logmsg, strlen(logmsg));

    return 0;
}

static kii_bool_t action_handler(
    tio_action_t* action,
    tio_action_err_t* err,
    void* userdata)
{
    // TODO: need implementation
    return KII_TRUE;
}

static kii_bool_t custom_push_handler(
        const char* message,
        size_t message_length,
        void* user_data)
{
    return KII_TRUE;
}

static void handler_init(
        tio_handler_t* handler,
        char* http_buffer,
        int http_buffer_size,
        void* http_ssl_ctx,
        char* mqtt_buffer,
        int mqtt_buffer_size,
        void* mqtt_ssl_ctx,
        jkii_resource_t* resource)
{
    tio_handler_init(handler);

    tio_handler_set_app(handler, EX_APP_ID, EX_APP_SITE);

    tio_handler_set_cb_push(handler, custom_push_handler, &(handler->_kii));

    tio_handler_set_cb_task_create(handler, task_create_cb_impl, NULL);
    tio_handler_set_cb_delay_ms(handler, delay_ms_cb_impl, NULL);

    tio_handler_set_cb_sock_connect_http(handler, socket_connect_cb_impl, http_ssl_ctx);
    tio_handler_set_cb_sock_send_http(handler, socket_send_cb_impl, http_ssl_ctx);
    tio_handler_set_cb_sock_recv_http(handler, socket_recv_cb_impl, http_ssl_ctx);
    tio_handler_set_cb_sock_close_http(handler, socket_close_cb_impl, http_ssl_ctx);

    tio_handler_set_cb_sock_connect_mqtt(handler, socket_connect_cb_impl, mqtt_ssl_ctx);
    tio_handler_set_cb_sock_send_mqtt(handler, socket_send_cb_impl, mqtt_ssl_ctx);
    tio_handler_set_cb_sock_recv_mqtt(handler, socket_recv_cb_impl, mqtt_ssl_ctx);
    tio_handler_set_cb_sock_close_mqtt(handler, socket_close_cb_impl, mqtt_ssl_ctx);

    tio_handler_set_mqtt_to_sock_recv(handler, EX_TO_RECV_SEC);
    tio_handler_set_mqtt_to_sock_send(handler, EX_TO_SEND_SEC);

    tio_handler_set_http_buff(handler, http_buffer, http_buffer_size);
    tio_handler_set_mqtt_buff(handler, mqtt_buffer, mqtt_buffer_size);

    tio_handler_set_keep_alive_interval(handler, EX_KEEP_ALIVE_SEC);

    tio_handler_set_json_parser_resource(handler, resource);

//    tio_handler_set_cb_task_continue(handler, _handler_continue, NULL);
//    tio_handler_set_cb_task_exit(handler, _handler_exit, NULL);
}

static char handler_mqtt_buff[EX_MQTT_BUFF_SIZE];
static char handler_http_buff[EX_COMMAND_HANDLER_BUFF_SIZE];

static int onboard_command ( int argc, char *argv[] ) {
    char *vendorThingID = "";
    char *thingID = NULL;
    char *password = "";

   for (int i = 1; i < argc; ++i) {
       if (strncmp(argv[i], "--vendor-thing-id=", 18) == 0) {
           vendorThingID = argv[i] + 18;
       } else if (strncmp(argv[i], "--thing-id=", 11) == 0) {
           thingID = argv[i] + 11;
       } else if (strncmp(argv[i], "--password=", 11) == 0) {
           password = argv[i] + 11;
       }
   }
   if (vendorThingID == NULL && thingID == NULL) {
       wiced_log_printf("neither vendor-thing-id and thing-id are specified.\n");
       return ERR_CMD_OK;
   }
   if (password == NULL) {
       wiced_log_printf("password is not specified.\n");
       return ERR_CMD_OK;
   }
   if (vendorThingID != NULL && thingID != NULL) {
       wiced_log_printf("both vendor-thing-id and thing-id is specified.  either of one should be specified.\n");
       return ERR_CMD_OK;
   }

    tio_handler_t handler;
    app_socket_context_t handler_http_ctx;
    handler_http_ctx.to_recv = EX_TO_RECV_SEC;
    handler_http_ctx.to_send = EX_TO_SEND_SEC;

    app_socket_context_t handler_mqtt_ctx;
    handler_mqtt_ctx.to_recv = EX_TO_RECV_SEC;
    handler_mqtt_ctx.to_send = EX_TO_SEND_SEC;

    memset(handler_http_buff, 0x00, sizeof(char) * EX_COMMAND_HANDLER_BUFF_SIZE);
    memset(handler_mqtt_buff, 0x00, sizeof(char) * EX_MQTT_BUFF_SIZE);

    jkii_token_t handler_tokens[256];
    jkii_resource_t handler_resource = {handler_tokens, 256};

    handler_init(
            &handler,
            handler_http_buff,
            EX_COMMAND_HANDLER_BUFF_SIZE,
            &handler_http_ctx,
            handler_mqtt_buff,
            EX_MQTT_BUFF_SIZE,
            &handler_mqtt_ctx,
            &handler_resource);

    tio_code_t result = tio_handler_onboard(
            &handler,
            vendorThingID,
            password,
            NULL,
            NULL,
            NULL,
            NULL);
    if (result != TIO_ERR_OK) {
        wiced_log_printf("failed to onboard.\n");
        wiced_log_printf("status code: %d\n", khc_get_status_code(&(handler._kii._khc)));
        return ERR_CMD_OK;
    } else {
        wiced_log_printf("onboard succeed.\n");
    }
#ifdef DEBUG
    while(1) {
        wiced_rtos_delay_milliseconds(10000);
    }
#endif
    return ERR_CMD_OK;
}

static const command_t commands[] =
{
    //ALL_COMMANDS
    {"onboard", onboard_command, 2, NULL, NULL, "[--vendor-thing-id/--thing-id]=* --passwod=*", ""},
    CMD_TABLE_END
};
/******************************************************
 *               Function Definitions
 ******************************************************/
void application_start( void )
{
    wiced_result_t ret = WICED_SUCCESS;

    ret = wiced_init();
    if ( ret != WICED_SUCCESS )
    {
        wiced_log_printf("wiced_init failed.\n\n");
        return;
    }

    wiced_rtos_init_mutex(&m_mutex);
    wiced_log_init(WICED_LOG_PRINTF, wiced_log_output_handler, NULL);

    /* Disable roaming to other access points */
    wiced_wifi_set_roam_trigger( -99 ); /* -99dBm ie. extremely low signal level */

    /* Bringup the network interface */
    ret = wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );
    if ( ret != WICED_SUCCESS )
    {
        wiced_log_printf("\nNot able to join the requested AP\n\n");
        return;
    }

    command_console_init( STDIO_UART, MAX_LINE_LENGTH, line_buffer, MAX_HISTORY_LENGTH, history_buffer_storage, " " );
    console_add_cmd_table( commands );
}
