#pragma once

#include "message_handlers.h"

#include <cstdlib>
#include <vector>
#include <regex>
#include <unordered_map>
#include <filesystem>

class Bot {
public:
    Bot(const std::string& file_name)
        : file_name_(file_name),
          to_stop_(true),
          message_handlers_{{"/random", new MessageHandlerRandom()},
                            {"/weather", new MessageHandlerWeather()},
                            {"/styleguide", new MessageHandlerSyleGuide()},
                            {"/empty", new MessageHandlerEmpty()},
                            {"/gif", new MessageHandlerGif(file_name_)},
                            {"/sticker", new MessageHandlerSticker(file_name_)}} {
    }

    void Run() {
        CommandHandler handler(file_name_);
        handler.GetMe();
        while (to_stop_) {
            std::vector<CommandHandler::Update> messages = handler.GetUpdates();
            for (CommandHandler::Update message : messages) {
                MessageHandlerBase* message_handler = message_handlers_.at("/empty");

                if (message_handlers_.contains(message.text)) {
                    message_handler = message_handlers_.at(message.text);
                }

                message_handler->OnMsg(message, handler);
                handler.Commit(message.update_id + 1, message.date);

                if (message.text == "/stop") {
                    to_stop_ = false;
                    break;
                } else if (message.text == "/crash") {
                    throw SystemError("Bot|Run()|Crash on purpose.\n");
                }
            }
        }
    }

    ~Bot() {
        for (std::pair<std::string, MessageHandlerBase*> entry : message_handlers_) {
            delete entry.second;
        }
    }

private:
    std::string file_name_;
    bool to_stop_;
    std::unordered_map<std::string, MessageHandlerBase*> message_handlers_;
};
