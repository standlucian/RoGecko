#ifndef MESYTEC_MTDC_32_V2_H
#define MESYTEC_MTDC_32_V2_H

#include <stdint.h>

// Global definitions
#define MTDC32V2_NUM_CHANNELS 32
#define MTDC32V2_NUM_BITS 16

#define MTDC32V2_SIZE_MEMORY_DATA_WORDS 48640

#define MTDC32V2_VAL_IRQ_THRESHOLD_MAX 32768

#define MTDC32V2_LEN_EVENT_MAX 36

// Firmware
#define MTDC32V2_2_EXPECTED_FIRMWARE 0x0102

// Data structures

typedef union {
    struct {
        uint32_t data_length    :12;
        uint32_t tdc_resolution :4;
        uint32_t module_id      :8;
        uint32_t zero           :6;
        uint32_t signature      :2;
    }bits;
    uint32_t data;
} mtdc32_header_t;

//typedef union {
//    struct {
//        uint32_t signature      :2;
//        uint32_t zero           :6;
//        uint32_t module_id      :8;
//        uint32_t output_format  :1;
//        uint32_t adc_resolution :3;
//        uint32_t data_length    :12;
//    }bits;
//    uint32_t data;
//} mtdc32_header_t;


typedef union {
    struct {
        uint32_t value          :16;
        uint32_t channel        :5;
        uint32_t trigger	:1;
        uint32_t sub_signature  :8;
        uint32_t signature      :2;
    }bits;
    uint32_t data;
} mtdc32_data_t;

//typedef union {
//    struct {
//        uint32_t signature      :2;
//        uint32_t sub_signature  :9;
//        uint32_t channel        :5;
//        uint32_t zero           :1;
//        uint32_t out_of_range   :1;
//        uint32_t value          :14;
//    }bits;
//    uint32_t data;
//} mtdc32_data_t;


typedef union {
    struct {
        uint32_t timestamp      :16;
        uint32_t zero           :5;
        uint32_t sub_signature  :9;
        uint32_t signature      :2;
    } bits;
    uint32_t data;
} mtdc32_extended_timestamp_t;

//typedef union {
//    struct {
//        uint32_t signature      :2;
//        uint32_t sub_signature  :9;
//        uint32_t zero           :5;
//        uint32_t timestamp      :16;
//    } bits;
//    uint32_t data;
//} mtdc32_extended_timestamp_t;


typedef union {
    uint32_t zero;
} mtdc32_dummy_t;


typedef union {
    struct {
        uint32_t trigger_counter:30;
        uint32_t signature      :2;
    }bits;
    uint32_t data;
} mtdc32_end_of_event_t;

//typedef union {
//    struct {
//        uint32_t signature      :2;
//        uint32_t trigger_counter:30;
//    }bits;
//    uint32_t data;
//} mtdc32_end_of_event_t;


// Readout reset
// Single  -> Allow new trigger
// Mode: 1 -> check thr, set irq
// Mode: 3 -> clear BERR, allow next readout

// Multievent Mode 2 and 3:
// IRQ is set, when fill level > threshold
// IRQ is unset, when IRQ ACK or fill level < threshold


// Registers
#define MTDC32V2_DATA_FIFO          0x0000

//Address
#define MTDC32V2_ADDR_SOURCE        0x6000 // 0=board, 1=addr_reg
#define MTDC32V2_ADDR_REGISTER      0x6002 // 16 bit
#define MTDC32V2_MODULE_ID          0x6004 // 8 bit, written to header
#define MTDC32V2_SOFT_RESET         0x6008 // 1 bit, reset
#define MTDC32V2_FIRMWARE_REVISION  0x600E // e.g. 0x0110 = 1.10

// IRQ (only ROAK)
#define MTDC32V2_IRQ_LEVEL          0x6010 // 3 bit
#define MTDC32V2_IRQ_VECTOR         0x6012 // 8 bit
#define MTDC32V2_IRQ_TEST           0x6014 // init IRQ test
#define MTDC32V2_IRQ_RESET          0x6016 // reset IRQ test
#define MTDC32V2_IRQ_THRESHOLD      0x6018 // 15 bit
#define MTDC32V2_MAX_TRANSFER_DATA  0x601A // 15 bit, multivent == 3 or 0xb

