#include "SmallHttp.h"

HttpHeader::HttpHeader(String name, String value) {
  this->name = name;
  this->value = value;
  this->next = 0;
}

HttpMessage::HttpMessage(HttpConnection& connection, HttpHeader* headers) : connection(connection) {
  this->headers = headers;
}

HttpMessage::~HttpMessage() {
  release();
}

void HttpMessage::release() {
  HttpHeader* header = headers;

  while (header) {
    HttpHeader* tmp = header;

    header = header->next;

    delete tmp;
  }

  headers = 0;
}

String HttpMessage::getHeader(String name) {
  HttpHeader* header = findHeader(name);

  if (header) {
    return header->value;
  }

  return String();
}

void HttpMessage::setHeader(String name, String value) {
  HttpHeader* header = findHeader(name);

  if (header) {
    header->value = value;
  } else {
    appendHeader(name, value);
  }
}

HttpHeader* HttpMessage::appendHeader(String name, String value) {
  if (headers == 0) {
    return headers = new HttpHeader(name, value);
  }

  HttpHeader* header = headers;
          
  while(header->next) {
    header = header->next;
  }

  return header->next = new HttpHeader(name, value);
}

HttpHeader* HttpMessage::findHeader(String name) {
  HttpHeader* header = headers;

  while (header) {
    if (header->name.equalsIgnoreCase(name)) {
      return header;
    }

    header = header->next;
  }

  return 0;
}

HttpRequest::HttpRequest(HttpConnection& connection, String method, String target, HttpHeader* headers) : HttpMessage(connection, headers) {
  this->method = method;
  this->target = target;
}

void HttpRequest::release() {
  HttpMessage::release();
  
  method = "";
  target = "";
}

int HttpRequest::contentLength() {
  return getHeader("content-length").toInt();
}

HttpResponse::HttpResponse(HttpConnection& connection, int statusCode, String reasonPhrase, HttpHeader* headers) : HttpMessage(connection, headers) {
  this->statusCode = statusCode;
  this->reasonPhrase = reasonPhrase;
}

void HttpResponse::release() {
  HttpMessage::release();

  this->statusCode = 200;
  this->reasonPhrase = "OK";
}

void HttpResponse::sendHeaders() {
  connection.sendHeaders();
}

void HttpResponse::send(uint8_t* data, int size) {
  connection.send(data, size);
}

void HttpResponse::send(String data) {
  connection.send((uint8_t*)data.c_str(), data.length());
}

HttpConnection::HttpConnection(Stream* stream, int bufferSize) : stream(stream), request(*this), response(*this) {
  buffer = new char[bufferSize];
  bufferEnd = buffer + bufferSize;

  releaseRequest();
}

HttpConnection::~HttpConnection() {
  delete[] buffer;
}

void HttpConnection::releaseRequest() {
  status = established;
  bufferPos = buffer;
  contentLeft = -1;

  request.release();
  response.release();
}

bool HttpConnection::handle() {
  switch (status) {
    case established:
      return handleStartLine();

    case startLineParsed:
      return handleHeaders();

    default:
      return true;
  }
}

void HttpConnection::sendHeaders() {
  stream->print(String("HTTP/1.1 ") + String(response.statusCode) + ' ' + response.reasonPhrase + String("\r\n"));

  HttpHeader* header = response.headers;

  while(header) {
    stream->print(header->name + ": " + header->value + String("\r\n"));

    header = header->next;
  }

  stream->print("\r\n");
}

void HttpConnection::send(uint8_t* data, int size) {
  if (size > 0) {
    response.setHeader(String("content-length"), String(size));  
  }

  sendHeaders();

  if (data != 0 && size > 0) {
    stream->write(data, size);
  }
}

int HttpConnection::available() {
  if (contentLeft == -1) {
    contentLeft = request.contentLength();
  }
  
  return min(contentLeft, stream->available());
}

int HttpConnection::read() {
  if (available() == 0) {
    return -1;
  }

  contentLeft--;

  return stream->read();
}

int HttpConnection::peek() {
  if (available() == 0) {
    return -1;
  }

  return stream->peek();
}

void HttpConnection::flush() {
  stream->flush();
}

size_t HttpConnection::write(uint8_t c) {
  return stream->write(c);
}

bool HttpConnection::readLine() {
  while (stream->available()) {
    char c = (char)stream->read();

    if (bufferPos > buffer && c == '\n' && bufferPos[-1] == '\r') {
      bufferPos[-1] = 0;

      return true;
    }

    *(bufferPos++) = c;

    if (bufferPos == bufferEnd) {
      return true;
    }
  }

  return false;
}

bool HttpConnection::handleStartLine() {
  if (readLine()) {
    if (bufferPos == buffer) {
      status = bufferLimitReached;

      return false;
    }

    String startLine(buffer);

    int targetIndex = startLine.indexOf(' ');
    int versionIndex = startLine.lastIndexOf(' ');

    request.method = startLine.substring(0, targetIndex);
    request.target = startLine.substring(targetIndex + 1, versionIndex);

    headerPos = bufferPos;

    status = startLineParsed;
  }

  return false;
}

bool HttpConnection::handleHeaders() {
  if (readLine()) {
    if (bufferPos == buffer) {
      status = bufferLimitReached;

      return false;
    }

    String headerLine(headerPos);

    if (headerLine.length() == 0) {
      status = headersParsed;

      return true;
    }

    int colonIndex = headerLine.indexOf(':');

    String name = headerLine.substring(0, colonIndex);
    name.toLowerCase();

    String value = headerLine.substring(colonIndex + 1);
    value.trim();

    request.appendHeader(name, value);

    headerPos = bufferPos;
  }

  return false;
}
