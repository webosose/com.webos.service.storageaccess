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

#ifndef _DOCUMENT_PROVIDER_H_
#define _DOCUMENT_PROVIDER_H_


#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include "SA_Common.h"

using namespace std;

class DocumentProvider
{
public:
    DocumentProvider() = default;
    virtual ~DocumentProvider() = default;
	virtual void addRequest(std::shared_ptr<RequestData>&) = 0;
};

#endif /* _DOCUMENT_PROVIDER_H_ */
