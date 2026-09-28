/**
    Copyright (c) 2021 WIZnet Co.,Ltd

    SPDX-License-Identifier: BSD-3-Clause
*/

/**
    ----------------------------------------------------------------------------------------------------
    Includes
    ----------------------------------------------------------------------------------------------------
*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/powman.h"
#include "hardware/resets.h"
#include "hardware/uart.h"
#include "hardware/watchdog.h"

#include "port_common.h"
#include "socket.h"
#include "wizchip_conf.h"
#include "wizchip_spi.h"

/**
    ----------------------------------------------------------------------------------------------------
    Macros
    ----------------------------------------------------------------------------------------------------
*/

/* Buffer */
#define ETHERNET_BUF_MAX_SIZE (1024 * 2)


/* Socket */
#define SOCKET_NTP 0


/* Port */
#define PORT_NTP 123


/* UART */
#define UART_ID uart0
#define UART_IRQ UART0_IRQ
#define BAUD_RATE_INIT 9600
#define BAUD_RATE_MDF 115200
#define POSITIONING_FREQUENCY 5
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define FORCE_PIN 14
#define STANDBY_PIN 17
#define BUFFSIZE 1100


/* NTP */
#define UNIX_INIT_MS 1780000000000
#define UNIX_NTP_DIFF_MS  2208988800000
#define NTP_LEAP 0
#define NTP_VERSION 4
#define NTP_MODE 4
#define NTP_STRATUM 1
#define NTP_PRECISION 0
#define NTP_POLL_INTERVALL 0
#define NTP_PACKET_SIZE 48
#define MICROSECOND 1000000


/* System Boot Await milliseconds*/
#define SYSTEM_BOOT_AWAIT_MS 60000

/* System Reboot Par milliseconds*/
#define SYSTEM_REBOOT_MS 86400000


/* main */

/**
    ----------------------------------------------------------------------------------------------------
    Variables
    ----------------------------------------------------------------------------------------------------
*/
/* Network */
static wiz_NetInfo g_net_info = {
    .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
    .ip = {192, 168, 1, 10},                     // IP address
    .sn = {255, 255, 255, 0},                    // Subnet Mask
    .gw = {192, 168, 1, 1},                      // Gateway
    .dns = {1, 1, 1, 2},                         // DNS server
    .lla = {
        0xfe, 0x80, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x02, 0x08, 0xdc, 0xff,
        0xfe, 0x57, 0x57, 0x25
    },             // Link Local Address
    .gua = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    },             // Global Unicast Address
    .sn6 = {
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    },             // IPv6 Prefix
    .gw6 = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    },             // Gateway IPv6 Address
    .dns6 = {
        0x20, 0x01, 0x48, 0x60,
        0x48, 0x60, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x88, 0x88
    },             // DNS6 server
    .ipmode = NETINFO_STATIC_ALL
};

static int8_t loopback_mode = 0 ;
int8_t check_loopback_mode_W6x00(){
    if (loopback_mode != AS_IPV4 && loopback_mode != AS_IPV6 && loopback_mode != AS_IPDUAL){ 
        loopback_mode = AS_IPV4 ; 
    }
    return loopback_mode;
}

/**
    ----------------------------------------------------------------------------------------------------
    Functions
    ----------------------------------------------------------------------------------------------------
*/
int c2int(uint8_t c) {
    switch (c) {
        case '0':
            return 0;
        case '1':
            return 1;
        case '2':
            return 2;
        case '3':
            return 3;
        case '4':
            return 4;
        case '5':
            return 5;
        case '6':
            return 6;
        case '7':
            return 7;
        case '8':
            return 8;
        case '9':
            return 9;
        default:
            return 100;
    }
}


int hex2int(uint8_t c) {
    switch (c) {
        case '0':
            return 0;
        case '1':
            return 1;
        case '2':
            return 2;
        case '3':
            return 3;
        case '4':
            return 4;
        case '5':
            return 5;
        case '6':
            return 6;
        case '7':
            return 7;
        case '8':
            return 8;
        case '9':
            return 9;
        case 'A':
            return 10;
        case 'B':
            return 11;
        case 'C':
            return 12;
        case 'D':
            return 13;
        case 'E':
            return 14;
        case 'F':
            return 15;
        default:
            return 100;
    }
}


