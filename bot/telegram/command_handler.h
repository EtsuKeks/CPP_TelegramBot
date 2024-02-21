#pragma once

#include "exceptions.h"

#include <Poco/URI.h>

// Заменить всюду HTTPClientSession на HTTPSClientSession
// при запуске вне тестовой среды
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>

#include <vector>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <iostream>

class CommandHandler {
public:
    CommandHandler() {
    }

    CommandHandler(const std::string& file_name)
        : limit_(30),
          timeout_(5),
          is_starting_(true),
          file_name_(std::filesystem::absolute(file_name).string()) {
        if (!std::filesystem::exists(file_name_)) {
            throw SystemError("CommandHandler|Constructor|File \"" + file_name_ +
                              "\" doesn't exist.\n");
        }
        std::ifstream in(file_name_);
        if (!in.is_open()) {
            throw SystemError("CommandHanlder|Constructor|File \"" + file_name_ +
                              "\" can't be opened for reading.\n");
        }

        std::string line;
        std::string api_key;
        std::string api_endpoint;
        bool api_key_is_present = false;
        bool api_endpoint_is_present = false;
        bool offset_is_present = false;
        bool date_is_present = false;

        while (std::getline(in, line)) {
            std::istringstream line_stream(line);
            std::string key;

            if (std::getline(line_stream, key, '=')) {
                if (key == "limit" && (line_stream >> limit_)) {
                } else if (key == "timeout" && (line_stream >> timeout_)) {
                } else if (key == "offset" && (line_stream >> offset_)) {
                    offset_is_present = true;
                } else if (key == "date" && (line_stream >> date_)) {
                    date_is_present = true;
                } else if (key == "api_endpoint" && (line_stream >> api_endpoint)) {
                    api_endpoint_is_present = true;
                } else if (key == "api_key" && (line_stream >> api_key)) {
                    api_key_is_present = true;
                }
            }
        }

        if (!api_key_is_present) {
            throw SystemError("CommandHandler|Constructor|File \"" + file_name_ +
                              "\" incorrect: api_key is not present.\n");
        } else if (!api_endpoint_is_present) {
            throw SystemError("CommandHandler|Constructor|File \"" + file_name_ +
                              "\" incorrect: api_endpoint is not present.\n");
        } else if (offset_is_present && !date_is_present) {
            throw SystemError("CommandHandler|Constructor|File \"" + file_name_ +
                              "\" incorrect: offset is given, but date is not present.\n");
        } else if (!offset_is_present && date_is_present) {
            throw SystemError("CommandHandler|Constructor|File \"" + file_name_ +
                              "\" incorrect: date is given, but offset is not present.\n");
        } else if (offset_is_present && date_is_present) {
            is_starting_ = false;
        }

        Poco::URI uri(api_endpoint);
        uri_ = uri;
        uri_.setPath(uri_.getPath() + "/bot" + api_key);
    }

    void GetMe() const {
        Poco::URI uri_get_me(uri_);
        uri_get_me.setPath(uri_get_me.getPath() + "/getMe");

        Poco::Net::HTTPClientSession session(uri_get_me.getHost(), uri_get_me.getPort());

        // Заменить uri_get_me.getPath() на uri_get_me.toString() при запуске вне тестовой среды
        Poco::Net::HTTPRequest request("GET", uri_get_me.getPath());
        session.sendRequest(request);

        Poco::Net::HTTPResponse response;
        std::istream& body = session.receiveResponse(response);

        if (response.getStatus() / 100 != 2) {
            throw TelegramAPIError(response.getStatus(), false,
                                   "CommandHanlder|GetMe()|Response status is invalid: uri=" +
                                       uri_get_me.toString() + "\n");
        }

        Poco::JSON::Parser parser_to_json;
        Poco::JSON::Object::Ptr reply =
            parser_to_json.parse(body).extract<Poco::JSON::Object::Ptr>();

        bool is_ok = reply->get("ok").extract<bool>();
        if (!is_ok) {
            std::string description = reply->get("description").extract<std::string>();
            throw TelegramAPIError(response.getStatus(), false,
                                   "CommandHandler|GetMe()|Response status is valid, but \"ok\" is "
                                   "set to false: description=" +
                                       description + ", uri=" + uri_get_me.toString() + "\n");
        }
    }

