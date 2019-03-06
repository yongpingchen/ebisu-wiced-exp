#include "wiced.h"
#include "kii_task_callback.h"
#include "khc_socket_callback.h"

typedef struct _socket_context {
    wiced_tcp_socket_t socket;
    wiced_tls_context_t tls_context;
    wiced_packet_t *packet;
    int packet_offset;
    unsigned int to_recv;
    unsigned int to_send;
    int received_all; // 0 or 1
} app_socket_context_t;

khc_sock_code_t socket_connect_cb_impl(
        void* socket_context,
        const char* host,
        unsigned int port);

khc_sock_code_t socket_send_cb_impl(
        void* socket_context,
        const char* buffer,
        size_t length,
        size_t* out_sent_length);

khc_sock_code_t socket_recv_cb_impl(
        void* socket_context,
        char* buffer,
        size_t length_to_read,
        size_t* out_actual_length);

khc_sock_code_t mqtt_socket_recv_cb_impl(
        void* socket_context,
        char* buffer,
        size_t length_to_read,
        size_t* out_actual_length);

khc_sock_code_t socket_close_cb_impl(void* socket_context);

void task_thread_function( wiced_thread_arg_t arg );

kii_task_code_t task_create_cb_impl(
        const char* name,
        KII_TASK_ENTRY entry,
        void* param,
        void* userdata);

void delay_ms_cb_impl(unsigned int msec, void* userdata);
