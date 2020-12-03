/* @@@LICENSE
 *
 * Copyright (c) 2020 LG Electronics, Inc.
 *
 * Confidential computer software. Valid license from LG required for
 * possession, use or copying. Consistent with FAR 12.211 and 12.212,
 * Commercial Computer Software, Computer Software Documentation, and
 * Technical Data for Commercial Items are licensed to the U.S. Government
 * under vendor's standard commercial license.
 *
 * LICENSE@@@ */

#ifndef SA_COMMON_H
#define SA_COMMON_H

#include <string>
#include <map>
#include <memory>

using namespace std;

class Content {
public:
    Content() {};
    virtual ~Content() {}
};

class Storage : public Content  {
public:
    Storage(string _type, string _id, string _name) : type(_type), id(_id), name(_name) {}
    string type;
    string id;
    string name;
};

enum class DataType {
    STRING, NUMBER, BOOLEAN
};

typedef pair<string,DataType> ValuePair;
typedef map<string,ValuePair> ValuePairMap;
typedef vector<shared_ptr<Content>> ContentList;
typedef pair<shared_ptr<ValuePairMap>, shared_ptr<ContentList>> ResultPair;
typedef shared_ptr<ResultPair> ReturnValue;

#endif /* SA_COMMON_H */

