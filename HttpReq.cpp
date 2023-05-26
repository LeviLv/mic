
#include <SPIFFS.h>
#include <HTTPClient.h>
#include "cJSON.h"

char *sttBaidu(const char *filename)
{
    File file;
    file = SPIFFS.open(filename, "r");
    HTTPClient http;
    http.setTimeout(15000);
    http.begin("http://vop.baidu.com/server_api?cuid=123123&token=24.208062b61f2056a7abaa1e1a7db3c348.2592000.1687105398.282335-33752582&dev_pid=1537");
    http.addHeader("Content-Type", "audio/wav;rate=16000");
    int httpResponseCode = http.sendRequest("POST", &file, file.size());
    if (httpResponseCode == 200)
    {
        String response = http.getString();
        // Serial.println(response);
        cJSON *root = cJSON_Parse(response.c_str());
        char *name = cJSON_GetObjectItem(root, "result")->child->valuestring;
        return name;
    }
    else
    {
        Serial.println("invoke baidu stt fail");
    }
    file.close();
    http.end();
    return "";
}

String toGpt(String txt)
{
    HTTPClient http;
    http.setTimeout(15000);
    http.begin("http://192.168.3.157:99/chat?str=" + txt);
    int httpResponseCode = http.GET();
    if (httpResponseCode == 200)
    {
        return http.getString();
    }
    else
    {
        Serial.println(httpResponseCode);
    }
    http.end();
    return "error";
}
