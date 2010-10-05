#include "MDBShared.h"

#include <Security/Security.h>
#include <Security/Authorization.h>

void log2(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    fprintf(stdout, "\n");
    va_end(ap);
}

void error(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    fprintf(stdout, "\n");
    va_end(ap);
}

char *
appendString(char *buffer, const char *format, ...) {
    if (buffer != NULL && format) {
        va_list args;
        va_start(args, format);
        vsprintf(buffer + strlen(buffer), format, args);
        va_end(args);
    }
    return buffer;
}

char *
allocateFormattedString(const char *format, ...) {
    char *buffer = (char *)malloc(1024);
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return buffer;
}

/**
 * Acquiring debug privileges on Snow Leopard is a bit of a pain.
 *
 * You have to follow the following steps:
 *
 * 1. Create a code signing certificate in your login keychain.
 * 2. Find your .plist file in XCode and add the following entry:
 *    "SecTaskAccess" and set it to "allowed". Make sure the Info.plist file
 *    generated in your bundle actually has this value in it.
 * 3. Make XCode sign the executable during the build process, or sign
 *    it yourself manually using the codesign command line utility.
 * 4. Call the following method before making calls to task_for_pid().
 */

int acquireTaskportRight() {
    OSStatus status;
    AuthorizationItem taskporItem[] = {{"system.privilege.taskport"}};
    AuthorizationRights rights = {1, taskporItem}, *outRights = NULL;
    AuthorizationRef author;
    AuthorizationFlags authorizationFlags = kAuthorizationFlagExtendRights
										  | kAuthorizationFlagPreAuthorize
										  | kAuthorizationFlagInteractionAllowed
										  | (1 << 5);

    status = AuthorizationCreate(NULL,
                                 kAuthorizationEmptyEnvironment,
                                 authorizationFlags,
                                 &author);

    if (status != errAuthorizationSuccess) {
        return 1;
    }

    status = AuthorizationCopyRights(author,
                                     &rights,
                                     kAuthorizationEmptyEnvironment,
                                     authorizationFlags,
                                     &outRights);

    if (status == errAuthorizationSuccess) {
        return 0;
    } else if (status == errAuthorizationInteractionNotAllowed) {
    	return 0;
    }
    return 1;
}
