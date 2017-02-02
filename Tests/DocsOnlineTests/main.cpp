#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "version.h"
#include "resource.h"

static void Fail(const char* msg)
{
    std::printf("FAIL %s\n", msg);
    std::exit(1);
}

int main()
{
    if (VER_MAJOR != 1)
        Fail("major");
    if (VER_MINOR != 3)
        Fail("minor");
    if (std::strcmp(STR_VER, "1.3.0.0") != 0)
        Fail("strver");
    if (IDD_DOCSONLINE_DIALOG != 102)
        Fail("dialog");
    if (ID_STARTSTOP != 110)
        Fail("startstop");
    if (ID_CONVERTDOCS != 113)
        Fail("convert");
    std::printf("OK DocsOnlineTests\n");
    return 0;
}
