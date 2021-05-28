//
// Created by  Easy Living on 10/24/20.
//

#ifndef P2_SWAPQUERY_H
#define P2_SWAPQUERY_H

#include "../Query.h"

class SwapQuery : public ComplexQuery {
    static constexpr const char *qname = "SWAP";
    Table::FieldIndex field1;
    Table::FieldIndex field2;
public:
    using ComplexQuery::ComplexQuery;

    QueryResult::Ptr execute() override;

    std::string toString() override;

    inline const char *getQname() override {
        return qname;
    };
};

#endif //P2_SWAPQUERY_H
