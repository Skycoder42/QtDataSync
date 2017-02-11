#include "app.h"

#include <QFile>

App::App(int &argc, char **argv) :
	QtBackgroundProcess::App(argc, argv),
	connector(nullptr)
{}

void App::setupParser(QCommandLineParser &parser, bool useShortOptions)
{
	QtBackgroundProcess::App::setupParser(parser, useShortOptions);

	parser.addOption({
						 {"n", "name"},
						 "The <name> of the server to be displayed in HTTP Handshakes.",
						 "name",
						 QCoreApplication::applicationName()
					 });
	parser.addOption({
						 {"p", "port"},
						 "The <port> the server should use. By default, a random port is used.",
						 "port",
						 "0"
					 });
	parser.addOption({
						 {"a", "address"},
						 "The host <address> to listen on for incoming connections. Default is any.",
						 "address",
						 QHostAddress(QHostAddress::Any).toString()
					 });
	parser.addOption({
						 {"s", "wss"},
						 "Use secure websockets (TLS). <path> must be a valid p12 file containing the local certificate "
						 "and private key. If secured with a password, use the --pass option.",
						 "path"
					 });
	parser.addOption({
						 {"p", "pass"},
						 "The <passphrase> for the p12 file specified by the --wss option.",
						 "passphrase"
					 });
}

int App::startupApp(const QCommandLineParser &parser)
{
	auto wss = parser.isSet("wss");
	connector = new ClientConnector(parser.value("name"), wss, this);
	if(wss) {
		if(!connector->setupWss(parser.value("wss"), parser.value("pass")))
			return EXIT_FAILURE;
	}
	if(!connector->listen(QHostAddress(parser.value("address")), parser.value("port").toUShort()))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

bool App::requestAppShutdown(QtBackgroundProcess::Terminal *terminal, int &exitCode)
{
	return true;
}

int main(int argc, char *argv[])
{
	App a(argc, argv);
	return a.exec();
}
