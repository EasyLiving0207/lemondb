//
// Created by  Easy Living on 10/24/20.
//

#ifndef P2_DUPLICATEQUERY_H
#define P2_DUPLICATEQUERY_H

#include "../Query.h"

class DuplicateQuery : public ComplexQuery {
    static constexpr const char *qname = "DUPLICATE";
public:
    using ComplexQuery::ComplexQuery;

    QueryResult::Ptr execute() override;

    std::string toString() override;

    inline const char *getQname() override {
        return qname;
    };
};

#endif //P2_DUPLICATEQUERY_H
