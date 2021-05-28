//
// Created by ASUS on 2020/11/16.
//

#ifndef PGROUP_02_LISTENQUERY_H
#define PGROUP_02_LISTENQUERY_H

#include "../Query.h"

class ListenQuery : public Query {
    static constexpr const char *qname = "LISTEN";
public:
    using Query::Query;

    QueryResult::Ptr execute() override;

    std::string toString() override;

    inline const char *getQname() override {
        return qname;
    };
};

#endif //PGROUP_02_LISTENQUERY_H
