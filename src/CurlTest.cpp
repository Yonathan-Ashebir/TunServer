//
// Created by yoni_ash on 5/29/23.
//

#include "curl/curl.h"
#include <iostream>

using namespace std;

/*Lambdas do not work*/
size_t reader(char *buf, size_t _, size_t len, void *customData) {
    if (*(int *) customData != 3)throw invalid_argument("Custom data should be null");
    cout << buf;
    return len;
};

/*Functors failed too*/
class ReaderFunctor {
private:
    void *arg;

public:
    ReaderFunctor() = delete;

    explicit ReaderFunctor(void *arg) : arg(arg) {
    }

    size_t operator()(char *buf, size_t _, size_t len, void *customData) {
        return reader(buf, _, len, customData);
    }

    size_t reader(char *buf, size_t _, size_t len, void *customData) {
        if (*(int *) customData != 3 && *(int *) (this->arg) != 3)throw invalid_argument("Custom data should be null");
        cout << buf;
        return len;
    };
};

void httpRequest() {
    CURL *curl;
    CURLcode res;

    /* In windows, this will init the winsock stuff */
    curl_global_init(CURL_GLOBAL_ALL);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://example.com");

        /* Use HTTP/3 but fallback to earlier HTTP if necessary */
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION,
                         (long) CURL_HTTP_VERSION_3);

//         curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "name=daniel&project=curl");
        /* Perform the request, res will get the return code */

        int a = 3;
        ReaderFunctor functor(&a);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, reader);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &a);
        res = curl_easy_perform(curl);

        /* Check for errors */
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        /* always cleanup */
        curl_easy_cleanup(curl);
    }

    /*When no more networking is needed - yoni_ash*/
    curl_global_cleanup();
}

void preprocessorCheck() {
#ifdef BUILD_SHARED_LIBS
    printf("BUILD_SHARED_LIBS is set\n");
#endif
#ifdef HB_STATIC_CURL
    printf("HB_STATIC_CURL is set\n");
#endif
#ifdef CURL_STATICLIB
    printf("CURL_STATICLIB is set\n");
#endif
}

int main() {
    preprocessorCheck();
    httpRequest();
}