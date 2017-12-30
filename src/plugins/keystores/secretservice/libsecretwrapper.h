#ifndef LIBSECRETWRAPPER_H
#define LIBSECRETWRAPPER_H

#include <QtCore/QByteArray>

//fwd declare
typedef struct _GError GError;
typedef struct _GHashTable  GHashTable;
typedef struct _SecretService SecretService;

class LibSecretException : public std::exception
{
public:
	LibSecretException(GError *error = nullptr);
	const char *what() const noexcept override;

private:
	const QByteArray _message;
};

class LibSecretWrapper
{
public:
	LibSecretWrapper(const QByteArray &appName);
	~LibSecretWrapper();

	static bool testAvailable();

	bool isOpen() const;
	void setup();
	void cleanup();

	QByteArray loadSecret(const QByteArray &key);
	void storeSecret(const QByteArray &key, const QByteArray &data);
	void removeSecret(const QByteArray &key);

private:
	QByteArray _appName;
	SecretService *_secret;

	GHashTable *attribs(const QByteArray &key);
};

#endif // LIBSECRETWRAPPER_H
