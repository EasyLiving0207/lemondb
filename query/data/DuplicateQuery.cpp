//
// Created by  Easy Living on 10/24/20.
//

#include "DuplicateQuery.h"
#include "../../db/Database.h"

#include <algorithm>

constexpr const char *DuplicateQuery::qname;

QueryResult::Ptr DuplicateQuery::execute() {
    using namespace std;
    Database &db = Database::getInstance();
    try {
        auto &table = db[this->targetTable];
        auto result = initCondition(table);
        vector<pair<std::string, vector<Table::ValueType>>> datum;
        if (result.second)
            for (auto it = table.begin(); it != table.end(); ++it)
                if (this->evalCondition(*it)) {
                    auto key = it->key() + "_copy";
                    vector<Table::ValueType> data;
                    for (size_t i = 0; i < table.field().size(); ++i)
                        data.push_back((*it)[i]);
                    datum.push_back(pair<std::string, vector<Table::ValueType>>(move(key), move(data)));
                }
        Table::SizeType count = 0;
        for (auto &p: datum) {
            if (table.insertByDuplicate(p.first, move(p.second)))
                ++count;
        }
        return std::make_unique<RecordCountResult>(count);
    }
    catch (const TableNameNotFound &e) {
        return make_unique<ErrorMsgResult>(qname, this->targetTable, "No such table."s);
    } catch (const IllFormedQueryCondition &e) {
        return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
    } catch (const invalid_argument &e) {
        // Cannot convert operand to string
        return make_unique<ErrorMsgResult>(qname, this->targetTable, "Unknown error '?'"_f % e.what());
    } catch (const exception &e) {
        return make_unique<ErrorMsgResult>(qname, this->targetTable, "Unknown error '?'."_f % e.what());
    }
}

std::string DuplicateQuery::toString() {
    return "QUERY = DUPLICATE " + this->targetTable + "\"";
}
