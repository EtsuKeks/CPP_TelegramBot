#include <tests/test_scenarios.h>

#include <catch2/catch_test_macros.hpp>

#include "telegram/command_handler.h"
#include <filesystem>

void Clear(std::string filename) {
    std::vector<std::string> lines;
    filename = std::filesystem::absolute(filename).string();

    std::ifstream in(filename);
    if (in.is_open()) {
        std::string line;
        while (std::getline(in, line)) {
            lines.push_back(line);
        }
        in.close();
    }

    for (std::string& line : lines) {
        if (line.compare(0, 5, "date=") == 0) {
            line = "";
        } else if (line.compare(0, 7, "offset=") == 0) {
            line = "";
        }
    }

    std::ofstream out(filename);
    if (out.is_open()) {
        for (const std::string& line : lines) {
            out << line << '\n';
        }

        out.close();
    }
}

void Create(std::string filename) {
    filename = std::filesystem::absolute(filename).string();
    std::ofstream output_file(filename);

    if (output_file.is_open()) {
        output_file
            << "limit=50\ntimeout=5\napi_key=6587697124:AAEVIC-iUpvYKztQcwTl5RfyfFokxWccUg4\napi_endpoint=https://api.telegram.org"
               "\nsticker1=CAACAgIAAxkBAAEK6XZlcMkMmMyxC4-"
               "sH32ggsIgesBGkwAC0SEAAoO2iUvHPZI31gnlsjME\nsticker2="
               "CAACAgIAAxkBAAEK6XhlcMkYlgd7KuG__GTGbAqXSNvELAACcjQAAlM9sEiQt2eKPj2oajME\nsticker3="
               "CAACAgIAAxkBAAEK6ztlchReOpGSLR3vIr5LmcjeZy_hmAACrRgAAplGoEpTLJl8j5RL1zME\ngif1="
               "https://c.tenor.com/lQniqWJ1zRAAAAAC/spin-monkey.gif\ngif2=https://media.tenor.com/"
               "J_PBo6NeOrUAAAAd/monkey-spin.gif\ngif3=https://media.tenor.com/OxjwgcHotRAAAAAd/"
               "monkeys-eating.gif";
        output_file.close();
    }
}

void TestSingleGetMe(std::string_view) {
    Create("/tmp/db.log");

    bool flag_created_successfully = true;
    CommandHandler client;
    try {
        client = CommandHandler("/tmp/db.log");
    } catch (...) {
        flag_created_successfully = false;
    }
    REQUIRE(flag_created_successfully);
    REQUIRE_NOTHROW(client.GetMe());

    Clear("/tmp/db.log");
}

void TestGetMeErrorHandling(std::string_view) {
    Create("/tmp/db.log");

    bool flag_created_successfully = true;
    CommandHandler client;
    try {
        client = CommandHandler("/tmp/db.log");
    } catch (SystemError& e) {
        flag_created_successfully = false;
    }
    REQUIRE(flag_created_successfully);

    bool first_status_correct = false;
    bool second_status_correct = false;
    bool second_ok_correct = false;

    try {
        client.GetMe();
    } catch (TelegramAPIError& e) {
        if (e.http_code == 500) {
            first_status_correct = true;
        }

    } catch (SystemError& e) {
    }

    try {
        client.GetMe();
    } catch (TelegramAPIError& e) {
        if (e.http_code == 401) {
            second_status_correct = true;
        }
        if (!e.is_ok) {
            second_ok_correct = true;
        }

    } catch (SystemError& e) {
    }

    REQUIRE(first_status_correct);
    REQUIRE(second_status_correct);
    REQUIRE(second_ok_correct);

    Clear("/tmp/db.log");
}

