
#define I2S_MCLK_MHZ 45.158
#define SCLK_PERIOD_MCLKS 8
#define LR_PERIOD_SCLKS 32
#define SAMPLE_BITWIDTH 16

#define i2s_sample_t int16_t

#include "../i2s/i2s.h"


typedef struct app_to_i2s_rx_t {
    i2s_lrsample_t lr_sample;
    uint1_t samples_ready;
    uint1_t reset_n;
} app_to_i2s_rx_t;
typedef struct i2s_to_app_tx_t {
    uint1_t next_sample_ready;
} i2s_to_app_tx_t;
app_to_i2s_rx_t app_to_i2s_rx;
i2s_to_app_tx_t i2s_to_app_tx;

typedef struct app_to_i2s_tx_t {
    uint1_t sclk;
    uint1_t lrclk;
    uint1_t data;
} app_to_i2s_tx_t;

app_to_i2s_tx_t app_to_i2s_tx;


MAIN_MHZ(fmsynth_i2s_mac, I2S_MCLK_MHZ)
void fmsynth_i2s_mac()
{
    app_to_i2s_rx_t to_i2s_rx;
    WIRE_READ(app_to_i2s_rx_t, app_to_i2s_rx, to_i2s_rx)

    i2s_sample_stream_t sample_stream;
    sample_stream.samples = to_i2s_rx.lr_sample;

    i2s_output_t mac = i2s_tx_only(sample_stream, to_i2s_rx.samples_ready, to_i2s_rx.reset_n);

    app_to_i2s_tx_t to_i2s_tx;
    to_i2s_tx.data = mac.data;
    to_i2s_tx.sclk = mac.sclk;
    to_i2s_tx.lrclk = mac.lrclk;

    i2s_to_app_tx_t to_app_tx;
    to_app_tx.next_sample_ready = mac.next_sample_ready;

    WIRE_WRITE(app_to_i2s_tx_t, app_to_i2s_tx, to_i2s_tx)
    WIRE_WRITE(i2s_to_app_tx_t, i2s_to_app_tx, to_app_tx)
}



#pragma MAIN app
void app(uint1_t reset_n) {
    
}