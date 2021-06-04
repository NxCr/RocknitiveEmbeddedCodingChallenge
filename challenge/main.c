#include <time.h>
#include <stdlib.h>
#include <FreeRTOS.h>
#include <task.h>
#include <message_buffer.h>
#include <string.h>
#include <challenge.h>
#include "console.h"

// oh hi, you found the emulator code.
// please don't use this as a guideline for your implementation, this is neither pretty nor robust.

static MessageBufferHandle_t message_buffer;

// called by the challenge code
void send(uint8_t* message, uint32_t length) {
    xMessageBufferSend(message_buffer, message, length, 0);
}

static void execute(
        uint8_t* request, uint32_t request_length,
        uint8_t* response, uint32_t response_length,
        uint32_t response_time) {

    for (uint32_t i = 0; i < request_length; i++) {
        receive_ISR(request[i]);
        if (i < request_length - 1) vTaskDelay(1);
    }
    if (response == NULL) return;
    uint32_t start = xTaskGetTickCount();
    uint8_t rx[16];
    size_t length = xMessageBufferReceive(message_buffer, rx, sizeof(rx), 5000);
    if (length == 0) {
        console_print("CHALLENGE ERROR: response timeout\n");
        return;
    }
    uint32_t elapsed = xTaskGetTickCount() - start;
//    console_print("response time: %u\n", elapsed);
    if (response_time > 0 && response_time != elapsed) {
        console_print("CHALLENGE ERROR: invalid response time %u ms (expected %u ms)\n", elapsed, response_time);
    }
    if (length != response_length || memcmp(rx, response, length) != 0) {
        console_print("CHALLENGE ERROR: invalid response\n");
    } else {
        console_print("CHALLENGE: response ok\n");
    }
}

static void send_empty() {
    uint8_t empty[] = { 0x10 };
    execute(empty, sizeof(empty), empty, sizeof(empty), 0);
}

static void send_add() {
    uint8_t add[] = { 0x24, rand(), rand(), rand(), rand() };
    uint16_t r = (add[1] + add[3]) * 256 + add[2] + add[4];
    uint8_t result[] = { 0x32, r >> 8, r };
    execute(add, sizeof(add), result, sizeof(result), 0);
}

static void send_delay() {
    uint8_t delay[] = { 0x43, rand() & 0x3, rand(), rand() };
    uint8_t result[] = { 0x51, delay[3] };
    execute(delay, sizeof(delay), result, sizeof(result), delay[1] * 256 + delay[2]);
}

static void send_log() {
    static char* messages[] = {
            "Hello World",
            "Whazzup?",
            "coffee time",
            "test",
            "does it work?!"
    };
    static uint8_t i = 0;
    char* msg = messages[i];
    uint8_t l = strlen(msg);
    uint8_t log[16] = { 0x60 + l };
    for (uint32_t j = 0; j < l; j++) {
        log[j + 1] = msg[j];
    }
    execute(log, l + 1, NULL, 0, 0);
    i = (i + 1) % (sizeof(messages) / sizeof(*messages));
}

static void emulator(void* argument) {
    console_print("\nCHALLENGE: test each message type\n\n");
    send_empty();
    vTaskDelay(1000);
    send_add();
    vTaskDelay(1000);
    send_delay();
    vTaskDelay(1000);
    send_log();
    vTaskDelay(1000);

    console_print("\nCHALLENGE: test random messages\n\n");
    vTaskDelay(2000);

    int i = 0;
    while (1) {
        i++;
        switch (rand() % 4) {
            default:
            case 0:
                send_empty();
                break;
            case 1:
                send_add();
                break;
            case 2:
                send_delay();
                break;
            case 3:
                send_log();
                break;
        }
        if (i < 20) {
            vTaskDelay(500);
        } else if (i == 20) {
            console_print("\nCHALLENGE: load test\n\n");
            vTaskDelay(2000);
        } else {
            vTaskDelay(10);
        }
    }
}

static void runner(void* argument) {
    challenge_run();
    console_print("CHALLENGE ERROR: main function returned\n");
    while (1) {
        vTaskDelay(portMAX_DELAY);
    }
}

int main() {
    srand(time(NULL));
    console_init();
    console_print("CHALLENGE: starting challenge\n");
    message_buffer = xMessageBufferCreate(128);
    challenge_init();
    xTaskCreate(emulator, "emulator", configMINIMAL_STACK_SIZE, NULL, 10, NULL);
    xTaskCreate(runner, "challenge", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    vTaskStartScheduler();
    return 0;
}
