//
// Created by rahul on 4/8/17.
//

// #include "typtop.h"
// #include "catch.hpp"

#define DEBUG 1
#ifdef __APPLE__
#include <security/pam_appl.h>
#include <security/openpam.h>
#else
#include <security/pam_misc.h>
#include <security/pam_ext.h>
#endif

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <dirent.h>
#include <libgen.h>
#include "catch.hpp"
#include "typtopconfig.h"
#include <stdlib.h>

using namespace std;
const char* cmd_uninstall = (const char*)"sudo bash /usr/local/bin/typtop.prerm";
const char* cmd_install = (const char*)"sudo bash /usr/local/bin/typtop.postinst";

int conv_func(int num_msg, const struct pam_message **msg,
              struct pam_response **resp, void *appdata_ptr) {
    resp[0] = new struct pam_response[num_msg];
    for (int i = 0; i < num_msg; i++) {
        if (msg[i]->msg_style == PAM_PROMPT_ECHO_OFF) {
            resp[i]->resp = strdup((char *) appdata_ptr);
            resp[i]->resp_retcode = 0;
        }
    }
    return 0;
}

int auth(const char *user, const char *password) {
    pam_handle_t *pamh = NULL;
    int retval;
    // REQUIRE(freopen(PASS_FILE, "r", stdin) != NULL);
    struct pam_conv conv = {conv_func, (void *) password};
    retval = pam_start("su", user, &conv, &pamh);
    if (retval == PAM_SUCCESS)
        retval = pam_authenticate(pamh, 0);    /* is user really user? */
    string pw;

    if (retval == PAM_SUCCESS)
        retval = pam_acct_mgmt(pamh, 0);       /* permitted access? */
    /* This is where we have been authorized or not. */
    if (pam_end(pamh, retval) != PAM_SUCCESS) {     /* close Linux-PAM */
        pamh = NULL;
        fprintf(stderr, "su: failed to release authenticator\n");
        exit(1);
    }
    return (retval == PAM_SUCCESS);       /* indicate success */
}

const vector<string> pws = {
        "hello_pass", // 0, ed=0
        "Hello_pass",  // 1, ed=1
        "hello_pass1", // 2, ed=1
        "HELLO_PASS",  // 3, ed=1
        "hlelo_pass", // 4, ed=1
        "Hello_Pass",  // 5, ed=2
};
const string user = "tmptyptop";

TEST_CASE("Check Install") {
    const string prepare = "sudo useradd tmptyptop -p $(openssl passwd -crypt hello_pass)"
            " && sudo rm -rf /usr/local/etc/typtop.d/tmptyptop";
    system(cmd_install);
    SECTION("Basic") {
        int i = 0;
        for (string pw: pws) {
            if (i <= 3) {
                cerr << "Trying " << i << " " << user << " " << pw << endl;
                REQUIRE(auth(user.c_str(), pw.c_str()));
            } else {
                REQUIRE_FALSE(auth(user.c_str(), pw.c_str()));
            }
            i++;
        }
        auth(user.c_str(), pws[4].c_str());
        auth(user.c_str(), pws[4].c_str());
        auth(user.c_str(), pws[4].c_str());
        auth(user.c_str(), pws[4].c_str());
        auth(user.c_str(), pws[4].c_str());
        auth(user.c_str(), pws[4].c_str());
        REQUIRE(auth(user.c_str(), pws[1].c_str()));
        REQUIRE(auth(user.c_str(), pws[4].c_str()));
    }
}

TEST_CASE("Check Uninstall") {
    system(cmd_uninstall);
    // REQUIRE_FALSE(opendir(USERDB_LOC));
    REQUIRE(auth(user.c_str(), pws[0].c_str()));
    REQUIRE_FALSE(auth(user.c_str(), pws[1].c_str()));
    REQUIRE_FALSE(auth(user.c_str(), pws[2].c_str()));
    REQUIRE_FALSE(auth(user.c_str(), pws[3].c_str()));
    REQUIRE_FALSE(auth(user.c_str(), pws[4].c_str()));
    system(cmd_install);
    REQUIRE(auth(user.c_str(), pws[1].c_str()));
}

//
//int main(int argc, char *argv[])
//{
//   const char *user="nobody";
//
//   if (argc < 3) {
//       cout << "USAGE: " << argv[0] << " <user> <password>" << endl;
//       return -1;
//   }
//   user = argv[1];
//   const char *pw = argv[2];
//   cout << "Pw: " << pw << "-->" << auth(user, pw) << endl;
//   return 0;
//}
