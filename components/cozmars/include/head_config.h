#pragma once

// Onboard I2C bus (external 4.7kΩ pullups) — ES8311, ES7210, ST7789, camera
#define I2C_ONBOARD_SDA_IO 10
#define I2C_ONBOARD_SCL_IO 13

// Extension board I2C bus (external 4.7kΩ pullups) — gyro, TOF, ESP32-C3
#define I2C_EXT_SDA_IO 11
#define I2C_EXT_SCL_IO 12

// ST7789 LCD SPI pins
#define LCD_SPI_SCLK_IO 8
#define LCD_SPI_MOSI_IO 9
#define LCD_DC_IO       2
#define LCD_CS_IO       7
#define LCD_RST_IO      1
#define LCD_BK_IO       21
#define LCD_BK_INVERT   1

// Camera DVP 8-bit interface pins (SCCB control shares onboard I2C bus — I2C_ONBOARD_SDA/SCL)
#define CAM_D0_IO   48  // Y2
#define CAM_D1_IO   17  // Y3
#define CAM_D2_IO   15  // Y4
#define CAM_D3_IO   18  // Y5
#define CAM_D4_IO   47  // Y6
#define CAM_D5_IO   41  // Y7
#define CAM_D6_IO   38  // Y8
#define CAM_D7_IO   43  // Y9
#define CAM_XCLK_IO  42  
#define CAM_PCLK_IO  39  
#define CAM_VSYNC_IO 40  
#define CAM_HREF_IO  44  
#define CAM_RESET_IO 45  
#define CAM_PWDN_IO  46

// Audio I2S pins (ES8311 DAC out + ES7210 mic in)
#define I2S_MCLK_IO  14  // master clock
#define I2S_BCLK_IO  6  // bit clock
#define I2S_LRCK_IO  5  // left/right word select
#define I2S_DOUT_IO  4  // data out → ES8311 DAC
#define I2S_DIN_IO   16  // data in  ← ES7210 ADC

#define AMP_ENABLE_IO 3  // amplifier enable


#define ES7210_I2C_ADDR 0x80  // mic ADC
#define ES8311_I2C_ADDR 0x30  // DAC + headphone out

// Audio codec config
#define AUDIO_SAMPLE_RATE        16000
#define AUDIO_BITS_PER_SAMPLE    16

#define AUDIO_INPUT_GAIN_DB     30.0   // ES7210 mic input gain (dB)
// #define AUDIO_INPUT_REF_GAIN_DB  0.0    // ES7210 echo ref gain, physical MIC3 (dB)
#define AUDIO_OUTPUT_DEFAULT_VOL 50

#define AUDIO_CODEC_DMA_DESC_NUM 6
#define AUDIO_CODEC_DMA_FRAME_NUM 240


#define TOF_I2C_ADDR 0x8  // front TOF proximity sensor
#define IMU_I2C_ADDR 0x2  // gyro sensor
// esp32c3 slave address is defined in cozmars-sub/sub_i2c_msg/sub_i2c_msg.h
