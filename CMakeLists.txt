cmake_minimum_required(VERSION 2.8)

project(cbor C)

set(CBOR_SRC
  cbor.c
  json.c)


add_library(${CMAKE_PROJECT_NAME} STATIC ${CBOR_SRC})

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY C_STANDARD 11)
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY C_STANDARD_REQUIRED ON)
