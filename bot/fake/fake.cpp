#include "fake.h"
#include "fake_data.h"

#include <mutex>
#include <iostream>
#include <stdexcept>
#include <sstream>

#include <Poco/URI.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/JSON/Parser.h>

namespace telegram {

using Request = Poco::Net::HTTPServerRequest;
using Response = Poco::Net::HTTPServerResponse;

class CheckFailedException : public std::exception {};

class TestCase {
public:
    virtual ~TestCase() = default;
    virtual void HandleRequest(Request& request, Response& response) = 0;

    void Check() const {
        std::ostringstream errors;
        auto fail = false;
        for (size_t i = fulfilled_; i < expectations_.size(); ++i) {
            fail = true;
            errors << "Expectation not satisfied: " << expectations_[i] << std::endl;
        }
        for (const auto& error : fails_) {
            fail = true;
            errors << "Error encountered: " << error << std::endl;
        }
        if (fail) {
            throw std::runtime_error{errors.str()};
        }
    }

    void Fail(const std::string& message) {
        fails_.push_back(message);
        throw CheckFailedException{};
    }

protected:
    void ExpectURI(Request& request, const std::string& uri) {
        auto req_uri = Poco::URI{request.getURI()};
        auto req_query = req_uri.getQueryParameters();
        auto compare_uri = Poco::URI{uri};
        auto compare_query = compare_uri.getQueryParameters();

        std::ranges::sort(req_query);
        std::ranges::sort(compare_query);

        if (req_uri.getHost() != compare_uri.getHost()) {
            Fail("Invalid Host: expected " + compare_uri.getHost() + ", got " + req_uri.getHost());
        }
        if (req_uri.getPath() != compare_uri.getPath()) {
            Fail("Invalid Path: expected " + compare_uri.getPath() + ", got " + req_uri.getPath());
        }
        if (req_query != compare_query) {
            Fail("Invalid Query params");
        }
    }

    void ExpectMethod(Request& request, const std::string& method) {
        if (request.getMethod() != method) {
            Fail("Invalid method: expected " + method + ", got " + request.getMethod());
        }
    }

    std::vector<std::string> expectations_;
    int fulfilled_ = 0;

private:
    std::vector<std::string> fails_;
};

class SingleGetMeTestCase : public TestCase {
public:
    SingleGetMeTestCase() {
        expectations_ = {"Client sends getMe request"};
    }

    void HandleRequest(Request& request, Response& response) override {
        ExpectURI(request, "/bot123/getMe");
        ExpectMethod(request, "GET");
        if (++fulfilled_ == 1) {
            response.setStatus(Response::HTTP_OK);
            response.send() << fake_data::kGetMeJson;
        } else {
            Fail("Unexpected extra request");
        }
    }
};

class ErrorHandlingTestCase : public TestCase {
public:
    ErrorHandlingTestCase() {
        expectations_ = {"Client sends getMe request and receives Internal Server error",
                         "Client sends getMe request and receives error json"};
    }

    void HandleRequest(Request& request, Response& response) override {
        ExpectURI(request, "/bot123/getMe");
        ExpectMethod(request, "GET");
        if (++fulfilled_ == 1) {
            response.setStatus(Response::HTTP_INTERNAL_SERVER_ERROR);
            response.send() << "Internal server error";
        } else if (fulfilled_ == 2) {
            response.setStatus(Response::HTTP_UNAUTHORIZED);
            response.send() << fake_data::kGetMeErrorJson;
        } else {
            Fail("Unexpected extra request");
        }
    }
};

class GetUpdatesAndSendMessagesTestCase : public TestCase {
public:
    GetUpdatesAndSendMessagesTestCase() {
        expectations_ = {"Client sends getUpdates request", "Client sends message \"Hi!\"",
                         "Client sends reply \"Reply\"", "Client sends reply \"Reply\""};
    }

