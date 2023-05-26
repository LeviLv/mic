#include <driver/i2s.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include "Audio.h"
#include "HttpReq.h"
#include "util.h"
#include <PubSubClient.h>

#define BUTTON_KEY 18
#define I2S_WS 15
#define I2S_SD 13
#define I2S_SCK 2
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE (16000)
#define I2S_SAMPLE_BITS (32)
#define I2S_READ_LEN (32 * 1024)
#define RECORD_TIME (2)
#define I2S_CHANNEL_NUM (1)
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)

const char filename[] = "/recording.wav";
const int headerSize = 44;
const char *ssid = "lu";
const char *password = "Vitaminb88";
const char *mqttServer = "192.168.2.1";
const char *mqttTopic = "ha-topic";
int mqttPort = 1883;

File file;
Audio audio(true, I2S_DAC_CHANNEL_BOTH_EN);
WiFiClient espClient;
PubSubClient client(espClient);
void setup()
{
    Serial.begin(115200);

    SPIFFSInit();
    i2sInit();

    wifiInit();

    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);

    while (!client.connect("ESP32Client"))
    {
        delay(1000);
        Serial.println("Connecting to MQTT server...");
    }
    Serial.println("Connected to MQTT server");
    client.subscribe("esp-listen");

    audio.setVolume(21);

    pinMode(BUTTON_KEY, INPUT_PULLUP);
}

uint32_t hasAudio = 0;
void loop()
{
    if (digitalRead(BUTTON_KEY) == LOW)
    {
        client.publish(mqttTopic, "speak");
        Serial.println("speaking...");
        vTaskDelay(1000);
    }

    // if (digitalRead(BUTTON_KEY) == HIGH)
    // {
    //     if (hasAudio == 1)
    //     {
    //         char *txt = sttBaidu(filename);
    //         char *result = toGpt(txt);
    //         std::string command = Util::extractString(result);
    //         // mqttClient.publish(mqttTopic, command.c_str());
    //         audio.connecttospeech(command.substr(0, command.find("${")).c_str(), "zh");
    //         hasAudio = 0;
    //     }
    // }

    if (!client.connected())
    {
        while (!client.connect("ESP32Client"))
        {
            delay(1000);
            Serial.println("Connecting to MQTT server...");
        }
        Serial.println("Connected to MQTT server");
        client.subscribe("esp-listen");
    }
    client.loop();
    audio.loop();
}

void callback(char *topic, byte *payload, unsigned int length)
{
    String message = "";
    for (int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }
    audio.connecttospeech(message.c_str(), "zh");

    Serial.println("send to llm：" + message);
    String result = toGpt(message);
    Serial.println("llm result:" + result);

    int delimiterIndex = result.indexOf("${");
    String part1 = result.substring(0, delimiterIndex);
    String part2 = result.substring(delimiterIndex + 2, result.lastIndexOf("}"));
    client.publish(mqttTopic, part2.c_str());
    audio.connecttospeech(part1.c_str(), "zh");
}

void wifiInit()
{
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("wifi conn success.");
}

void SPIFFSInit()
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS initialisation failed!");
        while (1)
            yield();
    }

    SPIFFS.remove(filename);
    file = SPIFFS.open(filename, FILE_WRITE);
    if (!file)
    {
        Serial.println("File is not available!");
    }

    byte header[headerSize];
    wavHeader(header, FLASH_RECORD_SIZE);

    file.write(header, headerSize);
    listSPIFFS();
}

void i2sInit()
{
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_PCM),
        .intr_alloc_flags = 0,
        .dma_buf_count = 32,
        .dma_buf_len = 1024,
        .use_apll = 1};

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

    const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD};

    i2s_set_pin(I2S_PORT, &pin_config);
}

void i2s_adc_data_scale(uint8_t *d_buff, uint8_t *s_buff, uint32_t len)
{
    uint32_t j = 0;
    uint32_t dac_value = 0;
    for (int i = 0; i < len; i += 4)
    {
        dac_value = ((((uint16_t)(s_buff[i + 3] & 0xf) << 8) | ((s_buff[i + 2]))));
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = dac_value * 128 / 2048;
    }
}

