//
// Created by ASUS on 2020/10/24.
//

#ifndef PGROUP_02_COPYTABLEQUERY_H
#define PGROUP_02_COPYTABLEQUERY_H

#include "../Query.h"

class CopyTableQuery : public Query {
    static constexpr const char *qname = "COPY";
    const std::string newTable;
public:
    CopyTableQuery(std::string table, std::string newtable)
    : Query(std::move(table)), newTable(std::move(newtable)) {}

    QueryResult::Ptr execute() override;

    std::string toString() override;

    inline const char *getQname() override {
        return qname;
    };
};

#endif //PGROUP_02_COPYTABLEQUERY_H
