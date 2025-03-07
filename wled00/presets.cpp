#include "wled.h"

/*
 * Methods to handle saving and loading presets to/from the filesystem
 */

bool applyPreset(byte index, byte callMode)
{
  if (index == 0) return false;

  const char *filename = index < 255 ? "/presets.json" : "/tmp.json";

  if (fileDoc) {
    errorFlag = readObjectFromFileUsingId(filename, index, fileDoc) ? ERR_NONE : ERR_FS_PLOAD;
    JsonObject fdo = fileDoc->as<JsonObject>();
    if (fdo["ps"] == index) fdo.remove("ps"); //remove load request for same presets to prevent recursive crash
    #ifdef WLED_DEBUG_FS
      serializeJson(*fileDoc, Serial);
    #endif
    deserializeState(fdo, callMode, index);
  } else {
    DEBUGFS_PRINTLN(F("Make read buf"));
    #ifdef WLED_USE_DYNAMIC_JSON
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    #else
    if (!requestJSONBufferLock(9)) return false;
    #endif
    errorFlag = readObjectFromFileUsingId(filename, index, &doc) ? ERR_NONE : ERR_FS_PLOAD;
    JsonObject fdo = doc.as<JsonObject>();
    if (fdo["ps"] == index) fdo.remove("ps");
    #ifdef WLED_DEBUG_FS
      serializeJson(doc, Serial);
    #endif
    deserializeState(fdo, callMode, index);
    releaseJSONBufferLock();
  }

  if (!errorFlag) {
    if (index < 255) currentPreset = index;
    return true;
  }
  return false;
}

void savePreset(byte index, bool persist, const char* pname, JsonObject saveobj)
{
  if (index == 0 || (index > 250 && persist) || (index<255 && !persist)) return;
  JsonObject sObj = saveobj;

  const char *filename = persist ? "/presets.json" : "/tmp.json";

  if (!fileDoc) {
    DEBUGFS_PRINTLN(F("Allocating saving buffer"));
    #ifdef WLED_USE_DYNAMIC_JSON
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    #else
    if (!requestJSONBufferLock(10)) return;
    #endif
    sObj = doc.to<JsonObject>();
    if (pname) sObj["n"] = pname;

    DEBUGFS_PRINTLN(F("Save current state"));
    serializeState(sObj, true);
    if (persist) currentPreset = index;

    writeObjectToFileUsingId(filename, index, &doc);

    releaseJSONBufferLock();
  } else { //from JSON API (fileDoc != nullptr)
    DEBUGFS_PRINTLN(F("Reuse recv buffer"));
    sObj.remove(F("psave"));
    sObj.remove(F("v"));

    if (!sObj["o"]) {
      DEBUGFS_PRINTLN(F("Save current state"));
      serializeState(sObj, true, sObj["ib"], sObj["sb"]);
      if (persist) currentPreset = index;
    }
    sObj.remove("o");
    sObj.remove("ib");
    sObj.remove("sb");
    sObj.remove(F("error"));
    sObj.remove(F("time"));

    writeObjectToFileUsingId(filename, index, fileDoc);
  }
  if (persist) presetsModifiedTime = toki.second(); //unix time
  updateFSInfo();
}

void deletePreset(byte index) {
  StaticJsonDocument<24> empty;
  writeObjectToFileUsingId("/presets.json", index, &empty);
  presetsModifiedTime = toki.second(); //unix time
  updateFSInfo();
}