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
#include <functional>
#include "ClientWatch.h"

using namespace std;

enum class DataType {
    STRING, NUMBER, BOOLEAN
};

enum class StorageType
{
    INVALID = -1, INTERNAL, USB, GDRIVE
};

class Content {
public:
    Content() {};
    virtual ~Content() {}
};

class Storage : public Content  {
public:
    Storage(StorageType _type, string _id, string _name) : type(_type), id(_id), name(_name) {}
    StorageType type;
    string id;
    string name;
};

typedef pair<string,DataType> ValuePair;
typedef map<string,ValuePair> ValuePairMap;
typedef vector<shared_ptr<Content>> ContentList;
typedef pair<shared_ptr<ValuePairMap>, shared_ptr<ContentList>> ResultPair;
typedef shared_ptr<ResultPair> ReturnValue;

typedef map<string, string> AuthParam;

enum class MethodType {
    TEST_METHOD,
    LIST_STORAGES_METHOD
};

class RequestData {
public:
	std::function<void(pbnjson::JValue, std::shared_ptr<LSUtils::ClientWatch>)> cb;
	StorageType storageType;
	MethodType	methodType;
	pbnjson::JValue params;
	std::shared_ptr<LSUtils::ClientWatch> subs;
};

class ReqContext
{
public:
	void *ctx;
	std::shared_ptr<RequestData> reqData;
};


#endif /* SA_COMMON_H */