uint64_t zda2utc(uint8_t* data) {
    int utc_begin_index = 0;
    int utc_end_index = 0;
    int day_begin_index = 0;
    int day_end_index = 0;
    int month_begin_index = 0;
    int month_end_index = 0;
    int year_begin_index = 0;
    int year_end_index = 0;

    bool is_zda = false;
    for (int add = 0; add < BUFFSIZE - 71; add++) {
        if (0 == strcmp(data[add], '$') &&
            0 == strcmp(data[add + 1], 'G') && 
            0 == strcmp(data[add + 2], 'P') && 
            0 == strcmp(data[add + 3], 'Z') &&
            0 == strcmp(data[add + 4], 'D') &&
            0 == strcmp(data[add + 5], 'A') &&
            0 == strcmp(data[add + 6], ',')) {
            is_zda = true;
            utc_begin_index = add + 7;
            break;
        }
    }

    if (!is_zda) {
        return UNIX_INIT_MS;
    }


    // Search index
    for (int j = utc_begin_index; j < BUFFSIZE - 71; j++) {
        if (0 == strcmp(data[j], ',')) {
            utc_end_index = j - 1;
            break;
        }
    }
    day_begin_index = utc_end_index + 2;
    day_end_index = day_begin_index + 1;
    month_begin_index = day_end_index + 2;
    month_end_index = month_begin_index + 1;
    year_begin_index = month_end_index + 2;
    year_end_index = year_begin_index + 3;

    if (0 != strcmp(data[year_end_index + 1], ',') ||
        0 != strcmp(data[year_end_index + 2], '0') ||
        0 != strcmp(data[year_end_index + 3], '0') ||
        0 != strcmp(data[year_end_index + 4], ',') ||
        0 != strcmp(data[year_end_index + 5], '0') ||
        0 != strcmp(data[year_end_index + 6], '0') ||
        0 != strcmp(data[year_end_index + 7], '*')) {
        return UNIX_INIT_MS;
    }
    

    // Checksum
    int zda_checksum = hex2int(data[year_end_index + 8]) * 10 + hex2int(data[year_end_index + 9]);
    int checksum = 0;
    for(int i = utc_begin_index; i <= year_end_index; i++) {
        if(100 == c2int(data[i])){
            continue;
        }
        checksum = checksum + c2int(data[i]);
    }

    if(zda_checksum != checksum) {
        return UNIX_INIT_MS;
    }


    // UTC
    bool is_carry = false;
    int time_h = 0;
    int time_m = 0;
    int time_s_int = 0;
    int time_s_dcm = 0;
    int date_d = 0;
    int date_m = 0;
    int date_y = 0;

    int temp_utc = 0;
    for (int k = utc_begin_index; k <= utc_end_index; k++) {
        if (0 == strcmp(data[k], '.')) {
            temp_utc = 0;
            is_carry = false;
            continue;
        }
        if (is_carry) {
            temp_utc = temp_utc * 10;
        } 

        int c_int = c2int(data[k]);
        if (100 == c_int) {
            return UNIX_INIT_MS;
        }
        temp_utc = temp_utc + c_int;
        is_carry = true;

        // UTC: H
        if (1 + utc_begin_index == k) {
            if (23 < temp_utc) {
                return UNIX_INIT_MS;
            }
            time_h = temp_utc;
            temp_utc = 0;
            is_carry = false;

        // UTC: M
        } else if (3 + utc_begin_index == k) {
            if (59 < temp_utc) {
                return UNIX_INIT_MS;
            }
            time_m = temp_utc;
            temp_utc = 0;
            is_carry = false;

        // UTC: S_int
        } else if (5 + utc_begin_index == k) {
            if (59 < temp_utc) {
                return UNIX_INIT_MS;
            }
            time_s_int = temp_utc;
            temp_utc = 0;
            is_carry = false;

        // UTC: S_dcm
        } else if (8 + utc_begin_index == k) {
            if (999 < temp_utc) {
                return UNIX_INIT_MS;
            }
            time_s_dcm = temp_utc;
            break;
        }
    }


    // DATE: year
    is_carry = false;
    int temp_y = 0;
    for(int n = year_begin_index; n <= year_end_index; n++) {
        if (is_carry) {
            temp_y = temp_y * 10;
        } 

        int c_int = c2int(data[n]);
        if (100 == c_int) {
            return UNIX_INIT_MS;
        }
        temp_y = temp_y + c_int;
        is_carry = true;
    }
    date_y = temp_y;
    
    // DATE: month
    is_carry = false;
    int temp_m = 0;
    for(int n = month_begin_index; n <= month_end_index; n++) {
        if (is_carry) {
            temp_m = temp_m * 10;
        } 

        int c_int = c2int(data[n]);
        if (100 == c_int) {
            return UNIX_INIT_MS;
        }
        temp_m = temp_m + c_int;
        is_carry = true;
    }
    date_m = temp_m;

    // DATE: day
    is_carry = false;
    int temp_d = 0;
    for(int n = day_begin_index; n <= day_end_index; n++) {
        if (is_carry) {
            temp_d = temp_d * 10;
        } 

        int c_int = c2int(data[n]);
        if (100 == c_int) {
            return UNIX_INIT_MS;
        }
        temp_d = temp_d + c_int;
        is_carry = true;
    }
    date_d = temp_d;


    // Return the GNSS time
    struct tm revision_time;
    revision_time.tm_year = date_y - 1900;
    revision_time.tm_mon = date_m - 1;
    revision_time.tm_mday = date_d;
    revision_time.tm_hour = time_h;
    revision_time.tm_min = time_m;
    revision_time.tm_sec = time_s_int;
    revision_time.tm_isdst = -1;
    time_t rv_time_t =  mktime(&revision_time);

    return (uint64_t)((uintmax_t)rv_time_t * 1000 + (time_s_dcm * 10));
}


