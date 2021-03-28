REM NOTE (jacquesh): This is not part of the usual libxml2 distribution, but exists to track the configuration options used in this particular repo

pushd "%~dp0"
cscript configure.js legacy=no ftp=no http=no sax1=no schematron=no schemas=no c14n=no catalog=no docb=no xptr=no xinclude=no iconv=no xml_debug=no regexps=no modules=no reader=no writer=no walker=no pattern=no push=no valid=no output=yes
popd