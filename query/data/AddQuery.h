//
// Created by  Easy Living on 10/24/20.
//

#ifndef P2_ADDQUERY_H
#define P2_ADDQUERY_H

#include "../Query.h"

class AddQuery : public ComplexQuery {
    static constexpr const char *qname = "ADD";
    std::vector<Table::FieldIndex> fields;
public:
    using ComplexQuery::ComplexQuery;

    QueryResult::Ptr execute() override;

    std::string toString() override;

    inline const char *getQname() override {
        return qname;
    };
};

#endif //P2_ADDQUERY_H