    // Не передавать is_answer, message_id при запуске вне тестовой реализации
    void SendMessage(int64_t chat_id, const std::string& type, const std::string& msg,
                     bool is_answer = false, int64_t message_id = 0) const {
        Poco::URI uri_send_message(uri_);
        Poco::JSON::Object empty_json;

        if (type == "sticker") {
            uri_send_message.setPath(uri_send_message.getPath() + "/sendSticker");
            empty_json.set("sticker", msg);
        } else if (type == "gif") {
            uri_send_message.setPath(uri_send_message.getPath() + "/sendPhoto");
            empty_json.set("photo", msg);
        } else if (type == "text") {
            uri_send_message.setPath(uri_send_message.getPath() + "/sendMessage");
            empty_json.set("text", msg);
        } else {
            throw std::runtime_error(
                "CommandHandler|SendMessage()|\"type\" parameter is invalid: uri=" +
                uri_send_message.toString() + ", chat_id=" + std::to_string(chat_id) +
                ", type=" + type + ", msg=" + msg + "\n");
        }

        if (is_answer) {
            empty_json.set("reply_to_message_id", std::to_string(message_id));
        }

        empty_json.set("chat_id", std::to_string(chat_id));

        Poco::Net::HTTPClientSession session(uri_send_message.getHost(),
                                             uri_send_message.getPort());

        std::ostringstream json_stream;
        empty_json.stringify(json_stream);
        std::string json_string = json_stream.str();

        Poco::Net::HTTPRequest request("POST", uri_send_message.getPath());
        request.setContentType("application/json");
        request.setContentLength(json_string.length());
        session.sendRequest(request) << json_string;

        Poco::Net::HTTPResponse response;
        std::istream& body = session.receiveResponse(response);

        if (response.getStatus() / 100 != 2) {
            throw TelegramAPIError(response.getStatus(), false,
                                   "CommandHanlder|SendMessage()|Response status is invalid: uri=" +
                                       uri_send_message.toString() +
                                       ", chat_id=" + std::to_string(chat_id) + ", type=" + type +
                                       ", msg=" + msg + "\n");
        }

        Poco::JSON::Parser parser_to_json;
        Poco::JSON::Object::Ptr reply =
            parser_to_json.parse(body).extract<Poco::JSON::Object::Ptr>();

        bool is_ok = reply->get("ok").extract<bool>();
        if (!is_ok) {
            std::string description = reply->get("description").extract<std::string>();
            throw TelegramAPIError(response.getStatus(), false,
                                   "CommandHandler|SendMessage()|Response status is valid, but "
                                   "\"ok\" is set to false: description=" +
                                       description + ", uri=" + uri_send_message.toString() +
                                       ", chat_id=" + std::to_string(chat_id) + ", type=" + type +
                                       ", msg=" + msg + "\n");
        }
    }

    struct Update {
        int64_t update_id;
        int64_t chat_id;
        int64_t date;
        std::string text;
    };

