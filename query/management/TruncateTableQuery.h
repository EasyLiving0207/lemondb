//
// Created by ASUS on 2020/10/20.
//

#ifndef PGROUP_02_TRUNCATETABLEQUERY_H
#define PGROUP_02_TRUNCATETABLEQUERY_H

#include "../Query.h"

class TruncateTableQuery : public Query {
    static constexpr const char *qname = "TRUNCATE";
public:
    using Query::Query;

    QueryResult::Ptr execute() override;

    std::string toString() override;

    inline const char *getQname() override {
        return qname;
    };
};

#endif //PGROUP_02_TRUNCATETABLEQUERY_H
