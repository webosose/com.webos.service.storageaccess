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

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <SAFLog.h>
#include "SAFErrors.h"
#include "XmlHandler.h"

XMLHandler::XMLHandler()
    : mDoc(NULL), mRoot(NULL), mOptions(0)
{
    mFileName.clear();
    mContents.clear();
    init();
}

XMLHandler::XMLHandler(std::string xmlContents)
    : mDoc(NULL), mRoot(NULL), mContents(xmlContents), mOptions(0)
{
    init();
}

XMLHandler::XMLHandler(std::string fileName, int options)
    : mDoc(NULL), mRoot(NULL)
    , mFileName(fileName), mOptions(options)
{
    init();
}

XMLHandler::~XMLHandler()
{
    deinit();
}

void XMLHandler::deinit()
{
    if (mDoc)
    {
        xmlCleanupParser();
        xmlFreeDoc(mDoc);
        mDoc = NULL;
        mRoot = NULL;
        mRes.clear();
    }
}

void XMLHandler::init()
{
    //std::cout << __func__ << std::endl;
    if (!mFileName.empty())
    {
        mDoc = xmlReadFile(mFileName.c_str(), NULL, mOptions);
        if (mDoc != NULL)
        {
           mRoot = xmlDocGetRootElement(mDoc);
        }
        else
        {
            mRoot = NULL;
            //std::cout << "Error in parsing file " << mFileName << std::endl;
        }
    }
    else if (!mContents.empty())
    {
        //ToDo for string stream
        mDoc = xmlReadDoc((xmlChar *)(mContents.c_str()), NULL, NULL, XML_PARSE_RECOVER | XML_PARSE_NOBLANKS | XML_PARSE_OLD10);
        if (mDoc != NULL)
        {
           mRoot = xmlDocGetRootElement(mDoc);
        }
        else
        {
            mRoot = NULL;
            //std::cout << "Error in parsing contents " << mFileName << std::endl;
        }
    }
}

void XMLHandler::setXmlContent(std::string xml)
{
    deinit();
    mContents.clear();
    mContents = xml;
    mDoc = xmlReadDoc((xmlChar *)(mContents.c_str()), NULL, NULL, XML_PARSE_RECOVER | XML_PARSE_NOBLANKS | XML_PARSE_OLD10);
    if (mDoc != NULL)
    {
        mRoot = xmlDocGetRootElement(mDoc);
    }
    else
    {
        mRoot = NULL;
        //std::cout << "Error in parsing contents " << mFileName << std::endl;
    }
}


void XMLHandler::getElementDetails(xmlNode* a_node, std::string nodeName)
{
    for (xmlNode *cur_node = a_node; cur_node; cur_node = cur_node->next)
    {
      std::string nodeVal((const char*)cur_node->name);
      std::cout << __func__ << " : " << nodeVal << ", Type = " << cur_node->type << std::endl;
      if ((cur_node->type == XML_ELEMENT_NODE) && (nodeVal == nodeName))
      {
         mRes = ((const char*)xmlNodeGetContent(cur_node));
         return; 
      }
      if (nodeName == nodeVal)
      {
         mRes = ((const char*)xmlNodeListGetString(mDoc, cur_node->xmlChildrenNode, 1));
         return;
      }
      getElementDetails(cur_node->children, nodeName);
   }
}

std::string XMLHandler::getValue(std::string nodeName)
{
    mRes.clear();
    getElementDetails(mRoot, nodeName);
    std::cout << __func__ << " : " << nodeName << " val : [" << mRes << "]" << std::endl;
    return mRes;
}

void XMLHandler::getCurDirElementDetails(xmlNode* a_node, int parentID)
{
    for (xmlNode *cur_node = a_node; cur_node; cur_node = cur_node->next)
    {
      std::string nodeVal((const char*)cur_node->name);
      if (nodeVal == "container" || nodeVal == "item")
      {
         std::cout << __func__ << " : " << nodeVal << std::endl;
         std::string key, val, id, pId, rest, chCount;
             xmlAttr* attribute = cur_node->properties;
         DirDetails dirDet;
         while(attribute && attribute->name && attribute->children)
         {
                 xmlChar* value = xmlNodeListGetString(mDoc, attribute->children, 1);
             key = std::string((const char*)attribute->name);
             val = std::string((const char*)value);
             if (key == "id")
                     id = val;
                 else if (key == "parentID")
                     pId = val;
                 else if (key == "restricted")
                     rest = val;
                 else if (key == "childCount")
                     chCount = val;
             xmlFree(value);
             attribute = attribute->next;
         }
         if (pId == std::to_string(parentID))
             {
                 if (!id.empty())
                dirDet.id = std::stoi(id);
                 if (!rest.empty())
                    dirDet.restricted = std::stoi(rest);
                 if (!chCount.empty())
                    dirDet.childCount = std::stoi(chCount);
             xmlNode *tmp_node = cur_node->children;
             while(tmp_node && tmp_node->name && (tmp_node->type == XML_ELEMENT_NODE))
             {
                 if (std::string((const char*)tmp_node->name) == "title")
                     dirDet.title = ((const char*)xmlNodeGetContent(tmp_node));
             else if (std::string((const char*)tmp_node->name) == "class")
                     dirDet.className = ((const char*)xmlNodeGetContent(tmp_node));
             else if (std::string((const char*)tmp_node->name) == "res")
                     dirDet.resUrl = ((const char*)xmlNodeGetContent(tmp_node));
                tmp_node = tmp_node->next;
             }
             }
         mCurDirDetails.push_back(dirDet);
      }
      getCurDirElementDetails(cur_node->children, parentID);
   }

}

std::vector<DirDetails> XMLHandler::getCurDirDetails(int parentID)
{
    mCurDirDetails.clear();
    getCurDirElementDetails(mRoot, parentID);
    return mCurDirDetails;
}

void XMLHandler::getMatchingElementDetails(xmlNode* a_node, std::string nodeName,
    std::string matchNode, std::string matchVal)
{
    for (xmlNode *cur_node = a_node; cur_node; cur_node = cur_node->next)
    {
      std::string nodeVal((const char*)cur_node->name);
      if (nodeVal == "service")
      {
     std::string matchData, nodeData;
         xmlNode *tmp_node = cur_node->children;
     while(tmp_node && tmp_node->name && (tmp_node->type == XML_ELEMENT_NODE))
     {
         if (std::string((const char*)tmp_node->name) == matchNode)
         matchData = ((const char*)xmlNodeGetContent(tmp_node));
             else if (std::string((const char*)tmp_node->name) == nodeName)
             nodeData = ((const char*)xmlNodeGetContent(tmp_node));
         tmp_node = tmp_node->next;
     }
     if (!matchData.empty() && !nodeData.empty() && (matchData.find(matchVal) != std::string::npos))
     {
         mRes = nodeData;
         break;
     }
      }
      getMatchingElementDetails(cur_node->children, nodeName, matchNode, matchVal);
   }
}

std::string XMLHandler::getValueUnderSameParent(std::string nodeName, std::string matchNode, std::string matchVal)
{
    mRes.clear();
    getMatchingElementDetails(mRoot, nodeName, matchNode, matchVal);
    std::cout << __func__ << " : " << nodeName << " val : [" << mRes << "]" << std::endl;
    return mRes;
}
