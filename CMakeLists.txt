cmake_minimum_required(VERSION 2.8)

project(cbor C)

set(CBOR_SRC
  cbor.c
  pointer.c
  json.c)


add_library(${PROJECT_NAME} STATIC ${CBOR_SRC})

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 11)
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD_REQUIRED ON)
