powershell -ExecutionPolicy Bypass -File .\configure.ps1 -TupitubeDir C:\devel\sources\tupitube.desk -DebugBuild
make -j4
powershell -ExecutionPolicy Bypass -File .\deploy.ps1 -TupitubeDir C:\devel\sources\tupitube.desk
"C:\Program Files (x86)\Inno Setup 6\iscc.exe" installer\tupitube_server.iss