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

#ifndef _INTERNAL_STORAGE_PROVIDER_H_
#define _INTERNAL_STORAGE_PROVIDER_H_

#include "DocumentProvider.h"
#include "DocumentProviderFactory.h"
#include <iostream>

class InternalStorageProvider: public DocumentProvider
{
public:
    InternalStorageProvider();
    virtual ~InternalStorageProvider();
    ReturnValue listFolderContents(string storageId, string path, int offset, int limit);
    ReturnValue getProperties();
    ReturnValue copy(string srcStorageId, string srcPath, string destStorageId, string destPath, bool overwrite);
    ReturnValue move(string srcStorageId, string srcPath, string destStorageId, string destPath, bool overwrite);
    ReturnValue remove(string storageId, string path);
    ReturnValue eject(string storageId);
    ReturnValue format(string storageId, string fileSystem, string volumeLabel);
    fstream getFileStream(string path, ios_base::openmode mode);

private:
};

#endif /* _INTERNAL_STORAGE_PROVIDER_H_ */
