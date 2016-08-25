CMAKE_MINIMUM_REQUIRED(VERSION 3.4)
PROJECT(LuaAutoBinding)

INCLUDE_DIRECTORIES(
Header
../ThirdParty/rapidjson/include
../ThirdParty/tclap/include
)

AUX_SOURCE_DIRECTORY(Source SRC_LIST)

ADD_EXECUTABLE(lab ${SRC_LIST})