// CBLT, MCST
#define MTDC32V2_CBLT_MCST_CTRL     0x6020 // 8 bit
#define MTDC32V2_CBLT_ADDRESS       0x6022 // 8 bit
#define MTDC32V2_MCST_ADDRESS       0x6024 // 8 bit

// FIFO handling
#define MTDC32V2_BUFFER_DATA_LENGTH 0x6030 // 16 bit
#define MTDC32V2_DATA_LENGTH_FORMAT 0x6032 // 2 bit
#define MTDC32V2_READOUT_RESET      0x6034 // only write
#define MTDC32V2_MULTIEVENT_MODE    0x6036 // 4 bit
#define MTDC32V2_MARKING_TYPE       0x6038 // 2 bit
#define MTDC32V2_START_ACQUISITION  0x603A // 1 bit
#define MTDC32V2_FIFO_RESET         0x603C // only write, init FIFO
#define MTDC32V2_DATA_READY         0x603E // DRDY == 1

// Operation mode
#define MTDC32V2_BANK_MODE          0x6040 // 2 bit
#define MTDC32V2_TDC_RESOLUTION     0x6042 // 5 bit
#define MTDC32V2_OUTPUT_FORMAT      0x6044 // 1 bit, 0 == mesytec

//Trigger
#define MTDC32V2_BANK0_WIN_START    0x6050 // 15 bit
#define MTDC32V2_BANK1_WIN_START    0x6052 // 15 bit
#define MTDC32V2_BANK0_WIN_WIDTH    0x6054 // 14 bit, max 16us
#define MTDC32V2_BANK1_WIN_WIDTH    0x6056 // 14 bit, max 16us
#define MTDC32V2_BANK0_TRIG_SOURCE  0x6058 // 10 bit
#define MTDC32V2_BANK1_TRIG_SOURCE  0x605A // 10 bit
#define MTDC32V2_FIRST_HIT          0x605C // 2 bit

// Inputs, outputs
#define MTDC32V2_NEGATIVE_EDGE      0x6060 // 2 bit
#define MTDC32V2_ECL_TERMINATED     0x6062 // 3 bit, switch off, when inputs unused
#define MTDC32V2_ECL_TRIG1_OSC      0x6064 // 1 bit, 0 == trig, 1 == osc (0x6096!)
#define MTDC32V2_ECL_OUT_CONFIG     0x6066 // 4 bit
#define MTDC32V2_TRIG_SELECT        0x6068 // 1 bit, 0 == NIM, 1 == ECL
#define MTDC32V2_NIM_TRIG1_OSC      0x606A // 2 bit, 0 == trig, 1 == osc (0x6096!)
#define MTDC32V2_NIM_BUSY           0x606E // 4 bit

// Pulser and thresholds
#define MTDC32V2_PULSER_STATUS      0x6070 // 1 bit
#define MTDC32V2_PULSER_PATTERN     0x6072 // 8 bit
#define MTDC32V2_BANK0_INPUT_THR    0x6078 // 8 bit
#define MTDC32V2_BANK1_INPUT_THR    0x607A // 8 bit

// Mesytec control bus
#define MTDC32V2_RC_BUSNO           0x6080 // 2 bit, 0 == external
#define MTDC32V2_RC_MODNUM          0x6082 // 4 bit, module id [0..15]
#define MTDC32V2_RC_OPCODE          0x6084 // 7 bit
#define MTDC32V2_RC_ADDR            0x6086 // 8 bit, internal address
#define MTDC32V2_RC_DATA            0x6088 // 16 bit
#define MTDC32V2_RC_SEND_RET_STATUS 0x608A // 4 bit

// Counters:
// =========
// Read the counters in the order of low, high
// Latched at low word read
#define MTDC32V2_RESET_COUNTER_AB   0x6090 // 4 bit
#define MTDC32V2_EVENT_COUNTER_LOW  0x6092 // 16 bit
#define MTDC32V2_EVENT_COUNTER_HIGH 0x6094 // 16 bit
#define MTDC32V2_TIMESTAMP_SOURCE   0x6096 // 2 bit
#define MTDC32V2_TIMESTAMP_DIVISOR  0x6098 // 16 bit, ts = t / div, 0 == 65536
#define MTDC32V2_TIMESTAMP_CNT_L    0x609C // 16 bit
#define MTDC32V2_TIMESTAMP_CNT_H    0x609E // 16 bit

