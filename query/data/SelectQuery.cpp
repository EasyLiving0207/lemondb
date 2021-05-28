//
// Created by  Easy Living on 10/24/20.
//

#include "SelectQuery.h"
#include "../../db/Database.h"

#include <algorithm>

constexpr const char *SelectQuery::qname;

QueryResult::Ptr SelectQuery::execute() {
    using namespace std;
    if (this->operands.size() < 1) // No field selected
        return make_unique<ErrorMsgResult>(
                qname, this->targetTable.c_str(),
                "Invalid number of operands (? operands)."_f % operands.size()
        );
    if (this->operands[0] != "KEY")
        return make_unique<ErrorMsgResult>(
                qname, this->targetTable.c_str(),
                "Invalid field (KEY not selected first)."
        );
    Database &db = Database::getInstance();
    string msg;
    try {
        auto &table = db[this->targetTable];
        for (size_t i = 1; i < this->operands.size(); ++i) {
            this->fields.push_back(table.getFieldIndex(this->operands[i]));
        }
        auto result = initCondition(table);
        if (result.second) {
            vector<Table::Iterator> rows;
            for (auto it = table.begin(); it != table.end(); ++it) {
                if (this->evalCondition(*it)) {
                    rows.push_back(it);
                }
            }
            std::sort(rows.begin(), rows.end(),
                      [](Table::Iterator a, Table::Iterator b) { return a->key() < b->key(); });
            for (auto &it : rows) {
                msg += "( " + it->key() + " ";
                for (auto &field : fields) {
                    msg += to_string(it->get(field)) + " ";
                }
                msg += ")\n";
            }
        }
        return make_unique<SelectResult>(msg);
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

std::string SelectQuery::toString() {
    return "QUERY = SELECT " + this->targetTable + "\"";
}
