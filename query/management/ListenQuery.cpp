//
// Created by ASUS on 2020/11/16.
//

#include "ListenQuery.h"
#include "../../db/Database.h"
#include <fstream>
#include <string>

constexpr const char *ListenQuery::qname;

QueryResult::Ptr ListenQuery::execute() {
        std::fstream fnew;
        std::string path = this->targetTable;
        fnew.open(path);
        if (fnew.is_open()) {
            auto pos = path.rfind("/");
            if (pos == std::string::npos)
                return std::make_unique<AnswerResult>("Answer = ( listening from "+path+" )");
            std::string filename = path.substr(pos+1);
            return std::make_unique<AnswerResult>("Answer = ( listening from "+filename+" )");
        }
        else
            return std::make_unique<AnswerResult>("Error: could not open "+path);
}

std::string ListenQuery::toString() {
    return "QUERY = LISTEN, FILE = \"" + targetTable + "\"";
}