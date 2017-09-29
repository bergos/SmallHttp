#include <ArduinoJson.h>
#include <SmallHttp.h>

HttpConnection connection(&Serial);

void setup() {
  Serial.begin(115200);

  while(!Serial) {
    delay(10);
  }
}

void loop() {
  if (connection.handle()) {
    HttpRequest req = connection.request;
    HttpResponse res = connection.response;

    if (req.method == "GET" && req.target == "/") {    
      StaticJsonBuffer<100> buffer;

      JsonObject& root = buffer.createObject();
      root["key"] = "value";

      String content;
      root.printTo(content);

      res.send(content);
    } else if (req.method == "PUT" && req.target == "/") {
      size_t length = req.contentLength();
      uint8_t* data = new uint8_t[length + 1];
      data[length] = 0;

      while(!req.handleContent(data, length)) {
        delay(10);
      }

      StaticJsonBuffer<100> buffer;

      JsonObject& root = buffer.parseObject((char*)data);
      String content = root["key"];

      res.send(content);

      delete[] data;
    }

    connection.releaseRequest();
  }

  delay(10);
}
