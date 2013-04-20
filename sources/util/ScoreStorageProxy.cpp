#include "ScoreStorageProxy.h"

#include "base/Log.h"
#include "base/ObjectSerializer.h"

ScoreStorageProxy::ScoreStorageProxy() {
    _tableName = "Score";

    _columnsNameAndType["points"] = "int";
    _columnsNameAndType["coins"] = "int";
    _columnsNameAndType["name"] = "string";
}

std::string ScoreStorageProxy::getValue(const std::string& columnName) {
    if (columnName == "points") {
        return ObjectSerializer<int>::object2string(_queue.back().points);
    } else if (columnName == "coins") {
        return ObjectSerializer<int>::object2string(_queue.back().coins);
    } else if (columnName == "name") {
        return _queue.back().name;
    } else {
        LOGE("No such column name: " << columnName);
    }
    return "";
}

void ScoreStorageProxy::setValue(const std::string& columnName, const std::string& value, bool pushNewElement) {
    if (pushNewElement) {
        pushAnElement();
    }

    if (columnName == "points") {
        _queue.back().points =  ObjectSerializer<int>::string2object(value);
    } else if (columnName == "coins") {
        _queue.back().coins = ObjectSerializer<int>::string2object(value);
    } else if (columnName == "name") {
        _queue.back().name = value;
    } else {
        LOGE("No such column name: " << columnName);
    }
}