// Проверим только то, что нужно нам по функционалу, остальные
// проверки истекают по всей видимости из архаичности требований
void TestSingleGetUpdatesAndSendMessages(std::string_view) {
    Create("/tmp/db.log");

    bool flag_created_successfully = true;
    CommandHandler client;
    try {
        client = CommandHandler("/tmp/db.log");
    } catch (...) {
        flag_created_successfully = false;
    }
    REQUIRE(flag_created_successfully);

    bool first_four_sent_successfully = true;
    bool first_first_message_correct = true;
    bool first_second_message_correct = true;
    bool first_third_message_correct = true;
    bool first_fourth_message_correct = true;

    std::vector<CommandHandler::Update> vect;
    try {
        vect = client.GetUpdates(false, false);
        if (vect.size() != 4) {
            first_four_sent_successfully = false;
        }

        if (vect[0].chat_id != 104519755 || vect[0].date != 1510493105 ||
            vect[0].text != "/start" || vect[0].update_id != 851793506) {
            first_first_message_correct = false;
        }

        if (vect[1].chat_id != 104519755 || vect[1].date != 1510493105 || vect[1].text != "/end" ||
            vect[1].update_id != 851793507) {
            first_second_message_correct = false;
        }

        if (vect[2].chat_id != -274574250 || vect[2].date != 1510519971 ||
            vect[2].update_id != 851793507) {
            first_third_message_correct = false;
        }

        if (vect[3].chat_id != -274574250 || vect[3].date != 1510520023 ||
            vect[3].text != "/1234" || vect[3].update_id != 851793508) {
            first_fourth_message_correct = false;
        }

    } catch (...) {
        first_four_sent_successfully = false;
        first_first_message_correct = false;
        first_second_message_correct = false;
        first_third_message_correct = false;
        first_fourth_message_correct = false;
    }

    REQUIRE(first_four_sent_successfully);
    REQUIRE(first_first_message_correct);
    REQUIRE(first_second_message_correct);
    REQUIRE(first_third_message_correct);
    REQUIRE(first_fourth_message_correct);

    bool second_did_not_fall = true;

    try {
        client.SendMessage(104519755, "text", "Hi!");
    } catch (...) {
        second_did_not_fall = false;
    }

    REQUIRE(second_did_not_fall);

    bool third_did_not_fall = true;
    bool third_did_not_fall_again = true;

    try {
        client.SendMessage(104519755, "text", "Reply", true, 2);
    } catch (...) {
        third_did_not_fall = false;
    }

    try {
        client.SendMessage(104519755, "text", "Reply", true, 2);
    } catch (...) {
        third_did_not_fall_again = false;
    }

    REQUIRE(third_did_not_fall);
    REQUIRE(third_did_not_fall_again);

    Clear("/tmp/db.log");
}

void TestHandleGetUpdatesOffset(std::string_view) {
    Clear("/tmp/db.log");
    Create("/tmp/db.log");

    bool flag_created_successfully = true;
    CommandHandler client;
    try {
        client = CommandHandler("/tmp/db.log");
    } catch (...) {
        flag_created_successfully = false;
    }
    REQUIRE(flag_created_successfully);

    bool first_did_not_fall = true;
    bool first_correct_number = true;

    try {
        std::vector<CommandHandler::Update> vect = client.GetUpdates(true, false);
        if (vect.size() != 2) {
            first_correct_number = false;
        }
    } catch (...) {
        first_correct_number = false;
        first_did_not_fall = false;
    }

    REQUIRE(first_did_not_fall);
    REQUIRE(first_correct_number);

    bool second_did_not_fall = true;
    bool second_correct_number = true;

    try {
        std::vector<CommandHandler::Update> vect = client.GetUpdates(true, false, false, 851793508);
        if (!vect.empty()) {
            second_correct_number = false;
        }
    } catch (...) {
        second_correct_number = false;
        second_did_not_fall = false;
    }

    REQUIRE(second_did_not_fall);
    REQUIRE(second_correct_number);

    bool third_did_not_fall = true;
    bool third_correct_number = true;

    try {
        std::vector<CommandHandler::Update> vect = client.GetUpdates(true, false, false, 851793508);
        if (vect.size() != 1) {
            third_correct_number = false;
        }
    } catch (...) {
        third_correct_number = false;
        third_did_not_fall = false;
    }

    REQUIRE(third_did_not_fall);
    REQUIRE(third_correct_number);

    Clear("/tmp/db.log");
}
