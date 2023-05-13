IF NOT DEFINED IS_ENV_READY (
    SET IS_ENV_READY=1
    CALL "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvars64.bat"
)
msbuild "WolfDec.sln" /p:Configuration="Release" /p:Platform="x64"