uint8_t pow_2(int exp) {
    uint8_t r = 1;
    for (int i = 0; i < exp; i++) {
        r = r * 2;
    }
    return r;
}


void dcm2hex(uint32_t dcm, uint8_t hex[4]) {
    uint8_t hex_index = 0;
    uint8_t hex_temp = 0;
    for (int i = 0; i < 32; i++) {
        if (0 == dcm) {
            hex[hex_index] = hex_temp;
            break;
        }
        dcm = dcm * 2;
        if (1 == (dcm / 1000)) {
            if (0 == i) {
                hex_temp = hex_temp + pow_2(7);
            } else {
                hex_temp = hex_temp + pow_2(7 - (i % 8));
            }
        }
        dcm = dcm % 1000;

        if ((i + 1) % 8 == 0) {
            hex[hex_index] = hex_temp;
            hex_index++;
            hex_temp = 0;
        }
    }
}


static uint64_t uart_reads_number = 0;
void on_uart_rx() {
    while (uart_is_readable(UART_ID)) {
        size_t length = (size_t)BUFFSIZE;
        uint8_t data[BUFFSIZE];
        uint8_t* data_p;
        data_p = &data;

        uart_read_blocking(UART_ID, data_p, length);
        uint64_t revision = zda2utc(data_p);

        if ((uint64_t)UNIX_INIT_MS < revision) {
            powman_timer_set_ms(revision);
        }

        uart_reads_number++;
        if (SYSTEM_REBOOT_MS < (uart_reads_number * 1000 / POSITIONING_FREQUENCY)) {
            watchdog_enable(100, 1);
            while (1);
        }
    }
}

