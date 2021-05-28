//
// Created by  Easy Living on 10/24/20.
//

#ifndef P2_SELECTQUERY_H
#define P2_SELECTQUERY_H

#include "../Query.h"

class SelectQuery : public ComplexQuery {
    static constexpr const char *qname = "SELECT";
    std::vector<Table::FieldIndex> fields;
public:
    using ComplexQuery::ComplexQuery;

    QueryResult::Ptr execute() override;

    std::string toString() override;

    inline const char *getQname() override {
        return qname;
    };
};

#endif //P2_SELECTQUERY_H
