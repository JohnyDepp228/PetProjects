cmake_minimum_required(VERSION 4.2)

project(BANK_Client 
	VERSION 4.20
	LANGUAGES CXX
	)

add_definitions(-DUNICODE -D_UNICODE)

add_executable(CLIENT main2.cpp)

add_library(FUNS2 Client.cpp)

target_link_libraries(CLIENT PRIVATE FUNS2)