    void HandleRequest(Request& request, Response& response) override {
        auto check_content_type = [&] {
            if (request.get("Content-Type") != "application/json") {
                Fail("Content-Type header is not set");
            }
        };

        int64_t chat_id;
        std::string text;
        auto has_reply_to_message_id = false;
        int64_t reply_to_message_id;
        auto parse_json = [&] {
            Poco::JSON::Parser parser;
            auto body = parser.parse(request.stream());
            auto message = body.extract<Poco::JSON::Object::Ptr>();

            chat_id = message->getValue<int64_t>("chat_id");
            text = message->getValue<std::string>("text");

            if (message->has("reply_to_message_id")) {
                has_reply_to_message_id = true;
                reply_to_message_id = message->getValue<int64_t>("reply_to_message_id");
            }
        };

        if (++fulfilled_ == 1) {
            ExpectURI(request, "/bot123/getUpdates");
            ExpectMethod(request, "GET");
            response.setStatus(Response::HTTP_OK);
            response.send() << fake_data::kGetUpdatesFourMessagesJson;
        } else if (fulfilled_ == 2) {
            ExpectURI(request, "/bot123/sendMessage");
            ExpectMethod(request, "POST");
            check_content_type();
            parse_json();

            if (text != "Hi!") {
                Fail("Invalid text in message #1");
            }
            if (chat_id != 104519755) {
                Fail("Invalid chat_id in message #1");
            }

            response.setStatus(Response::HTTP_OK);
            response.send() << fake_data::kSendMessageHiJson;
        } else if (fulfilled_ == 3 || fulfilled_ == 4) {
            ExpectURI(request, "/bot123/sendMessage");
            ExpectMethod(request, "POST");
            check_content_type();
            parse_json();

            if (text != "Reply") {
                Fail("Invalid text in reply message");
            }
            if (chat_id != 104519755) {
                Fail("Invalid chat id in reply message");
            }
            if (!has_reply_to_message_id || reply_to_message_id != 2) {
                Fail("reply_to_message_id field is incorrect");
            }

            response.setStatus(Response::HTTP_OK);
            response.send() << fake_data::kSendMessageReplyJson;
        } else {
            Fail("Unexpected extra request");
        }
    }
};

class HandleOffsetTestCase : public TestCase {
public:
    HandleOffsetTestCase() {
        expectations_ = {"Client sends request and receives 2 messages",
                         "Client sends request with correct offset and receives 0 messages",
                         "Client sends request with current offset and receives 1 message"};
    }

    void HandleRequest(Request& request, Response& response) override {
        if (++fulfilled_ == 1) {
            ExpectURI(request, "/bot123/getUpdates?timeout=5");
            ExpectMethod(request, "GET");
            response.setStatus(Response::HTTP_OK);
            response.send() << fake_data::kGetUpdatesTwoMessages;
        } else if (fulfilled_ == 2) {
            ExpectURI(request, "/bot123/getUpdates?offset=851793508&timeout=5");
            ExpectMethod(request, "GET");
            response.setStatus(Response::HTTP_OK);
            response.send() << fake_data::kGetUpdatesZeroMessages;
        } else if (fulfilled_ == 3) {
            ExpectURI(request, "/bot123/getUpdates?offset=851793508&timeout=5");
            ExpectMethod(request, "GET");
            response.setStatus(Response::HTTP_OK);
            response.send() << fake_data::kGetupdatesOneMessage;
        } else {
            Fail("Unexpected extra request");
        }
    }
};

class FakeHandler : public Poco::Net::HTTPRequestHandler {
public:
    FakeHandler(TestCase* test_case) : test_case_{test_case} {
    }

    void handleRequest(Request& request, Response& response) override {
        std::lock_guard guard{mutex_};
        try {
            test_case_->HandleRequest(request, response);
        } catch (const CheckFailedException& e) {
            response.setStatus(Response::HTTP_BAD_REQUEST);
            response.send();
        } catch (const std::exception& e) {
            test_case_->Fail(e.what());
            throw;
        }
    }

private:
    std::mutex mutex_;
    TestCase* test_case_;
};

class FakeHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
public:
    FakeHandlerFactory(TestCase* test_case) : test_case_{test_case} {
    }
    Poco::Net::HTTPRequestHandler* createRequestHandler(const Request&) override {
        return new FakeHandler{test_case_};
    }

private:
    TestCase* test_case_;
};

FakeServer::FakeServer(std::string_view test_case) {
    if (test_case == "Single getMe") {
        test_case_ = std::make_unique<SingleGetMeTestCase>();
    } else if (test_case == "getMe error handling") {
        test_case_ = std::make_unique<ErrorHandlingTestCase>();
    } else if (test_case == "Single getUpdates and send messages") {
        test_case_ = std::make_unique<GetUpdatesAndSendMessagesTestCase>();
    } else if (test_case == "Handle getUpdates offset") {
        test_case_ = std::make_unique<HandleOffsetTestCase>();
    } else {
        throw std::runtime_error{"Unknown test case name " + std::string{test_case}};
    }
}

FakeServer::~FakeServer() {
    Stop();
}

void FakeServer::Start() {
    Poco::Net::SocketAddress address{"localhost", 8080};
    socket_ = std::make_unique<Poco::Net::ServerSocket>(address);
    auto factory = std::make_unique<FakeHandlerFactory>(test_case_.get());
    auto* params = new Poco::Net::HTTPServerParams;
    server_ = std::make_unique<Poco::Net::HTTPServer>(factory.release(), *socket_, params);
    server_->start();
}

std::string_view FakeServer::GetUrl() const {
    return "http://localhost:8080/";
}

void FakeServer::Stop() {
    if (server_) {
        server_->stop();
        server_.reset();
        socket_.reset();
    }
}

void FakeServer::StopAndCheckExpectations() {
    Stop();
    test_case_->Check();
}

}  // namespace telegram
