#pragma once

#ifndef SCLK_PERIOD_MCLKS
# define SCLK_PERIOD_MCLKS 8
#endif
#ifndef LR_PERIOD_SCLKS
# define LR_PERIOD_SCLKS 32
#endif
#ifndef SAMPLE_BITWIDTH
# define SAMPLE_BITWIDTH 16
#endif

#include "wire.h"
#include "arrays.h"
#include "intN_t.h"
#include "uintN_t.h"

#ifndef i2s_sample_t
# define i2s_sample_t int16_t
#endif

typedef struct i2s_output_t {
    uint1_t sclk;
    uint1_t lrclk;
    uint1_t data;
    uint1_t next_sample_ready;
} i2s_output_t;

typedef struct i2s_lrsample_t {
    i2s_sample_t l_data;
    i2s_sample_t r_data;
} i2s_lrsample_t;

typedef struct i2s_sample_stream_t {
    i2s_lrsample_t samples;
    uint1_t valid;
} i2s_sample_stream_t;

typedef enum i2s_state_t {
    RL_WAIT,
    LR_WAIT,
    SAMPLE,
} i2s_state_t;

i2s_output_t i2s_tx_only(i2s_sample_stream_t samples, uint1_t reset_n) {
    static i2s_state_t state;
    static uint6_t lr_counter;
    static uint1_t lr;
    static uint1_t last_lr;
    static uint1_t curr_sample_bits[SAMPLE_BITWIDTH];
    static uint5_t curr_sample_bit_count;
    static uint1_t output_data_reg;
    static i2s_sample_stream_t input_reg;
    static uint1_t l_sample_done;
    static uint1_t r_sample_done;
    static uint3_t sclk_counter;
    static uint1_t sclk;
    
    uint1_t sclk_half_toggle = sclk_counter==((SCLK_PERIOD_MCLKS/2) - 1);
    //uint1_t sclk_rising_edge = sclk_half_toggle & (sclk == 0);
    uint1_t sclk_falling_edge = sclk_half_toggle & (sclk == 1);


    
    i2s_output_t rv;
    rv.data = output_data_reg;
    rv.next_sample_ready = !input_reg.valid;
    rv.sclk = sclk;
    rv.lrclk = lr;

    if (sclk_half_toggle) {
        sclk = !sclk;
        sclk_counter = 0;
    } else {
        sclk_counter += 1;
    }
    
    if (sclk_falling_edge) {
        if (lr_counter == ((LR_PERIOD_SCLKS/2) - 1)) {
            lr = !lr;
            lr_counter = 0;
        } else {
            lr_counter += 1;
        }

        if (state == RL_WAIT) {
            if ((last_lr == 0) & (lr == 1)) {
                if (input_reg.valid) {
                    state = SAMPLE;
                    UINT_TO_BIT_ARRAY(curr_sample_bits, SAMPLE_BITWIDTH, input_reg.samples.l_data)
                    output_data_reg = curr_sample_bits[SAMPLE_BITWIDTH-1];
                }
            }
        } else if (state == LR_WAIT) {
            output_data_reg = 0;
            if ((last_lr == 1) & (lr == 0)) {
                input_reg.valid = 0;
                state = SAMPLE;
                UINT_TO_BIT_ARRAY(curr_sample_bits, SAMPLE_BITWIDTH, input_reg.samples.r_data)
                output_data_reg = curr_sample_bits[SAMPLE_BITWIDTH-1];
            }
        } else {
            ARRAY_SHIFT_UP(curr_sample_bits, SAMPLE_BITWIDTH, 1);
            output_data_reg = curr_sample_bits[SAMPLE_BITWIDTH-1];

            if (curr_sample_bit_count == (SAMPLE_BITWIDTH-1)) {
                curr_sample_bit_count = 0;
                if (last_lr == 1) {
                    l_sample_done = 1;
                    state = LR_WAIT;
                } else {
                    r_sample_done = 1;
                    state = RL_WAIT;
                }

                if (l_sample_done & r_sample_done) {
                    l_sample_done = 0;
                    r_sample_done = 0;
                }
            } else {
                curr_sample_bit_count += 1;
            }
        }

        last_lr = lr;
    }

    if (rv.next_sample_ready) {
        input_reg = samples;
    }

    if (!reset_n) {
        sclk_counter = 0;
        sclk = 0;
        lr_counter = 0;
        lr = 0;
        state = RL_WAIT;
        last_lr = lr;
        curr_sample_bit_count = 0;
        input_reg.valid = 0;
        l_sample_done = 0;
        r_sample_done = 0;
        output_data_reg = 0;
    }


    return rv;
}