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

#ifndef __GDRIVE_OAUTH_HPP__
#define __GDRIVE_OAUTH_HPP__

#include <string>
#include "gdrive/config.hpp"
#include "gdrive/util.hpp"
#include "gdrive/credential.hpp"
#include "common/all.hpp"

namespace GDRIVE {

class OAuth {
    CLASS_MAKE_LOGGER
    public:
        OAuth(std::string client_id, std::string client_secret);

        std::string get_authorize_url();
        bool build_credential(std::string code, Credential& cred);
   
    private:
        std::string _client_id;
        std::string _client_secret;
        std::string _code;

        void _parse_response(std::string content);
        struct CredentialResponse {
            CredentialResponse() {
                access_token = "";
                token_type = "";
                refresh_token = "";
                id_token = "";
                expires_in = 0;
                token_expiry = 0;
            }

            std::string access_token;
            std::string token_type;
            std::string refresh_token;
            std::string id_token;
            long expires_in;
            long token_expiry;
        };

        CredentialResponse _resp;
        
};

}

#endif
