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

#ifndef __UPNP_OPERATION_H__
#define __UPNP_OPERATION_H__

#include <vector>
#include <thread>
#include "CurlClient.h"
#include "XmlHandler.h"

class UpnpOperation
{
public:
	static UpnpOperation& getInstance();
	~UpnpOperation();
	int getContainerId(std::string, std::string);
	std::vector<DirDetails> listDirContents(int);
private:
	void init();
	void deinit();
	int getNextDirId(int, std::string);
	UpnpOperation();
	UpnpOperation& operator = (const UpnpOperation&) = default;
	std::vector<std::string> mDirPathNames;
	OC::Bridging::CurlClient mCurlClient;
	XMLHandler mXmlHandObj;
    std::string mControlUrl;
};

#endif /*__UPNP_OPERATION_H__*/

