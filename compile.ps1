1. powershell -ExecutionPolicy Bypass -File .\configure.ps1 -TupitubeDir C:\devel\sources\tupitube.desk -DebugBuild
2. make -j4
3. powershell -ExecutionPolicy Bypass -File .\deploy.ps1 -TupitubeDir C:\devel\sources\tupitube.desk
4. "C:\Program Files (x86)\Inno Setup 6\iscc.exe" installer\tupitube_server.iss