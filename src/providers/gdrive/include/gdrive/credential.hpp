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

#ifndef __GDRIVE_CREDENTIAL_HPP__
#define __GDRIVE_CREDENTIAL_HPP__

#include "gdrive/config.hpp"
#include "gdrive/util.hpp"
#include "gdrive/request.hpp"
#include "gdrive/store.hpp"
#include "common/all.hpp"

#include <map>
#include <string>

namespace GDRIVE {

class CredentialHttpRequest;

class Credential {
    CLASS_MAKE_LOGGER
    public:
        Credential(Store* store);
        Credential(std::map<std::string, std::string> *auth);

        inline bool invalid() const { return _invalid; }
        void refresh(std::string at, std::string rt, long te, std::string it = "");
        void dump();
    private:
        std::string _access_token;
        std::string _client_id;
        std::string _client_secret;
        std::string _refresh_token;
        long _token_expiry;
        std::string _id_token;
        bool _invalid;

        Store *_store;
        std::map<std::string, std::string>* authMap;

        Credential(const Credential& other);
        Credential& operator=(const Credential& other);

    friend class CredentialHttpRequest;
};


class CredentialHttpRequest : public HttpRequest {
    CLASS_MAKE_LOGGER
    public:
        CredentialHttpRequest(Credential *cred, std::string uri, RequestMethod method);
        HttpResponse request();
    protected:
        Credential *_cred;

        void _apply_header();
        void _refresh();

        std::string _generate_request_body();
        RequestHeader _generate_request_header();

        void _parse_response(std::string content);
};

}

#endif
