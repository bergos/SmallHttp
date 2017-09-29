#ifndef SMALLHTTP_H_
#define SMALLHTTP_H_

#include <Arduino.h>

class HttpConnection;
class HttpHeader;
class HttpMessage;
class HttpRequest;
class HttpResponse;

class HttpHeader {
  public:
    String name;
    String value;
    HttpHeader *next;

    HttpHeader(String name, String value);
};

class HttpMessage {
  public:
    HttpConnection& connection;
    HttpHeader* headers;

    HttpMessage(HttpConnection& connection, HttpHeader* headers=0);
    virtual ~HttpMessage();

    virtual void release();

    void setHeader(String name, String value);
    String getHeader(String name);

    HttpHeader* appendHeader(String name, String value);
    HttpHeader* findHeader(String name);
};

class HttpRequest : public HttpMessage {
  public:
    String method;
    String target;

    HttpRequest(HttpConnection& connection, String method=String(), String target=String(), HttpHeader* headers=0);

    virtual void release();

    int contentLength();
};

class HttpResponse : public HttpMessage {
  public:
    int statusCode;
    String reasonPhrase;

    HttpResponse(HttpConnection& connection, int statusCode=200, String reasonPhrase="OK", HttpHeader* headers=0);

    virtual void release();

    void sendHeaders();

    void send(uint8_t* data=0, int size=0);
    void send(String data);
};

class HttpConnection : public Stream {
  public:
    Stream* stream;
    HttpRequest request;
    HttpResponse response;

    HttpConnection(Stream* stream, int bufferSize=1024);
    ~HttpConnection();

    virtual void releaseRequest();

    // request

    virtual bool handle();

    // response

    virtual void sendHeaders();

    virtual void send(uint8_t* data, int size);

    // stream
    virtual int available();
    virtual int read();
    virtual int peek();
    virtual void flush();

    virtual size_t write(uint8_t c);

  protected:
    enum Status {
      established,
      startLineParsed,
      headersParsed,
      bufferLimitReached
    };

    Status status;

    char* buffer;
    char* bufferEnd;
    char* bufferPos;

    char* headerPos;

    int contentLeft;

    bool readLine();

    bool handleStartLine();
    bool handleHeaders();
};

#endif  // SMALLHTTP_H_