    // Не передавать set_limit при запуске вне тестовой реализации
    std::vector<Update> GetUpdates(bool set_timeout = true, bool set_limit = true,
                                   bool is_offset_auto = true, int64_t offset_to_set = 0) {
        std::chrono::_V2::system_clock::time_point current_time = std::chrono::system_clock::now();
        std::chrono::seconds seconds =
            std::chrono::duration_cast<std::chrono::seconds>(current_time.time_since_epoch());
        int64_t unix_time = seconds.count();

        if (unix_time - date_ >= 5 * 24 * 60 * 60) {
            is_starting_ = true;
        }

        std::vector<Update> to_return;
        to_return.reserve(limit_);

        Poco::URI uri_get_updates(uri_);
        uri_get_updates.setPath(uri_get_updates.getPath() + "/getUpdates");

        if (!is_offset_auto) {
            uri_get_updates.addQueryParameter("offset", std::to_string(offset_to_set));
        }

        if (!is_starting_) {
            uri_get_updates.addQueryParameter("offset", std::to_string(offset_));
        }

        if (set_timeout) {
            uri_get_updates.addQueryParameter("timeout", std::to_string(timeout_));
        }

        if (set_limit) {
            uri_get_updates.addQueryParameter("limit", std::to_string(limit_));
        }

        Poco::Net::HTTPClientSession session(uri_get_updates.getHost(), uri_get_updates.getPort());

        // Заменить uri_get_updates.getPath() на uri_get_updates.toString() при запуске
        // вне тестовой среды
        Poco::Net::HTTPRequest request("GET", uri_get_updates.getPathAndQuery());
        session.sendRequest(request);

        Poco::Net::HTTPResponse response;
        std::istream& body = session.receiveResponse(response);

        if (response.getStatus() / 100 != 2) {
            throw TelegramAPIError(response.getStatus(), false,
                                   "CommandHandler|GetUpdates()|Response status is invalid: uri=" +
                                       uri_get_updates.toString() +
                                       ", offset=" + std::to_string(offset_) +
                                       ", limit=" + std::to_string(limit_) +
                                       ", timeout=" + std::to_string(timeout_) + "\n");
        }

        Poco::JSON::Parser parser_to_json;
        Poco::JSON::Object::Ptr reply =
            parser_to_json.parse(body).extract<Poco::JSON::Object::Ptr>();

        bool is_ok = reply->get("ok").extract<bool>();
        if (!is_ok) {
            std::string description = reply->get("description").extract<std::string>();
            throw TelegramAPIError(response.getStatus(), false,
                                   "CommandHandler|GetUpdates()|Response status is valid, but "
                                   "\"ok\" is set to false: description=" +
                                       description + ", uri=" + uri_get_updates.toString() +
                                       ", offset=" + std::to_string(offset_) +
                                       ", limit=" + std::to_string(limit_) +
                                       ", timeout=" + std::to_string(timeout_) + "\n");
        }

        Poco::JSON::Array::Ptr array = reply->getArray("result");
        for (std::size_t i = 0; i < array->size(); ++i) {
            Poco::JSON::Object::Ptr message = array->getObject(i);
            int64_t update_id = message->get("update_id").extract<int64_t>();
            message = message->get("message").extract<Poco::JSON::Object::Ptr>();
            int64_t chat_id = message->get("chat")
                                  .extract<Poco::JSON::Object::Ptr>()
                                  ->get("id")
                                  .extract<int64_t>();

            Poco::Dynamic::Var text = message->get("text");
            std::string to_put;
            if (message->has("text")) {
                to_put = message->get("text").extract<std::string>();
            }

            to_return.emplace_back(update_id, chat_id, message->get("date").extract<int64_t>(),
                                   to_put);
            is_starting_ = false;
        }

        return to_return;
    }

    void Commit(int64_t offset, int64_t date) {
        offset_ = offset;
        date_ = date;
        std::vector<std::string> lines;

        if (!std::filesystem::exists(file_name_)) {
            throw SystemError("CommandHandler|Commit()|File \"" + file_name_ +
                              "\" doesn't exist.\n");
        }

        std::ifstream in(file_name_);
        if (!in.is_open()) {
            throw SystemError("CommandHanlder|Commit()|File \"" + file_name_ +
                              "\" can't be opened for reading.\n");
        } else {
            std::string line;
            while (std::getline(in, line)) {
                lines.push_back(line);
            }
            in.close();
        }

        bool offset_is_present = false;
        bool date_is_present = false;
        for (std::string& line : lines) {
            if (line.compare(0, 5, "date=") == 0) {
                line = "date=" + std::to_string(date);
                date_is_present = true;
            } else if (line.compare(0, 7, "offset=") == 0) {
                line = "offset=" + std::to_string(offset);
                offset_is_present = true;
            }
        }

        std::ofstream out(file_name_);
        if (!out.is_open()) {
            throw SystemError("CommandHanlder|Commit()|File \"" + file_name_ +
                              "\" can't be opened for writing.\n");
        } else {
            for (const std::string& line : lines) {
                out << line << '\n';
            }

            if (!date_is_present) {
                out << "date=" + std::to_string(date) << '\n';
            }

            if (!offset_is_present) {
                out << "offset=" + std::to_string(offset) << '\n';
            }

            out.close();
        }
    }

private:
    int64_t offset_ = 0;
    int64_t date_ = 0;
    int64_t limit_;
    int64_t timeout_;
    bool is_starting_;
    std::string file_name_;
    Poco::URI uri_;
};
