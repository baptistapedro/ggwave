#include "ggwave/ggwave.h"

#include <driver/i2s.h>

// global GGwave instance
GGWave ggwave;

using TSample = int16_t;
const size_t kSampleSize_bytes = sizeof(TSample);

const int sampleRate = 12000;
const int samplesPerFrame = 256;

TSample sampleBuffer[samplesPerFrame];

const i2s_port_t     i2s_port    = I2S_NUM_0;
const adc_unit_t     adc_unit    = ADC_UNIT_1;
const adc1_channel_t adc_channel = ADC1_GPIO35_CHANNEL;

// i2s config for using the internal ADC
const i2s_config_t adc_i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate          = sampleRate,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_I2S_LSB,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 4,
    .dma_buf_len          = samplesPerFrame,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
};

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println(F("GGWave test for ESP32"));

    {
        Serial.println(F("Trying to create ggwave instance"));

        ggwave.setLogFile(nullptr);

        auto p = GGWave::getDefaultParameters();

        p.payloadLength   = 16;
        p.sampleRateInp   = sampleRate;
        p.sampleRateOut   = sampleRate;
        p.sampleRate      = sampleRate;
        p.samplesPerFrame = samplesPerFrame;
        p.sampleFormatInp = GGWAVE_SAMPLE_FORMAT_I16;
        p.sampleFormatOut = GGWAVE_SAMPLE_FORMAT_I16;
        p.operatingMode   = GGWAVE_OPERATING_MODE_RX | GGWAVE_OPERATING_MODE_TX | GGWAVE_OPERATING_MODE_USE_DSS | GGWAVE_OPERATING_MODE_TX_ONLY_TONES;

        GGWave::Protocols::tx().disableAll();
        //GGWave::Protocols::tx().toggle(GGWAVE_PROTOCOL_DT_NORMAL,  true);
        //GGWave::Protocols::tx().toggle(GGWAVE_PROTOCOL_DT_FAST,    true);
        //GGWave::Protocols::tx().toggle(GGWAVE_PROTOCOL_DT_FASTEST, true);
        GGWave::Protocols::tx().toggle(GGWAVE_PROTOCOL_MT_NORMAL,  true);
        GGWave::Protocols::tx().toggle(GGWAVE_PROTOCOL_MT_FAST,    true);
        GGWave::Protocols::tx().toggle(GGWAVE_PROTOCOL_MT_FASTEST, true);

        GGWave::Protocols::rx().disableAll();
        //GGWave::Protocols::rx().toggle(GGWAVE_PROTOCOL_DT_NORMAL,  true);
        //GGWave::Protocols::rx().toggle(GGWAVE_PROTOCOL_DT_FAST,    true);
        //GGWave::Protocols::rx().toggle(GGWAVE_PROTOCOL_DT_FASTEST, true);
        GGWave::Protocols::rx().toggle(GGWAVE_PROTOCOL_MT_NORMAL,  true);
        GGWave::Protocols::rx().toggle(GGWAVE_PROTOCOL_MT_FAST,    true);
        GGWave::Protocols::rx().toggle(GGWAVE_PROTOCOL_MT_FASTEST, true);

        ggwave.prepare(p);

        delay(1000);

        Serial.print(F("Instance initialized! Memory used: "));
        Serial.print(ggwave.heapSize());
        Serial.println(F(" bytes"));

        Serial.println(F("Trying to start I2S ADC"));
    }

    {
        //install and start i2s driver
        i2s_driver_install(i2s_port, &adc_i2s_config, 0, NULL);

        //init ADC pad
        i2s_set_adc_mode(adc_unit, adc_channel);

        // enable the adc
        i2s_adc_enable(i2s_port);

        Serial.println(F("I2S ADC started"));
    }
}

void loop() {
    static int nr = 0;
    static int niter = 0;

    static GGWave::TxRxData result;

    // read from i2s - the samples are 12-bit so we need to do some massaging to make them 16-bit
    {
        size_t bytes_read = 0;
        i2s_read(i2s_port, sampleBuffer, sizeof(int16_t)*samplesPerFrame, &bytes_read, portMAX_DELAY);

        int samples_read = bytes_read / sizeof(int16_t);
        if (samples_read != samplesPerFrame) {
            Serial.println("Failed to read samples");
            return;
        }

        for (int i = 0; i < samples_read; i += 2) {
            auto & s0 = sampleBuffer[i];
            auto & s1 = sampleBuffer[i + 1];

            s0 = s0 & 0x0fff;
            s1 = s1 & 0x0fff;

            s0 = s0 ^ s1;
            s1 = s0 ^ s1;
            s0 = s0 ^ s1;
        }
    }

    auto tStart = millis();
    if (ggwave.decode(sampleBuffer, samplesPerFrame*kSampleSize_bytes) == false) {
        Serial.println("Failed to decode");
    }
    auto tEnd = millis();

    if (++niter % 10 == 0) {
        // print the time it took the last decode() call to complete
        // should be smaller than samplesPerFrame/sampleRate seconds
        // for example: samplesPerFrame = 128, sampleRate = 6000 => not more than 20 ms
        Serial.println(tEnd - tStart);
        if (tEnd - tStart > 1000*(float(samplesPerFrame)/sampleRate)) {
            Serial.println(F("Warning: decode() took too long to execute!"));
        }
    }

    nr = ggwave.rxTakeData(result);
    if (nr > 0) {
        Serial.println(tEnd - tStart);
        Serial.print(F("Received data with length "));
        Serial.print(nr); // should be equal to p.payloadLength
        Serial.println(F(" bytes:"));

        Serial.println((char *) result.data());
    }

    //for (int i = 0; i < samples_read; i++) {
    //    Serial.println(samples[i]);
    //}
}