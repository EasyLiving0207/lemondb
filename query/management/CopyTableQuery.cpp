//
// Created by ASUS on 2020/10/24.
//


#include "CopyTableQuery.h"
#include "../../db/Database.h"
#include "../../db/Table.h"

constexpr const char *CopyTableQuery::qname;

QueryResult::Ptr CopyTableQuery::execute() {
    using namespace std;
    Database &db = Database::getInstance();
    try {
        auto oldone = db[this->targetTable];
        auto newone = std::make_unique<Table>(this->newTable, oldone);
        db.registerTable(move(newone));
        return make_unique<SuccessMsgResult>(qname);
    } catch (const TableNameNotFound &e) {
        return make_unique<ErrorMsgResult>(qname, targetTable, "No such table."s);
    } catch (const exception &e) {
        return make_unique<ErrorMsgResult>(qname, e.what());
    }
}

std::string CopyTableQuery::toString() {
    return "QUERY = COPY, Old Table = \"" + targetTable + "\"" + "New Table = \"" + newTable + "\"";
}