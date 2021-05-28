//
// Created by  Easy Living on 10/24/20.
//

#include "SumQuery.h"
#include "../../db/Database.h"

constexpr const char *SumQuery::qname;

QueryResult::Ptr SumQuery::execute() {
    using namespace std;
    Database &db = Database::getInstance();
    try {
        auto &table = db[this->targetTable];
        for (auto &op: this->operands) {
            this->sum.push_back(pair<Table::FieldIndex, Table::ValueType>(table.getFieldIndex(op), 0));
        }
        auto result = initCondition(table);
        if (result.second) {
            for (auto it = table.begin(); it != table.end(); ++it)
                if (this->evalCondition(*it))
                    for (auto &p: sum)
                        p.second += (*it)[p.first];
        }
        return make_unique<AnswerResult>(sum);
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

std::string SumQuery::toString() {
    return "QUERY = SUM " + this->targetTable + "\"";
}