/**
    ----------------------------------------------------------------------------------------------------
    Main
    ----------------------------------------------------------------------------------------------------
*/
void thread_gnss() {
    /* GNSS UART setup*/
    uart_init(UART_ID, BAUD_RATE_INIT);
    gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));

    // Initialize GPIO pins
    gpio_init(STANDBY_PIN);
    gpio_set_dir(STANDBY_PIN, true);
    gpio_put(STANDBY_PIN, 0);

    gpio_init(FORCE_PIN);
    gpio_set_dir(FORCE_PIN, false);
    gpio_put(FORCE_PIN, 0);

    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, true);

    const uint8_t pcas01[] = {'$', 'P', 'C', 'A', 'S', '0', '1', ',', '5', '*', '1', '9', '\r', '\n'};
    const uint8_t* pcas01_p;
    pcas01_p = &pcas01;
    uart_write_blocking(UART_ID, pcas01_p, strlen(pcas01_p));
    sleep_ms(2000);
    uart_set_baudrate(UART_ID, BAUD_RATE_MDF);

    const uint8_t pcas04[] = {'$', 'P', 'C', 'A', 'S', '0', '4', ',', '1', '*', '1', '8', '\r', '\n'};
    const uint8_t* pcas04_p;
    pcas04_p = &pcas04;
    uart_write_blocking(UART_ID, pcas04_p, strlen(pcas04_p));
    sleep_ms(2000);

    const uint8_t pcas02[] = {'$', 'P', 'C', 'A', 'S', '0', '2', ',', '2', '0', '0', '*', '1', 'D', '\r', '\n'};
    const uint8_t* pcas02_p;
    pcas02_p = &pcas02;
    uart_write_blocking(UART_ID, pcas02_p, strlen(pcas02_p));
    sleep_ms(2000);

    powman_timer_start();
    powman_timer_set_ms(UNIX_INIT_MS);

    // Set handler
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);
}



