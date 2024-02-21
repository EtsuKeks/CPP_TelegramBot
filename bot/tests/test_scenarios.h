#pragma once

#include <string_view>

void TestSingleGetMe(std::string_view url);
void TestGetMeErrorHandling(std::string_view url);
void TestSingleGetUpdatesAndSendMessages(std::string_view url);
void TestHandleGetUpdatesOffset(std::string_view url);
