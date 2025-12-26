#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

bool initFs();
bool isFileExists(char* filename);
DynamicJsonDocument readJsonFromFile(char* filename);
void writeJsonToFile(const char* filename, DynamicJsonDocument doc);
bool deleteFile(char* filename);