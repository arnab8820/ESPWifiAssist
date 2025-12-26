#include "file-manager.h"

bool initFs(){
    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed!");
        return false;
    }
    Serial.println("LittleFS mounted successfully!");
    return true;
}

bool isFileExists(char* filename) {
    return LittleFS.exists(filename);
}

DynamicJsonDocument readJsonFromFile(char* filename) {
    File jsonFile = LittleFS.open(filename, "r");
    DynamicJsonDocument doc(1024); // Adjust the size as needed
    if (!jsonFile) {
        Serial.print("Failed to open file ");
        Serial.print(filename);
        Serial.println(" for reading");
        return doc; // Return an empty document if the file doesn't exist
    }

    // Read the file content into a String
    String jsonString = jsonFile.readString();
    jsonFile.close();

    // Parse the JSON string
    DeserializationError error = deserializeJson(doc, jsonString.c_str());

    if (error) {
        Serial.print(F("Failed to parse JSON: "));
        Serial.println(error.f_str());
        return doc;
    }

    return doc;
}

void writeJsonToFile(const char* filename, DynamicJsonDocument doc) {
    // Serialize the JSON object to a String
    String jsonString;
    serializeJson(doc, jsonString);

    // Open the file for writing
    File jsonFile = LittleFS.open(filename, "w");
    if (!jsonFile) {
        Serial.print("Failed to open file ");
        Serial.print(filename);
        Serial.println(" for writing");
        return;
    }

    // Write the JSON string to the file
    jsonFile.print(jsonString);
    jsonFile.close();
}

bool deleteFile(char* filename) {
    if (LittleFS.remove(filename)) {
        Serial.print("File ");
        Serial.print(filename);
        Serial.println(" deleted successfully");
        return true;
    } else {
        Serial.print("Failed to delete file ");
        Serial.println(filename);
        return false;
    }
}