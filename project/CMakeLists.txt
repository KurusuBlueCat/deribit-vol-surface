# add_executable(step1 step1.cpp)
include_directories(Solver)
add_subdirectory(Solver)
add_subdirectory(date)

add_library(CubicSmile STATIC CubicSmile.cpp)
add_library(CsvFeeder STATIC CsvFeeder.cpp)
add_library(Msg STATIC Msg.cpp)
add_library(Date STATIC Date.cpp)

add_executable(step1 step1.cpp)

target_link_libraries(step1 CsvFeeder Msg)

add_executable(step2 step2.cpp)

target_link_libraries(step2 CsvFeeder Msg Date)

add_executable(step3 step3.cpp)

target_link_libraries(step3 CsvFeeder Msg Date CubicSmile)

add_executable(unittestsGradientMaker unittestsGradientMaker.cpp)