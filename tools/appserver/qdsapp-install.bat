@echo off
sc create qdsapp binPath= "\"%{QT_INSTALL_BINS}\qdsappsvc.exe\" --backend windows" start= auto displayname= "QDSApp Service" || exit /B 1
sc description qdsapp "QtDataSync AppServer Service" || exit /B 1
