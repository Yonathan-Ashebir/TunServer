//
// Created by yoni_ash on 5/27/23.
//
#include <ArduinoJson.h>
#include <iostream>

using namespace std;

void parserTest() {
    StaticJsonDocument<200> doc;

    char json[] =
            "{\"sensor\":\"gps\",\"time\":1351824120,\"data\":[48.756080,2.302038]}";

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, json);

    // Test if parsing succeeds.
    // Fetch values.
    //
    // Most of the time, you can rely on the implicit casts.
    // In other case, you can do doc["time"].as<long>();
    const char *sensor = doc["sensor"];
    long time = doc["time"];
    double latitude = doc["data"][0];
    double longitude = doc["data"][1];

    // Print values.
    cout << sensor << endl;
    cout << time << endl;
    cout << latitude << endl;
    cout << longitude << endl;

}

void buildTest() {
    DynamicJsonDocument doc(1024);

    doc["sensor"] = "gps";
    doc["time"] = 1351824120;
    doc["data"][0] = 48.756080;
    doc["data"][1] = 2.302038;


    serializeJson(doc, cout);
}

int main() {
    parserTest();
    buildTest();
}
