/* @@@LICENSE
 *
 * Copyright (c) 2021 LG Electronics, Inc.
 *
 * Confidential computer software. Valid license from LG required for
 * possession, use or copying. Consistent with FAR 12.211 and 12.212,
 * Commercial Computer Software, Computer Software Documentation, and
 * Technical Data for Commercial Items are licensed to the U.S. Government
 * under vendor's standard commercial license.
 *
 * LICENSE@@@ */

#ifndef __XML_HANDLER_H__
#define __XML_HANDLER_H__

#include <string>
#include <vector>
#include <libxml/parser.h>
#include <libxml/tree.h>

typedef struct sDirDetails
{
    int id;
    int restricted;
    int childCount;
    std::string title;
    std::string className;
    std::string resUrl;
}DirDetails;

class XMLHandler
{
public:
	XMLHandler();
	XMLHandler(std::string);
    XMLHandler(std::string, int);
    ~XMLHandler();
    void setXmlContent(std::string);
    std::string getValue(std::string);
    std::vector<DirDetails> getCurDirDetails(int);
    std::string getValueUnderSameParent(std::string, std::string, std::string);
private:
    void init();
    void deinit();
    void getElementDetails(xmlNode*, std::string);
    void getCurDirElementDetails(xmlNode*, int);
    void getMatchingElementDetails(xmlNode*, std::string, std::string, std::string);
    xmlDoc *mDoc;
    xmlNode *mRoot;
    std::string mFileName;
    std::string mContents;
    int mOptions;
    std::string mRes;
    std::vector<DirDetails> mCurDirDetails;
};

#endif /*__XML_HANDLER__*/
