//
// Created by  Easy Living on 10/24/20.
//

#include "SwapQuery.h"
#include "../../db/Database.h"

constexpr const char *SwapQuery::qname;

QueryResult::Ptr SwapQuery::execute() {
    using namespace std;
    if (this->operands.size() != 2)
        return make_unique<ErrorMsgResult>(
                qname, this->targetTable.c_str(),
                "Invalid number of operands (? operands)."_f % operands.size()
        );
    if (this->operands[0] == "KEY" || this->operands[1] == "KEY")
        return make_unique<ErrorMsgResult>(
                qname, this->targetTable.c_str(),
                "Invalid field (KEY included)."
        );
    Database &db = Database::getInstance();
    try {
        auto &table = db[this->targetTable];
        this->field1 = table.getFieldIndex(this->operands[0]);
        this->field2 = table.getFieldIndex(this->operands[1]);
        Table::SizeType count = 0;
        if (field1 != field2) {
            auto result = initCondition(table);
            if (result.second)
                for (auto it = table.begin(); it != table.end(); ++it)
                    if (this->evalCondition(*it)) {
                        swap((*it)[this->field1], (*it)[this->field2]);
                        ++count;
                    }
        }
        return std::make_unique<RecordCountResult>(count);
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

std::string SwapQuery::toString() {
    return "QUERY = SWAP " + this->targetTable + "\"";
}