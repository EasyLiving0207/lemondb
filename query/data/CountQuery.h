//
// Created by  Easy Living on 10/24/20.
//

#ifndef P2_COUNTQUERY_H
#define P2_COUNTQUERY_H

#include "../Query.h"

class CountQuery : public ComplexQuery {
    static constexpr const char *qname = "SELECT";
public:
    using ComplexQuery::ComplexQuery;

    QueryResult::Ptr execute() override;

    std::string toString() override;

    inline const char *getQname() override {
        return qname;
    };
};

#endif //P2_COUNTQUERY_H
