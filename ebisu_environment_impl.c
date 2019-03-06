#include "wiced.h"
#include "kii_task_callback.h"
#include "khc_socket_callback.h"
#include "ebisu_environment_impl.h"
#include "wiced_log.h"
#include <command_console_commands.h>
khc_sock_code_t socket_connect_cb_impl(
        void* socket_context,
        const char* host,
        unsigned int port)
{
    app_socket_context_t *sock_ctx = (app_socket_context_t*)socket_context;
    wiced_ip_address_t addr;
    wiced_tls_identity_t* identity = NULL;
    wiced_result_t rc;

    rc = wiced_hostname_lookup(host, &addr, 10000, WICED_STA_INTERFACE);
    if(rc != WICED_SUCCESS) {
        return KHC_SOCK_FAIL;
    }
//    sock_ctx = malloc(sizeof(app_socket_context_t));
    rc = wiced_tcp_create_socket(&(sock_ctx->socket), WICED_STA_INTERFACE);
    if (rc != WICED_SUCCESS) {
//        free(sock_ctx);
        return KHC_SOCK_FAIL;
    }

    wiced_tls_init_context(&(sock_ctx->tls_context), identity, NULL);
    wiced_tcp_enable_tls(&(sock_ctx->socket), &(sock_ctx->tls_context));
    sock_ctx->packet = NULL;
    sock_ctx->packet_offset = 0;

    rc = wiced_tcp_connect(&(sock_ctx->socket), &addr, port, 10000);
    if (rc != WICED_SUCCESS) {
//        free(sock_ctx);
        wiced_tcp_disconnect(&(sock_ctx->socket));
        wiced_tcp_delete_socket(&(sock_ctx->socket));
        return KHC_SOCK_FAIL;
    }
    return KHC_SOCK_OK;
}

khc_sock_code_t socket_send_cb_impl(
        void* socket_context,
        const char* buffer,
        size_t length,
        size_t* out_sent_length)
{
    wiced_result_t ret;
    app_socket_context_t *context = (app_socket_context_t*)socket_context;
    ret = wiced_tcp_send_buffer(&(context->socket), buffer, length);
    if (ret == WICED_SUCCESS) {
        *out_sent_length = length;
        return KHC_SOCK_OK;
    } else {
        return KHC_SOCK_FAIL;
    }
}

khc_sock_code_t socket_recv_cb_impl(
        void* socket_context,
        char* buffer,
        size_t length_to_read,
        size_t* out_actual_length)
{
    wiced_result_t ret = WICED_SUCCESS;
    app_socket_context_t *context = (app_socket_context_t*)socket_context;
    wiced_packet_t *packet = context->packet;
    int offset = context->packet_offset;
    if (packet == NULL) {
        ret = wiced_tcp_receive(&(context->socket), &packet, 10000);
        context->received_all = 0;
        offset = 0;
    }

    if (context->received_all == 1) {
        wiced_packet_delete(packet);
        context->packet = NULL;
        context->packet_offset = 0;
        *out_actual_length = 0;
        return KHC_SOCK_OK;
    }
    if (ret == WICED_SUCCESS) {
        uint16_t        total;
        uint16_t        length;
        uint8_t*        data;

        wiced_packet_get_data(packet, offset, &data, &length, &total);
        *out_actual_length = MIN(length, length_to_read);
        memcpy(buffer, data, *out_actual_length);
        // buffer[*out_actual_length] = 0;
        offset += *out_actual_length;
        if (*out_actual_length < total) {
            context->packet = packet;
            context->packet_offset = offset;
        } else {
            context->received_all = 1;
        }
        return KHC_SOCK_OK;
    } else {
        return KHC_SOCK_FAIL;
    }
}

khc_sock_code_t mqtt_socket_recv_cb_impl(
        void* socket_context,
        char* buffer,
        size_t length_to_read,
        size_t* out_actual_length)
{
    wiced_result_t ret = WICED_SUCCESS;
    app_socket_context_t *context = (app_socket_context_t*)socket_context;
    wiced_packet_t *packet = context->packet;
    int offset = context->packet_offset;
    if (packet == NULL) {
        ret = wiced_tcp_receive(&(context->socket), &packet, 10000);
        context->received_all = 0;
        offset = 0;
    }

    if (context->received_all == 1) {

    }
    if (ret == WICED_SUCCESS) {
        uint16_t        total;
        uint16_t        length;
        uint8_t*        data;

        wiced_packet_get_data(packet, offset, &data, &length, &total);
        *out_actual_length = MIN(length, length_to_read);
        memcpy(buffer, data, *out_actual_length);
        // buffer[*out_actual_length] = 0;
        offset += *out_actual_length;
        if (*out_actual_length < total) {
            context->packet = packet;
            context->packet_offset = offset;
        } else {
            wiced_packet_delete(packet);
            context->packet = NULL;
            context->packet_offset = 0;
            return KHC_SOCK_OK;
        }
        return KHC_SOCK_OK;
    } else {
        return KHC_SOCK_FAIL;
    }
}

khc_sock_code_t socket_close_cb_impl(void* socket_context)
{
    app_socket_context_t *context = (app_socket_context_t*)socket_context;

    if (context->packet != NULL) {
        wiced_packet_delete(context->packet);
    }
    wiced_tcp_disconnect(&(context->socket));
    wiced_tcp_delete_socket(&(context->socket));
//    free(context);
//    socket_context = NULL;
    return KHC_SOCK_OK;
}

// task callbacks
typedef struct {
    wiced_thread_t thread;
    KII_TASK_ENTRY entry;
    void* param;
    void* userdata;
} task_thread_arg_t;

void task_thread_function( wiced_thread_arg_t arg ) {
    task_thread_arg_t* task_arg = (task_thread_arg_t*)arg;
    task_arg->entry(task_arg->param);
}

static task_thread_arg_t tio_task;
kii_task_code_t task_create_cb_impl(
        const char* name,
        KII_TASK_ENTRY entry,
        void* param,
        void* userdata)
{
    unsigned int stk_size = WICED_DEFAULT_APPLICATION_STACK_SIZE;
    unsigned int priority = WICED_NETWORK_WORKER_PRIORITY;
    task_thread_arg_t *task_arg = &tio_task;

    task_arg->entry = entry;
    task_arg->param = param;
    task_arg->userdata = userdata;
    if (wiced_rtos_create_thread(&(task_arg->thread), priority, name, task_thread_function, stk_size, task_arg) != WICED_SUCCESS) {
        wiced_log_printf("create thread [%s] failed.\n", name);
        return KII_TASKC_FAIL;
    } else {
        return KII_TASKC_OK;
    }
}

void delay_ms_cb_impl(unsigned int msec, void* userdata)
{
    wiced_rtos_delay_milliseconds(msec);
}
