// The MIT License (MIT)
//
// Copyright (c) 2014 Justin (Jianfeng) Lin
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __GDRIVE_SERVICEREQUEST_HPP__
#define __GDRIVE_SERVICEREQUEST_HPP__

#include "gdrive/config.hpp"
#include "gdrive/credential.hpp"
#include "gdrive/util.hpp"
#include "gdrive/gitem.hpp"
#include "gdrive/filecontent.hpp"
#include "gdrive/error.hpp"
#include "common/all.hpp"

#include <vector>
#include <set>

#define FILES_URL SERVICE_URI "/files"
#define ABOUT_URL SERVICE_URI "/about"
#define CHANGES_URL SERVICE_URI "/changes"
#define APPS_URL SERVICE_URI "/apps"

#define STRING_SET_ATTR(name) void set_##name(std::string name) { \
    _query[#name] = name;\
}

#define LONG_SET_ATTR(name) void set_##name(long name) { \
    _query[#name] = COMMON::VarString::itos(name);\
}

#define BOOL_SET_ATTR(name) void set_##name(bool flag) { \
    if (flag) _query[#name] = "true";\
    else _query[#name] = "false"; \
}

#define ADD_REMOVE_PARENT \
    public: \
        void add_parent(std::string parent) { \
            _parents.insert(parent); \
            _query["addParents"] = VarString::join(_parents,",");\
        } \
        void remove_parent(std::string parent) { \
            _parents.erase(parent); \
            _query["addParents"] = VarString::join(_parents,","); \
        } \
    private:\
        std::set<std::string> _parents; \
    public:

namespace GDRIVE {

GoogleJsonResponseException make_json_exception(std::string content);

template<class ResType, RequestMethod method>
class ResourceRequest : public CredentialHttpRequest {
    CLASS_MAKE_LOGGER
    public:
        ResourceRequest(Credential* cred, std::string uri)
            :CredentialHttpRequest(cred, uri, method) {}

        ResType execute() {
            ResType _1;
            CredentialHttpRequest::request();
            get_resource(_1);
            return _1;

        }

        inline void clear_fields() {
            if (_query.find("fields") == _query.end()) return;
            _query.erase("fields");
        }

        inline void add_field(std::string field) {
            if (_query.find("fields") == _query.end()) {
                _query["fields"] = field;
            } else {
                _query["fields"] += "," + field;
            }
        };

    protected:
        void get_resource(ResType& res) {

            if (_resp.status() != 200) {
                GoogleJsonResponseException exc = make_json_exception(_resp.content());
                throw exc;
            } else {
                PError error;
                JObject* obj = (JObject*)loads(_resp.content(), error);
                if (obj != NULL) {
                    res.from_json(obj);
                    delete obj;
                }
            }
        }
};

class DeleteRequest : public CredentialHttpRequest {
    CLASS_MAKE_LOGGER
    public:
        DeleteRequest(Credential* cred, std::string uri)
            :CredentialHttpRequest(cred, uri, RM_DELETE) {}
        void execute();
};

template<class ResType, RequestMethod method>
class ResourceAttachedRequest : public ResourceRequest<ResType, method> {
    CLASS_MAKE_LOGGER
    public:
        ResourceAttachedRequest(ResType* resource, Credential* cred, std::string uri)
            :ResourceRequest<ResType, method>(cred, uri), _resource(resource) {}

        ResType execute() {
            _json_encode_body();
            ResType _1 = *_resource;
            CredentialHttpRequest::request();
            this->get_resource(_1);
            return _1;
        }

    protected:
        void _json_encode_body() {
            std::set<std::string> fields = _resource->get_modified_fields();
            _resource->clear();

            JObject* tmp = _resource->to_json();
            JObject* rst_obj = new JObject();
            for(std::set<std::string>::iterator iter = fields.begin();
                    iter != fields.end(); iter ++) {
                std::string field = *iter;
                if (tmp->contain(field)) {
                    JValue* v = tmp->pop(field);
                    rst_obj->put(field, v);
                }
            }
            char* buf;
            dumps(rst_obj, &buf);
            delete tmp;
            delete rst_obj;
            this->_body = std::string(buf);
            free(buf);

            this->_header["Content-Type"] = "application/json";
            this->_header["Content-Length"] = VarString::itos(this->_body.size());
        }

        ResType* _resource;
};

class FileListRequest: public ResourceRequest<GFileList, RM_GET> {
    CLASS_MAKE_LOGGER
    public:
        FileListRequest(Credential* cred, std::string uri)
            :ResourceRequest<GFileList, RM_GET>(cred, uri) {}
        STRING_SET_ATTR(pageToken)
        STRING_SET_ATTR(q)
        void set_corpus(std::string corpus);
        void set_maxResults(int max_results);
};

class FileGetRequest: public ResourceRequest<GFile, RM_GET> {
    CLASS_MAKE_LOGGER
    public:
        FileGetRequest(Credential* cred, std::string uri)
            :ResourceRequest<GFile, RM_GET>(cred, uri) {}
        BOOL_SET_ATTR(updateViewedDate)
};

typedef ResourceRequest<GFile, RM_POST> FileTrashRequest;
typedef FileTrashRequest FileUntrashRequest;
typedef DeleteRequest FileDeleteRequest;
typedef FileDeleteRequest FileEmptyTrashRequest;
typedef ResourceRequest<GFile, RM_POST> FileTouchRequest;

class FilePatchRequest : public ResourceAttachedRequest<GFile, RM_PATCH> {
    CLASS_MAKE_LOGGER
    public:
        FilePatchRequest(GFile* file, Credential* cred, std::string uri)
            :ResourceAttachedRequest<GFile, RM_PATCH>(file, cred, uri) {}


        BOOL_SET_ATTR(convert)
        BOOL_SET_ATTR(newRevision)
        BOOL_SET_ATTR(ocr)
        STRING_SET_ATTR(orcLanguag)
        BOOL_SET_ATTR(pinned) 
        BOOL_SET_ATTR(setModifiedDate)
        STRING_SET_ATTR(timedTextLanguge)
        STRING_SET_ATTR(timedTextTrackName)
        BOOL_SET_ATTR(updateViewedDate)
        BOOL_SET_ATTR(useContentAsIndexableText)

        ADD_REMOVE_PARENT
};

class FileCopyRequest : public ResourceAttachedRequest<GFile, RM_POST> {
    CLASS_MAKE_LOGGER
    public:
        FileCopyRequest(GFile* file, Credential* cred, std::string uri)
            :ResourceAttachedRequest<GFile, RM_POST>(file, cred, uri) {}

        BOOL_SET_ATTR(convert)
        BOOL_SET_ATTR(ocr)
        STRING_SET_ATTR(ocrLanguage)
        BOOL_SET_ATTR(pinned)
        STRING_SET_ATTR(timedTextLanguage)
        STRING_SET_ATTR(timedTextTrackName)
        void set_visibilty(std::string v) {
            if (v == "DEFAULT" || v == "PRIVATE")
                _query["visibility"] = v;
        }
};


enum UploadType {
    UT_CREATE,
    UT_UPDATE
};

class FileUploadRequest: public ResourceAttachedRequest<GFile, RM_POST> {
    CLASS_MAKE_LOGGER
    public:
        FileUploadRequest(FileContent* content, GFile* file, Credential* cred, std::string uri, bool resumable = false)
            :ResourceAttachedRequest<GFile, RM_POST>(file, cred, uri), _content(content), _resumable(resumable), _type(UT_CREATE) {}

        GFile execute();
        BOOL_SET_ATTR(convert)
        BOOL_SET_ATTR(ocr)
        STRING_SET_ATTR(orcLanguag)
        BOOL_SET_ATTR(pinned) 
        STRING_SET_ATTR(timedTextLanguge)
        STRING_SET_ATTR(timedTextTrackName)
        BOOL_SET_ATTR(useContentAsIndexableText)
        void set_visibilty(std::string v) {
            if (v == "DEFAULT" || v == "PRIVATE")
                _query["visibility"] = v;
        }

    protected:
        std::string _generate_boundary() { return "======xxxxx=="; }
        int _resume();
        FileContent* _content;
        bool _resumable;
        UploadType _type;
};

typedef FileUploadRequest FileInsertRequest;

class FileUpdateRequest: public FileUploadRequest {
    CLASS_MAKE_LOGGER
    public:
        FileUpdateRequest(FileContent* content, GFile* file, Credential* cred, std::string uri, bool resumable = false)
            :FileUploadRequest(content, file, cred, uri, resumable)
        { 
            _type = UT_UPDATE;
            _method = RM_PUT;
        }

        ADD_REMOVE_PARENT
};

class AboutGetRequest: public ResourceRequest<GAbout, RM_GET> {
    CLASS_MAKE_LOGGER
    public:
        AboutGetRequest(Credential* cred, std::string uri)
            :ResourceRequest<GAbout, RM_GET>(cred, uri) {}
        
        BOOL_SET_ATTR(includeSubscribed)
        LONG_SET_ATTR(maxChangeIdCount)
        LONG_SET_ATTR(startChangeId)
};

typedef ResourceRequest<GChange, RM_GET> ChangeGetRequest;

class ChangeListRequest: public ResourceRequest<GChangeList, RM_GET> {
    CLASS_MAKE_LOGGER
    public:
        ChangeListRequest(Credential* cred, std::string uri)
            :ResourceRequest<GChangeList, RM_GET>(cred, uri) {}

        BOOL_SET_ATTR(includeDeleted)
        BOOL_SET_ATTR(includeSubscribed)
        STRING_SET_ATTR(pageToken)
        LONG_SET_ATTR(maxResults)
        LONG_SET_ATTR(startChangeId)
};


class ChildrenListRequest: public ResourceRequest<GChildrenList, RM_GET> {
    CLASS_MAKE_LOGGER
    public:
        ChildrenListRequest(Credential* cred, std::string uri)
            :ResourceRequest<GChildrenList, RM_GET>(cred, uri) {}

        LONG_SET_ATTR(maxResults)
        STRING_SET_ATTR(pageToken)
        STRING_SET_ATTR(q)
};

typedef ResourceRequest<GChildren, RM_GET> ChildrenGetRequest;
typedef ResourceAttachedRequest<GChildren, RM_POST> ChildrenInsertRequest;
typedef DeleteRequest ChildrenDeleteRequest;

typedef ResourceRequest<GParentList, RM_GET> ParentListRequest;
typedef ResourceRequest<GParent, RM_GET> ParentGetRequest;
typedef ResourceAttachedRequest<GParent, RM_POST> ParentInsertRequest;

typedef DeleteRequest ParentDeleteRequest;
typedef ResourceRequest<GPermissionList, RM_GET> PermissionListRequest;
typedef ResourceRequest<GPermission, RM_GET> PermissionGetRequest;

class PermissionInsertRequest: public ResourceAttachedRequest<GPermission, RM_POST> {
    CLASS_MAKE_LOGGER
    public:
        PermissionInsertRequest(GPermission* permission, Credential* cred, std::string uri)
            :ResourceAttachedRequest<GPermission, RM_POST>(permission, cred, uri) {}
        STRING_SET_ATTR(emailMessage)
        BOOL_SET_ATTR(sendNotificationEmails)
};

typedef DeleteRequest PermissionDeleteRequest;

class PermissionPatchRequest : public ResourceAttachedRequest<GPermission, RM_PATCH> {
    CLASS_MAKE_LOGGER
    public:
        PermissionPatchRequest(GPermission* permission, Credential* cred, std::string uri)
            :ResourceAttachedRequest<GPermission, RM_PATCH>(permission, cred, uri) {}

        BOOL_SET_ATTR(transferOwnership)
};

class PermissionUpdateRequest : public ResourceAttachedRequest<GPermission, RM_PUT> {
    CLASS_MAKE_LOGGER
    public:
        PermissionUpdateRequest(GPermission* permission, Credential* cred, std::string uri)
            :ResourceAttachedRequest<GPermission, RM_PUT>(permission, cred, uri) {}

        BOOL_SET_ATTR(transferOwnership)
};

typedef ResourceRequest<GPermissionId, RM_GET> PermissionGetIdForEmailRequest;

typedef ResourceRequest<GRevisionList, RM_GET> RevisionListRequest;
typedef ResourceRequest<GRevision, RM_GET> RevisionGetRequest;
typedef DeleteRequest RevisionDeleteRequest;
typedef ResourceAttachedRequest<GRevision, RM_PATCH> RevisionPatchRequest;

class RevisionUpdateRequest : public ResourceAttachedRequest<GRevision, RM_PUT> {
    CLASS_MAKE_LOGGER
    public:
        RevisionUpdateRequest(GRevision* revision, Credential* cred, std::string uri)
            :ResourceAttachedRequest<GRevision, RM_PUT>(revision, cred, uri) {}

        BOOL_SET_ATTR(pinned)
        BOOL_SET_ATTR(publishedAuto)
        BOOL_SET_ATTR(published)
        BOOL_SET_ATTR(publishedOutsideDomain)
};

class AppListRequest: public ResourceRequest<GAppList, RM_GET> {
    CLASS_MAKE_LOGGER
    public:
        AppListRequest(Credential* cred, std::string uri)
            :ResourceRequest<GAppList, RM_GET>(cred, uri) {}
        STRING_SET_ATTR(appFilterExtensions)
        STRING_SET_ATTR(appFilterMimeTypes)
        STRING_SET_ATTR(languageCode)
};

typedef ResourceRequest<GApp, RM_GET> AppGetRequest;

class ReplyListRequest: public ResourceRequest<GReplyList, RM_GET> {
    CLASS_MAKE_LOGGER
    public:
        ReplyListRequest(Credential* cred, std::string uri)
            :ResourceRequest<GReplyList, RM_GET>(cred, uri) {}
        BOOL_SET_ATTR(includedDeleted)
        LONG_SET_ATTR(maxResults)
        STRING_SET_ATTR(pageToken)
};

class ReplyGetRequest: public ResourceRequest<GReply, RM_GET> {
    CLASS_MAKE_LOGGER
    public:
        ReplyGetRequest(Credential* cred, std::string uri)
            :ResourceRequest<GReply, RM_GET>(cred, uri) {}
        BOOL_SET_ATTR(includedDeleted)
};

typedef DeleteRequest ReplyDeleteRequest;
typedef ResourceAttachedRequest<GReply, RM_POST> ReplyInsertRequest;
typedef ResourceAttachedRequest<GReply, RM_PATCH> ReplyPatchRequest;
typedef ResourceAttachedRequest<GReply, RM_PUT> ReplyUpdateRequest;

class CommentListRequest: public ResourceRequest<GCommentList, RM_GET> {
    CLASS_MAKE_LOGGER
    public:
        CommentListRequest(Credential* cred, std::string uri)
            :ResourceRequest<GCommentList, RM_GET>(cred, uri) {}
        BOOL_SET_ATTR(includedDeleted)
        LONG_SET_ATTR(maxResults)
        STRING_SET_ATTR(pageToken)
        STRING_SET_ATTR(updateMin)
};

class CommentGetRequest: public ResourceRequest<GComment, RM_GET> {
    CLASS_MAKE_LOGGER
    public:
        CommentGetRequest(Credential* cred, std::string uri)
            :ResourceRequest<GComment, RM_GET>(cred, uri) {}
        BOOL_SET_ATTR(includedDeleted)
};

typedef DeleteRequest CommentDeleteRequest;
typedef ResourceAttachedRequest<GComment, RM_POST> CommentInsertRequest;
typedef ResourceAttachedRequest<GComment, RM_PATCH> CommentPatchRequest;
typedef ResourceAttachedRequest<GComment, RM_PUT> CommentUpdateRequest;

}

#endif
