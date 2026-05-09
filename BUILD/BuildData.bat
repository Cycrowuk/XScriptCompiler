@Echo off
XScript.exe --builddata XML/x3fl.xml --out default_data.dat
XScript.exe --builddata XML/x3fl.xml --out Data/XScript_X3FL.dat
XScript.exe --load_data default_data.dat --exportudl
mkdir notepad++
mkdir notepad++\X3FL
move xscript.xml notepad++\X3FL\xscript.xml
move XScript_UDL.xml notepad++\X3FL\XScript_UDL.xml
del XScript_Compiler.zip
del ..\XScript_Compiler.zip
CScript zip.vbs . ..\XScript_Compiler.zip
move ..\XScript_Compiler.zip .
