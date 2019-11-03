@REM make sure to run from a visual studio command line... Tested on visual studio 2019

cl /Zi /W4 /WX /wd4214 /wd4201 /wd4204 /DEBUG /D_CRT_SECURE_NO_WARNINGS /TC -c jzon.c
cl /Zi /W4 /WX /wd4214 /wd4201 /wd4204 /DEBUG /D_CRT_SECURE_NO_WARNINGS /TC -c test.c
link /WX /DEBUG:FULL /out:test.exe jzon.obj test.obj