if (APPLE)
  link_directories(/usr/local/lib)
  link_directories(/usr/local/opt/openssl/lib)

  include_directories(/usr/local/include)
  include_directories(/usr/local/opt/openssl/include)
endif()

if (TEST_SOLUTION)
  file(GLOB SOLUTION_SRC CONFIGURE_DEPENDS "../tests/bot/telegram/*.cpp")
else()
  file(GLOB SOLUTION_SRC CONFIGURE_DEPENDS "telegram/*.cpp")
endif()
add_library(telegram ${SOLUTION_SRC})
target_link_libraries(telegram PocoNet PocoNetSSL PocoFoundation PocoJSON PocoUtil)

if (TEST_SOLUTION)
  target_include_directories(telegram PUBLIC ../tests/bot)
else()
  target_include_directories(telegram PUBLIC .)
endif()

add_shad_executable(bot-run SOLUTION_SRCS telegram/main.cpp)
target_link_libraries(bot-run PRIVATE telegram)

add_executable(fake fake/fake_main.cpp fake/fake.cpp fake/fake_data.cpp)
target_link_libraries(fake PRIVATE telegram)

add_catch(test_telegram tests/test_api.cpp fake/fake.cpp
 fake/fake_data.cpp SOLUTION_SRCS tests/test_scenarios.cpp)
target_link_libraries(test_telegram PRIVATE telegram)
