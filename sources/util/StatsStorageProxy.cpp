/*
    This file is part of RecursiveRunner.

    @author Soupe au Caillou - Jordane Pelloux-Prayer
    @author Soupe au Caillou - Gautier Pelloux-Prayer
    @author Soupe au Caillou - Pierre-Eric Pelloux-Prayer

    RecursiveRunner is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    RecursiveRunner is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with RecursiveRunner.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "StatsStorageProxy.h"

#include "base/Log.h"
#include "base/ObjectSerializer.h"

StatsStorageProxy::StatsStorageProxy() {
    _tableName = "Stats";

    _columnsNameAndType["coins"] = "int";
    _columnsNameAndType["lifetime"] = "float";
    _columnsNameAndType["points"] = "int";
}

std::string StatsStorageProxy::getValue(const std::string& columnName) {
    if (columnName == "coins") {
        return ObjectSerializer<int>::object2string(_queue.front().coinsCollected);
    } else if (columnName == "points") {
        return ObjectSerializer<int>::object2string(_queue.front().pointScored);
    } else if (columnName == "lifetime") {
        return ObjectSerializer<float>::object2string(_queue.front().lifetime);
    } else {
        LOGE("No such column name: " << columnName);
    }
    return "";
}

void StatsStorageProxy::setValue(const std::string& columnName, const std::string& value, bool pushNewElement) {
    if (pushNewElement) {
        pushAnElement();
    }

    if (columnName == "points") {
        _queue.back().pointScored =  ObjectSerializer<int>::string2object(value);
    } else if (columnName == "coins") {
        _queue.back().coinsCollected = ObjectSerializer<int>::string2object(value);
    } else if (columnName == "lifetime") {
        _queue.back().lifetime = ObjectSerializer<float>::string2object(value);
    } else {
        LOGE("No such column name: " << columnName);
    }
}

