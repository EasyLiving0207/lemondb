//
// Created by  Easy Living on 10/24/20.
//

#include "MinQuery.h"
#include "../../db/Database.h"

constexpr const char *MinQuery::qname;

QueryResult::Ptr MinQuery::execute() {
    using namespace std;
    Database &db = Database::getInstance();
    try {
        auto &table = db[this->targetTable];
        for (auto &op: this->operands) {
            this->min.push_back(
                    pair<Table::FieldIndex, Table::ValueType>(table.getFieldIndex(op), Table::ValueTypeMax));
        }
        auto result = initCondition(table);
        Table::SizeType count = 0;
        if (result.second) {
            for (auto it = table.begin(); it != table.end(); ++it)
                if (this->evalCondition(*it)) {
                    ++count;
                    for (auto &p: min)
                        if ((*it)[p.first] < p.second)
                            p.second = (*it)[p.first];
                }
        }
        if (count > 0)
            return make_unique<AnswerResult>(min);
        return make_unique<SuccessMsgResult>(qname, this->targetTable);
    } catch (const TableNameNotFound &e) {
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

std::string MinQuery::toString() {
    return "QUERY = MIN " + this->targetTable + "\"";
}