#pragma once

#include <string_view>
#include <memory>

#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/ServerSocket.h>

namespace telegram {

class FakeServer {
public:
    FakeServer(std::string_view test_case);
    ~FakeServer();

    void Start();
    std::string_view GetUrl() const;
    void StopAndCheckExpectations();

private:
    void Stop();

    std::unique_ptr<class TestCase> test_case_;
    std::unique_ptr<Poco::Net::ServerSocket> socket_;
    std::unique_ptr<Poco::Net::HTTPServer> server_;
};

}  // namespace telegram