int main() {
    /* Initialize */
    stdio_flush();
    sleep_ms(3000);

    reset_block_num(RESET_PWM);
    unreset_block_num_wait_blocking(RESET_PWM);
    reset_block_mask((1u << RESET_PWM) | (1u << RESET_ADC));
    unreset_block_mask_wait_blocking((1u << RESET_PWM) | (1u << RESET_ADC));

    stdio_init_all();
    sleep_ms(3000);

    wizchip_spi_initialize();
    wizchip_cris_initialize();
    wizchip_reset();
    wizchip_initialize();
    wizchip_check();

    /* Get network information */
    network_initialize(g_net_info);
    print_network_information(g_net_info);
    sleep_ms(3000);

    /* Get UTC from GNSS */
    multicore_fifo_clear_irq();
    multicore_reset_core1();
    multicore_launch_core1(thread_gnss);

    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    while ((uint64_t)(UNIX_INIT_MS + SYSTEM_BOOT_AWAIT_MS) > powman_timer_get_ms() ||
           SYSTEM_BOOT_AWAIT_MS > (uart_reads_number * 1000 / POSITIONING_FREQUENCY)) {
        gpio_put(25, true);
        sleep_ms(1000);
        gpio_put(25, false);
        sleep_ms(1000);
    }
    gpio_put(25, true);
    
    /* NTP */
    check_loopback_mode_W6x00();
    uint8_t status;
    static uint8_t destip[16] = {0,};
    static uint16_t destport;
    uint8_t addr_len;
    uint16_t ret;
    uint16_t received_size;
    uint8_t buf_temp[NTP_PACKET_SIZE];

    while (true) {
        getsockopt(SOCKET_NTP, SO_STATUS, &status);
        if (0 == strcmp(SOCK_UDP, &status)) {
            continue;
        }

        switch(status)
        {
        case SOCK_UDP:
            getsockopt(SOCKET_NTP, SO_RECVBUF, &received_size);
            uint64_t receive_timestamp = powman_timer_get_ms() + UNIX_NTP_DIFF_MS;
            ret = recvfrom(SOCKET_NTP, &buf_temp, received_size, (uint8_t*)&destip, (uint16_t*)&destport, &addr_len);
            if(ret <= 0) {
                return ret;
            }
            received_size = (uint16_t) ret;

            if(received_size == NTP_PACKET_SIZE) {
                uint8_t sent_message[NTP_PACKET_SIZE];
                // Header
                sent_message[0] = ((NTP_LEAP & 0x03)<<6) | ((NTP_VERSION & 0x07)<<3) | ((NTP_MODE & 0x07));
                sent_message[1] = NTP_STRATUM;
                sent_message[2] = NTP_POLL_INTERVALL;
                sent_message[3] = NTP_PRECISION;
                sent_message[12] = "G";
                sent_message[13] = "P";
                sent_message[14] = "S";
                sent_message[15] = ".";

                // receive_timestamp
                uint32_t receive_timestamp_int = receive_timestamp / 1000;
                uint32_t receive_timestamp_dcm = receive_timestamp % 1000;
                uint8_t receive_timestamp_dcm_u8[4] = {0};
                dcm2hex(receive_timestamp_dcm, receive_timestamp_dcm_u8);
                sent_message[32] = (receive_timestamp_int >> 24) & 0xFF;
                sent_message[33] = (receive_timestamp_int >> 16) & 0xFF;
                sent_message[34] = (receive_timestamp_int >> 8) & 0xFF;
                sent_message[35] = receive_timestamp_int & 0xFF;
                sent_message[36] = receive_timestamp_dcm_u8[0];
                sent_message[37] = receive_timestamp_dcm_u8[1];
                sent_message[38] = receive_timestamp_dcm_u8[2];
                sent_message[39] = receive_timestamp_dcm_u8[3];

                // reference_timestamp
                uint64_t reference_timestamp = powman_timer_get_ms() + UNIX_NTP_DIFF_MS;
                uint32_t reference_timestamp_int = reference_timestamp / 1000;
                uint32_t reference_timestamp_dcm = reference_timestamp % 1000;
                uint8_t reference_timestamp_dcm_u8[4] = {0};
                dcm2hex(reference_timestamp_dcm, reference_timestamp_dcm_u8);
                sent_message[32] = (reference_timestamp_int >> 24) & 0xFF;
                sent_message[33] = (reference_timestamp_int >> 16) & 0xFF;
                sent_message[34] = (reference_timestamp_int >> 8) & 0xFF;
                sent_message[35] = reference_timestamp_int & 0xFF;
                sent_message[36] = reference_timestamp_dcm_u8[0];
                sent_message[37] = reference_timestamp_dcm_u8[1];
                sent_message[38] = reference_timestamp_dcm_u8[2];
                sent_message[39] = reference_timestamp_dcm_u8[3];

                // origin_timestamp;
                for(int i = 0; i < 8; i++) {
                    sent_message[24 + i] = buf_temp[40 + i];
                }

                // transmit_timestamp
                uint64_t transmit_timestamp = powman_timer_get_ms() + UNIX_NTP_DIFF_MS;
                uint32_t transmit_timestamp_int = transmit_timestamp / 1000;
                uint32_t transmit_timestamp_dcm = transmit_timestamp % 1000;
                uint8_t transmit_timestamp_dcm_u8[4] = {0};
                dcm2hex(transmit_timestamp_dcm, transmit_timestamp_dcm_u8);
                sent_message[40] = (transmit_timestamp_int >> 24) & 0xFF;
                sent_message[41] = (transmit_timestamp_int >> 16) & 0xFF;
                sent_message[42] = (transmit_timestamp_int >> 8) & 0xFF;
                sent_message[43] = transmit_timestamp_int & 0xFF;
                sent_message[44] = transmit_timestamp_dcm_u8[0];
                sent_message[45] = transmit_timestamp_dcm_u8[1];
                sent_message[46] = transmit_timestamp_dcm_u8[2];
                sent_message[47] = transmit_timestamp_dcm_u8[3];
                uint16_t sent_message_len = NTP_PACKET_SIZE;
                uint16_t sent_size = 0;
                while(sent_size != sent_message_len){
                    ret = sendto(SOCKET_NTP, sent_message + sent_size, sent_message_len - sent_size, destip, destport, addr_len);
                    if(ret < 0) return ret;
                    sent_size += ret; // Don't care SOCKERR_BUSY, because it is zero.
                }
            }
            break;
        case SOCK_CLOSED:
            socket(SOCKET_NTP,Sn_MR_UDP4, PORT_NTP, SOCK_IO_NONBLOCK);
        }
    }
}
