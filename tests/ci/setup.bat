powershell -Command "Invoke-WebRequest https://github.com/docker/compose/releases/download/1.12.0/docker-compose-Windows-x86_64.exe -UseBasicParsing -OutFile $Env:ProgramFiles\docker\docker-compose.exe"

docker -v
docker-compose -v

docker-compose -f .\tools\qdatasyncserver\docker-compose.yaml up -d
