CMAKE_MINIMUM_REQUIRED(VERSION 3.4)
PROJECT(HeaderParser)

INCLUDE_DIRECTORIES(
Header
../ThirdParty/rapidjson/include
../ThirdParty/tclap/include
)

AUX_SOURCE_DIRECTORY(Source SRC_LIST)

ADD_EXECUTABLE(hp ${SRC_LIST})