#pragma once

#include "command_handler.h"

#include <cstdlib>
#include <vector>
#include <regex>

class MessageHandlerBase {
public:
    virtual ~MessageHandlerBase() = default;
    virtual void OnMsg(const CommandHandler::Update& message, const CommandHandler& handler) = 0;
};

class MessageHandlerRandom : public MessageHandlerBase {
public:
    void OnMsg(const CommandHandler::Update& message, const CommandHandler& handler) override {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        int num = std::rand() % 100;
        handler.SendMessage(message.chat_id, "text", std::to_string(num));
    }
};

class MessageHandlerWeather : public MessageHandlerBase {
public:
    void OnMsg(const CommandHandler::Update& message, const CommandHandler& handler) override {
        handler.SendMessage(message.chat_id, "text", "Winter Is Coming.");
    }
};

class MessageHandlerSyleGuide : public MessageHandlerBase {
public:
    void OnMsg(const CommandHandler::Update& message, const CommandHandler& handler) override {
        handler.SendMessage(
            message.chat_id, "text",
            "10 lines of code = 10 issues. 50 lines of code = \"looks fine.\". Code reviews.");
    }
};

class MessageHandlerEmpty : public MessageHandlerBase {
public:
    void OnMsg(const CommandHandler::Update&, const CommandHandler&) override {
    }
};

class MessageHandlerGif : public MessageHandlerBase {
public:
    MessageHandlerGif(const std::string& file_name)
        : file_name_(std::filesystem::absolute(file_name).string()) {
    }

    void OnMsg(const CommandHandler::Update& message, const CommandHandler& handler) override {
        std::vector<std::string> gifs;
        std::regex regex("gif[0-9]+=(.*)");

        if (!std::filesystem::exists(file_name_)) {
            throw SystemError("MessageHandlerGif|OnMsg()|File \"" + file_name_ +
                              "\" doesn't exist.\n");
        }
        std::ifstream input_file(file_name_);
        if (input_file.is_open()) {
            std::string line;
            while (std::getline(input_file, line)) {
                if (std::regex_match(line, regex)) {
                    gifs.push_back(line);
                }
            }
            input_file.close();
        } else {
            throw SystemError("MessageHandlerGif|OnMsg()|File \"" + file_name_ +
                              "\" can't be opened for reading.\n");
        }

        if (gifs.empty()) {
            return;
        }

        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        std::string gif = gifs.at(std::rand() % gifs.size());
        std::smatch match;
        std::regex_match(gif, match, regex);
        if (match.size() == 2) {
            gif = match[1].str();
        }

        handler.SendMessage(message.chat_id, "gif", gif);
    }

private:
    std::string file_name_;
};

class MessageHandlerSticker : public MessageHandlerBase {
public:
    MessageHandlerSticker(const std::string& file_name)
        : file_name_(std::filesystem::absolute(file_name).string()) {
    }

    void OnMsg(const CommandHandler::Update& message, const CommandHandler& handler) override {
        std::vector<std::string> stickers;
        std::regex regex("sticker[0-9]+=(.*)");

        if (!std::filesystem::exists(file_name_)) {
            throw SystemError("MessageHandlerSticker|OnMsg()|File \"" + file_name_ +
                              "\" doesn't exist.\n");
        }
        std::ifstream input_file(file_name_);
        if (input_file.is_open()) {
            std::string line;
            while (std::getline(input_file, line)) {
                if (std::regex_match(line, regex)) {
                    stickers.push_back(line);
                }
            }
            input_file.close();
        } else {
            throw SystemError("MessageHandlerSticker|OnMsg()|File \"" + file_name_ +
                              "\" can't be opened for reading.\n");
        }

        if (stickers.empty()) {
            return;
        }

        std::srand(static_cast<unsigned int>(std::time(nullptr)));

        std::string sticker = stickers.at(std::rand() % stickers.size());
        std::smatch match;
        std::regex_match(sticker, match, regex);
        if (match.size() == 2) {
            sticker = match[1].str();
        }

        handler.SendMessage(message.chat_id, "sticker", sticker);
    }

private:
    std::string file_name_;
};
