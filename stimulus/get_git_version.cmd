for /f "tokens=*" %%a in ('git rev-parse --short HEAD') do (
    echo #define REVISION "%%a" > Revision.h
)

