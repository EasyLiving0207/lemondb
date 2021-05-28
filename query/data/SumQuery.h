//
// Created by  Easy Living on 10/24/20.
//

#ifndef P2_SUMQUERY_H
#define P2_SUMQUERY_H

#include "../Query.h"

class SumQuery : public ComplexQuery {
    static constexpr const char *qname = "SUM";
    std::vector<std::pair<Table::FieldIndex, Table::ValueType>> sum;
public:
    using ComplexQuery::ComplexQuery;

    QueryResult::Ptr execute() override;

    std::string toString() override;

    inline const char *getQname() override {
        return qname;
    };
};

#endif //P2_SUMQUERY_H