void i2s_adc()
{
    int flash_wr_size = 0;
    int i2s_read_len = I2S_READ_LEN;
    size_t bytes_read;
    char *i2s_read_buff = (char *)calloc(i2s_read_len, sizeof(char));
    uint8_t *flash_write_buff = (uint8_t *)calloc(i2s_read_len, sizeof(char));

    i2s_read(I2S_PORT, (void *)i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    i2s_read(I2S_PORT, (void *)i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);

    Serial.println(" start record ");
    while (flash_wr_size < FLASH_RECORD_SIZE)
    {
        i2s_read(I2S_PORT, (void *)i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
        // example_disp_buf((uint8_t *)i2s_read_buff, 32);
        i2s_adc_data_scale(flash_write_buff, (uint8_t *)i2s_read_buff, i2s_read_len);
        file.write((const byte *)flash_write_buff, i2s_read_len);
        flash_wr_size += i2s_read_len;

        ets_printf("Sound recording %u%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
        ets_printf("Never Used Stack Size: %u\n", uxTaskGetStackHighWaterMark(NULL));
    }
    file.close();
    free(i2s_read_buff);
    i2s_read_buff = NULL;
    free(flash_write_buff);
    flash_write_buff = NULL;

    listSPIFFS();
    vTaskDelete(NULL);
}

void example_disp_buf(uint8_t *buf, int length)
{
    Serial.println("======");
    for (int i = 0; i < length; i++)
    {
        printf("%02x ", buf[i]);
        if ((i + 1) % 8 == 0)
        {
            printf("\n");
        }
    }
    Serial.println("======");
}

void wavHeader(byte *header, int wavSize)
{
    header[0] = 'R';
    header[1] = 'I';
    header[2] = 'F';
    header[3] = 'F';
    unsigned int fileSize = wavSize + headerSize - 8;
    header[4] = (byte)(fileSize & 0xFF);
    header[5] = (byte)((fileSize >> 8) & 0xFF);
    header[6] = (byte)((fileSize >> 16) & 0xFF);
    header[7] = (byte)((fileSize >> 24) & 0xFF);
    header[8] = 'W';
    header[9] = 'A';
    header[10] = 'V';
    header[11] = 'E';
    header[12] = 'f';
    header[13] = 'm';
    header[14] = 't';
    header[15] = ' ';
    header[16] = 0x10;
    header[17] = 0x00;
    header[18] = 0x00;
    header[19] = 0x00;
    header[20] = 0x01;
    header[21] = 0x00;
    header[22] = 0x01;
    header[23] = 0x00;
    header[24] = 0x80;
    header[25] = 0x3E;
    header[26] = 0x00;
    header[27] = 0x00;
    header[28] = 0x00;
    header[29] = 0xFA;
    header[30] = 0x00;
    header[31] = 0x00;
    header[32] = 0x04;
    header[33] = 0x00;
    header[34] = 0x20;
    header[35] = 0x00;
    header[36] = 'd';
    header[37] = 'a';
    header[38] = 't';
    header[39] = 'a';
    header[40] = (byte)(wavSize & 0xFF);
    header[41] = (byte)((wavSize >> 8) & 0xFF);
    header[42] = (byte)((wavSize >> 16) & 0xFF);
    header[43] = (byte)((wavSize >> 24) & 0xFF);
}

void listSPIFFS(void)
{
    Serial.println(F("\r\nListing SPIFFS files:"));
    static const char line[] PROGMEM = "=================================================";

    Serial.println(FPSTR(line));
    Serial.println(F("  File name                              Size"));
    Serial.println(FPSTR(line));

    fs::File root = SPIFFS.open("/");
    if (!root)
    {
        Serial.println(F("Failed to open directory"));
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(F("Not a directory"));
        return;
    }

    fs::File file = root.openNextFile();
    while (file)
    {

        if (file.isDirectory())
        {
            Serial.print("DIR : ");
            String fileName = file.name();
            Serial.print(fileName);
        }
        else
        {
            String fileName = file.name();
            Serial.print("  " + fileName);
            int spaces = 33 - fileName.length();
            if (spaces < 1)
                spaces = 1;
            while (spaces--)
                Serial.print(" ");
            String fileSize = (String)file.size();
            spaces = 10 - fileSize.length();
            if (spaces < 1)
                spaces = 1;
            while (spaces--)
                Serial.print(" ");
            Serial.println(fileSize + " bytes");
        }

        file = root.openNextFile();
    }

    Serial.println(FPSTR(line));
    Serial.println();
    delay(1000);
}
