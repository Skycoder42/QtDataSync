#include "libsecretwrapper.h"

#include <libsecret/secret.h>

const SecretSchema *schema()
{
	static const SecretSchema _schema = {
		"de.skycoder42.qtdatasync.keystore", SECRET_SCHEMA_NONE,
		{
			{  "app", SECRET_SCHEMA_ATTRIBUTE_STRING },
			{  "key", SECRET_SCHEMA_ATTRIBUTE_STRING },
			{  NULL, SECRET_SCHEMA_ATTRIBUTE_STRING },
		}
	};
	return &_schema;
}

LibSecretWrapper::LibSecretWrapper(const QByteArray &appName) :
	_appName(appName),
	_secret(nullptr)
{}

LibSecretWrapper::~LibSecretWrapper()
{
	cleanup();
}

bool LibSecretWrapper::testAvailable()
{
	GError *error = nullptr;
	auto secret = secret_service_get_sync(SECRET_SERVICE_OPEN_SESSION,
										  nullptr,
										  &error);
	auto ok = true;
	if(error) {
		ok = false;
		g_error_free(error);
	}
	if(secret)
		g_object_unref(secret);
	else
		ok = false;
	return ok;
}

bool LibSecretWrapper::isOpen() const
{
	return _secret;
}

void LibSecretWrapper::setup()
{
	GError *error = nullptr;
	_secret = secret_service_get_sync(SECRET_SERVICE_OPEN_SESSION,
									  nullptr,
									  &error);
	if(error) {
		cleanup();
		throw LibSecretException(error);
	}
	if(!_secret)
		throw LibSecretException();
}

void LibSecretWrapper::cleanup()
{
	if(_secret) {
		g_object_unref(_secret);
		_secret = nullptr;
	}
}

QByteArray LibSecretWrapper::loadSecret(const QByteArray &key)
{
	auto att = attribs(key);
	GError *error = nullptr;
	auto value = secret_service_lookup_sync(_secret,
											schema(),
											att,
											nullptr,
											&error);

	g_hash_table_destroy(att);
	if(error) {
		if(value)
			secret_value_unref(value);
		throw LibSecretException(error);
	}

	if(value) {
		auto res = QByteArray::fromBase64(secret_value_get_text(value));
		secret_value_unref(value);
		return res;
	} else
		return QByteArray();
}

void LibSecretWrapper::storeSecret(const QByteArray &key, const QByteArray &data)
{
	auto att = attribs(key);

	auto baseData = data.toBase64();
	auto value = secret_value_new(baseData.data(),
								  baseData.size(),
								  "text/plain");

	GError *error = nullptr;
	auto ok = secret_service_store_sync(_secret,
										schema(),
										att,
										nullptr,
										key.constData(),
										value,
										nullptr,
										&error);

	g_hash_table_destroy(att);
	secret_value_unref(value);

	if(error)
		throw LibSecretException(error);
	if(!ok)
		throw LibSecretException();
}

void LibSecretWrapper::removeSecret(const QByteArray &key)
{
	auto att = attribs(key);
	GError *error = nullptr;
	auto ok = secret_service_clear_sync(_secret,
										schema(),
										att,
										nullptr,
										&error);

	g_hash_table_destroy(att);

	if(error)
		throw LibSecretException(error);
	if(!ok)
		throw LibSecretException();
}

GHashTable *LibSecretWrapper::attribs(const QByteArray &key)
{
	auto attribs = g_hash_table_new(g_str_hash, g_str_equal);
	g_hash_table_insert(attribs, (gpointer)"app", (gpointer)_appName.constData());
	g_hash_table_insert(attribs, (gpointer)"key", (gpointer)key.constData());
	return attribs;
}



LibSecretException::LibSecretException(GError *error) :
	_message(error ? error->message : "unkown error")
{
	if(error)
		g_error_free(error);
}

const char *LibSecretException::what() const noexcept
{
	return _message.constData();
}