#define MTDC32V2_TIME_0             0x60A8 // 16 bit, [1 us], 48 bit total
#define MTDC32V2_TIME_1             0x60AA // 16 bit
#define MTDC32V2_TIME_2             0x60AC // 16 bit
#define MTDC32V2_STOP_COUNTER       0x60AE // 2 bit

// Multiplicity Filter:
// =========
#define MTDC32V2_HIGH_LIMIT_0             0x60B0 // 8 bit
#define MTDC32V2_LOW_LIMIT_0              0x60B2 // 8 bit
#define MTDC32V2_HIGH_LIMIT_1             0x60B4 // 8 bit
#define MTDC32V2_LOW_LIMIT_1              0x60B6 // 8 bit

// Signatures
#define MTDC32V2_SIG_DATA       0x0
#define MTDC32V2_SIG_HEADER     0x1
#define MTDC32V2_SIG_END_BERR   0x2
#define MTDC32V2_SIG_END        0x3

// Sub-signatures
#define MTDC32V2_SIG_DATA_EVENT 0x10
#define MTDC32V2_SIG_DATA_TIME  0x24
#define MTDC32V2_SIG_DATA_DUMMY 0x00

// Acquisition modes:
// ==================
// Mode: 0 -> Single event
// Mode: 1 -> Unlimited transfer, no readout reset needed (no CBLT)
// Mode: 3 -> Limited transfer until threshold, then BERR
//
// Bit 2 set: Sends EOB with SIGNATURE == 0x2, otherwise 0x3 and BERR
// Bit 3 set: Compares number of transmitted events with max_transfer_data
//            for BERR condition
#define MTDC32V2_VAL_MULTIEVENT_MODE_OFF        0x0
#define MTDC32V2_VAL_MULTIEVENT_MODE_1          0x1
#define MTDC32V2_VAL_MULTIEVENT_MODE_2          0x2
#define MTDC32V2_VAL_MULTIEVENT_MODE_3          0x3
#define MTDC32V2_VAL_MULTIEVENT_MODE_EOB_BERR   0x4
#define MTDC32V2_VAL_MULTIEVENT_MODE_MAX_DATA   0x8

// Counter reset mode
#define MTDC32V2_VAL_RST_COUNTER_AB_ALL_A       0x1
#define MTDC32V2_VAL_RST_COUNTER_AB_ALL_B       0x2
#define MTDC32V2_VAL_RST_COUNTER_AB_ALL         0x3
#define MTDC32V2_VAL_RST_COUNTER_AB_A_EXT       0xb

// Offsets
#define MTDC32V2_OFF_DATA_SIG   30

#define MTDC32V2_MSK_DATA_SIG   0x3

#define MTDC32V2_OFF_CBLT_MCST_CTRL_DISABLE_CBLT         0
#define MTDC32V2_OFF_CBLT_MCST_CTRL_ENABLE_CBLT          1
#define MTDC32V2_OFF_CBLT_MCST_CTRL_DISABLE_LAST_MODULE  2
#define MTDC32V2_OFF_CBLT_MCST_CTRL_ENABLE_LAST_MODULE   3
#define MTDC32V2_OFF_CBLT_MCST_CTRL_DISABLE_FIRST_MODULE 4
#define MTDC32V2_OFF_CBLT_MCST_CTRL_ENABLE_FIRST_MODULE  5
#define MTDC32V2_OFF_CBLT_MCST_CTRL_DISABLE_MCST         6
#define MTDC32V2_OFF_CBLT_MCST_CTRL_ENABLE_MCST          7

#define MTDC32V2_OFF_CBLT_ADDRESS 24
#define MTDC32V2_OFF_MCST_ADDRESS 24

#define MTDC32V2_OFF_ECL_TERMINATION_TRIG_0 0
#define MTDC32V2_OFF_ECL_TERMINATION_TRIG_1 1
#define MTDC32V2_OFF_ECL_TERMINATION_RES    2

// Lengths
#define MTDC32V2_LEN_DATA_SIG   2

// Data handling:
// ==============
// FIFO has 8k x 32 bit
// Event structure, maximum size of event is 34 words
//
// Event structure:
// 0 : header
// 1 : data
// ...
// n-1 : data
// n : end of event



#endif // MESYTEC_MTDC_32_V2_H
