//
// Created by  Easy Living on 10/24/20.
//

#include "CountQuery.h"
#include "../../db/Database.h"

constexpr const char *CountQuery::qname;

QueryResult::Ptr CountQuery::execute() {
    using namespace std;
    Database &db = Database::getInstance();
    try {
        auto &table = db[this->targetTable];
        auto result = initCondition(table);
        Table::SizeType count = 0;
        if (result.second)
            for (auto it = table.begin(); it != table.end(); ++it)
                if (this->evalCondition(*it))
                    ++count;
        return make_unique<AnswerResult>(count);
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

std::string CountQuery::toString() {
    return "QUERY = COUNT " + this->targetTable + "\"";
}