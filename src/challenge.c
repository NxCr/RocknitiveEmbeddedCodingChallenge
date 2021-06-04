#include <FreeRTOS.h>
#include <task.h>
#include <console.h>
#include "stream_buffer.h"
#include "portmacro.h"
#include "timers.h"


#define CC_CIRC_BUF_LEN 256
static uint8_t circ_buf_data[CC_CIRC_BUF_LEN];
static StaticStreamBuffer_t circ_buf_struct;
static StreamBufferHandle_t circ_buf_handle;


/**
 * Call this to "send" data over the (simulated) serial interface.
 * @param message message buffer
 * @param length length of the message
 */
void send(uint8_t* message, uint32_t length);

size_t has_sent = 0;
BaseType_t higherPriorityTask = pdFALSE;
/**
 * This will get called for each byte of data received.
 * @param data received byte
 */
void receive_ISR(uint8_t data) {
    has_sent = xStreamBufferSendFromISR(circ_buf_handle, &data, 1, &higherPriorityTask);
    if (!has_sent) {
        // error case.
    }
    portYIELD_FROM_ISR(higherPriorityTask);
}

/**
 * Initialize challenge. This is called once on startup, before any interrupts can occur.
 */
void challenge_init() {
    // initialize circ buf for message passing ISR->main task
    circ_buf_handle = xStreamBufferCreateStatic(CC_CIRC_BUF_LEN, 1, circ_buf_data, &circ_buf_struct);
}



typedef enum {
    cc_message_type_EMPTY   = 1,
    cc_message_type_ADD     = 2,
    cc_message_type_RESULT  = 3,
    cc_message_type_DELAY   = 4,
    cc_message_type_TIMEOUT = 5,
    cc_message_type_LOG     = 6,
} cc_message_type;


typedef struct {
    union {
        uint8_t header;
        struct {
            uint8_t len : 4;
            uint8_t type : 4;
        };
    };
    uint8_t value[];
} protocol_message;

typedef struct {
    TickType_t delay_over;
    uint8_t payload;
} cc_timeout;

typedef enum {
    task_receive_state_WAITING_FOR_HEADER,
    task_receive_state_WAITING_FOR_DATA,
    task_receive_state_EXECUTING
} task_receive_state;

static void print_message(protocol_message* msg){
    console_print("message: type: %02x - len: %02x - values: ", msg->type, msg->len);
    for (int i = 0; i<msg->len; i++){
        console_print("%02x ", msg->value[i]);
    }
    console_print("\n");
}

/**
 * Main function for the coding challenge. Must not return.
 *
 * Please use the following API functions if necessary:
 *
 * print string to stdout
 * console_print("format %d", 123);
 *
 * millisecond delay
 * vTaskDelay(123);
 *
 * get elapsed milliseconds
 * TickType_t ms = xTaskGetTickCount();
 */
void challenge_run() {
    uint8_t task_buffer[16]; // 16 is max size of one message, as header + 4 bit length can be max 16
    cc_timeout timeouts[8] = {0};
    protocol_message* msg = (protocol_message*)task_buffer;
    size_t received;
    size_t expecting;
    task_receive_state current_task_state;
    uint8_t* write_ptr = task_buffer;
    TickType_t current_time;
    uint8_t timeout_buf[2];
    protocol_message* timeout_msg = (protocol_message*)timeout_buf;
    while(1){
        current_time = xTaskGetTickCount();
        switch(current_task_state){
            case task_receive_state_WAITING_FOR_HEADER:{
                // receive header (type+len)
                received = xStreamBufferReceive(circ_buf_handle, write_ptr, 1, 0);
                if (received == 1) {
                    // advance pointer and prepare for payload recv
                    expecting = msg->len;
                    write_ptr += 1;
                    received = 0;
                    current_task_state = task_receive_state_WAITING_FOR_DATA;
                } else break;
            }
            case task_receive_state_WAITING_FOR_DATA:{
                received = xStreamBufferReceive(circ_buf_handle, write_ptr, expecting, 0);
                expecting -= received;
                if (expecting > 0){ // still expecting data
                    write_ptr += received;
                    break;
                } else {
                    write_ptr = task_buffer; // reset pointer
                    current_task_state = task_receive_state_EXECUTING; // advance state
                }
            }
            case task_receive_state_EXECUTING:{
                console_print("received ");
                print_message(msg);
                current_task_state = task_receive_state_WAITING_FOR_HEADER;
                received = 0;
                write_ptr = task_buffer;
                expecting = 0;
                switch(msg->type){
                    case cc_message_type_EMPTY:{
                        send(task_buffer, msg->len+1);
                        console_print("sent ");
                        print_message(msg);
                    } break;
                    case cc_message_type_ADD:{
                        uint16_t a, b;
                        msg->type = cc_message_type_RESULT;
                        msg->len = 2;
                        a = __builtin_bswap16(*(uint16_t*)&msg->value[0]);
                        b = __builtin_bswap16(*(uint16_t*)&msg->value[2]);
                        *((uint16_t*)&msg->value[0])=__builtin_bswap16(a+b);
                        send(task_buffer, msg->len+1);
                        console_print("sent ");
                        print_message(msg);
                    } break;
                    case cc_message_type_DELAY:{
                        // function docs say ticks are 1 ms so I'll roll with it
                        TickType_t delay_time_ms = (TickType_t)__builtin_bswap16(*(uint16_t*)&msg->value[0]);
                        for(int i = 0; i<8; i++){
                            if (timeouts[i].delay_over == 0){
                                current_time = xTaskGetTickCount();
                                timeouts[i].delay_over = current_time + delay_time_ms;
                                timeouts[i].payload = msg->value[2];
                                break;
                            }
                        }
                    } break;
                    case cc_message_type_LOG:{
                        console_print("Log: %.*s\n", msg->len, (char*)msg->value);
                    } break;
                    case cc_message_type_RESULT:
                    case cc_message_type_TIMEOUT:
                    default: {
                        console_print("Received err type: %u\n", msg->type);

                    } break;
                }
            }
        }
        for(int i = 0; i<8; i++){
            if (timeouts[i].delay_over != 0 && timeouts[i].delay_over <= current_time){
                timeout_msg->type = cc_message_type_TIMEOUT;
                timeout_msg->len = 1;
                timeout_msg->value[0] = timeouts[i].payload;
                timeouts[i].delay_over = 0;
                console_print("sent ");
                print_message(timeout_msg);
                send(timeout_buf, 2);
            }
        }
    }
}