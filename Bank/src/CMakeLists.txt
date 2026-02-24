cmake_minimum_required(VERSION 4.2)

project(BANK_Server 
	VERSION 4.20
	LANGUAGES CXX
	)

add_definitions(-DUNICODE -D_UNICODE)

add_executable(SERVER main1.cpp)

add_library(FUNS Server.cpp)

target_link_libraries(SERVER PRIVATE FUNS)


