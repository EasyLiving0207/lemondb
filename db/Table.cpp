//
// Created by liu on 18-10-23.
//

#include "Database.h"
#include "Table.h"

#include <iomanip>
#include <iostream>
#include <sstream>

constexpr const Table::ValueType Table::ValueTypeMax;
constexpr const Table::ValueType Table::ValueTypeMin;

Table::FieldIndex Table::getFieldIndex(const Table::FieldNameType &field) const {
    try {
        return this->fieldMap.at(field);
    } catch (const std::out_of_range &e) {
        throw TableFieldNotFound(
                R"(Field name "?" doesn't exists.)"_f % (field)
        );
    }
}

void Table::insertByIndex(KeyType key, std::vector<ValueType> &&dat) {
    if (this->keyMap.find(key) != this->keyMap.end()) {
        std::string err = "In Table \"" + this->tableName
                          + "\" : Key \"" + key + "\" already exists!";
        throw ConflictingKey(err);
    }
    this->keyMap.emplace(key, this->data.size());
    this->data.emplace_back(std::move(key), dat);
}

bool Table::insertByDuplicate(KeyType key, std::vector<ValueType> &&dat) {
    if (this->keyMap.find(key) == this->keyMap.end()) {
        this->keyMap.emplace(key, this->data.size());
        this->data.emplace_back(std::move(key), dat);
        return true;
    }
    return false;
}

Table::Object::Ptr Table::operator[](const Table::KeyType &key) {
    auto it = keyMap.find(key);
    if (it == keyMap.end()) {
        // not found
        return nullptr;
    } else {
        return createProxy(data.begin() + it->second, this);
    }
}

std::ostream &operator<<(std::ostream &os, const Table &table) {
    const int width = 10;
    os << table.tableName << "\t" << (table.fields.size() + 1) << "\n";
    os << std::setw(width) << "KEY";
    for (const auto &field : table.fields) {
        os << std::setw(width) << field;
    }
    os << "\n";
    auto numFields = table.fields.size();
    for (const auto &datum : table.data) {
        os << std::setw(width) << datum.key;
        for (decltype(numFields) i = 0; i < numFields; ++i) {
            os << std::setw(width) << datum.datum[i];
        }
        os << "\n";
    }
    return os;
}