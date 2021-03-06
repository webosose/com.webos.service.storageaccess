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


#ifndef _DOCUMENT_PROVIDER_MANAGER_H_
#define _DOCUMENT_PROVIDER_MANAGER_H_

#include <string>
#include <vector>
#include <memory>
#include "SA_Common.h"
#include "DocumentProviderFactory.h"

class DocumentProviderManager {
public:
    DocumentProviderManager();
    ~DocumentProviderManager();
	void addRequest(std::shared_ptr<RequestData>&);
};

#endif /* _DOCUMENT_PROVIDER_MANAGER_H_ */
