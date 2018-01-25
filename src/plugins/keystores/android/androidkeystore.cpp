#include "androidkeystore.h"
#include <QtAndroidExtras/QtAndroid>

AndroidKeyStore::AndroidKeyStore(const QtDataSync::Defaults &defaults, QObject *parent) :
	KeyStore(defaults, parent),
	_preferences()
{}

QString AndroidKeyStore::providerName() const
{
	return QStringLiteral("android");
}

bool AndroidKeyStore::isOpen() const
{
	return _preferences.isValid();
}

void AndroidKeyStore::openStore()
{
	if(!_preferences.isValid()) {
		QAndroidJniEnvironment env;

		auto MODE_PRIVATE = QAndroidJniObject::getStaticField<jint>("android/content/Context",
																	"MODE_PRIVATE");
		jniThrow(env);

		auto context = QtAndroid::androidContext();
		jniThrow(env);
		_preferences = context.callObjectMethod("getSharedPreferences",
												"(Ljava/lang/String;I)Landroid/content/SharedPreferences;",
												QAndroidJniObject::fromString(QStringLiteral("QtDataSync-Keystore")).object<jstring>(),
												MODE_PRIVATE);
		jniThrow(env);
	}
}

void AndroidKeyStore::closeStore()
{
	_preferences = QAndroidJniObject();
}

bool AndroidKeyStore::contains(const QString &key) const
{
	QAndroidJniEnvironment env;
	auto res = _preferences.callMethod<jboolean>("contains",
												 "(Ljava/lang/String;)V",
												 QAndroidJniObject::fromString(key).object<jstring>());
	jniThrow(env);
	return static_cast<bool>(res);
}

void AndroidKeyStore::save(const QString &key, const QByteArray &pKey)
{
	auto baseData = QString::fromUtf8(pKey.toBase64());

	QAndroidJniEnvironment env;
	auto editor = prefEditor(env);
	editor.callObjectMethod("putString",
							"(Ljava/lang/String;Ljava/lang/String;)Landroid/content/SharedPreferences$Editor;",
							QAndroidJniObject::fromString(key).object<jstring>(),
							QAndroidJniObject::fromString(baseData).object<jstring>());
	jniThrow(env);
	prefApply(editor, env);
}

QByteArray AndroidKeyStore::load(const QString &key)
{
	QAndroidJniEnvironment env;
	auto jData = _preferences.callObjectMethod("getString",
											   "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
											   QAndroidJniObject::fromString(key).object<jstring>(),
											   NULL);
	jniThrow(env);
	if(jData.isValid())
		return QByteArray::fromBase64(jData.toString().toUtf8());
	else
		throw QtDataSync::KeyStoreException(this, QStringLiteral("Key does not exist"));
}

void AndroidKeyStore::remove(const QString &key)
{
	QAndroidJniEnvironment env;
	auto editor = prefEditor(env);
	editor.callObjectMethod("remove",
							"(Ljava/lang/String;)Landroid/content/SharedPreferences$Editor;",
							QAndroidJniObject::fromString(key).object<jstring>());
	jniThrow(env);
	prefApply(editor, env);
}

void AndroidKeyStore::jniThrow(QAndroidJniEnvironment &env) const
{
	if(env->ExceptionCheck()) {
		auto exception = QAndroidJniObject::fromLocalRef(env->ExceptionOccurred());
		auto message = QStringLiteral("Java-Exception occured:\n\tMessage: ") +
					   exception.callObjectMethod("getMessage", "()Ljava/lang/String;").toString();

#ifndef Q_NO_DEBUG
		QAndroidJniObject stringWriter("java/io/StringWriter");
		QAndroidJniObject printWriter("java/io/PrintWriter", "(Ljava/lang/Writer;)V", stringWriter.object());
		exception.callMethod<void>("printStackTrace", "(Ljava/lang/PrintWriter;)V", printWriter.object());
		message += QStringLiteral("\n\tStack-Trace:\n") +
				   stringWriter.callObjectMethod("toString", "()Ljava/lang/String;").toString();
#endif

		env->ExceptionClear();
		throw QtDataSync::KeyStoreException(this, message);
	}
}

QAndroidJniObject AndroidKeyStore::prefEditor(QAndroidJniEnvironment &env)
{
	auto editor = _preferences.callObjectMethod("edit",
												"()Landroid/content/SharedPreferences$Editor;");
	jniThrow(env);
	return editor;
}

void AndroidKeyStore::prefApply(QAndroidJniObject &editor, QAndroidJniEnvironment &env)
{
	editor.callMethod<void>("apply");
	jniThrow(env);
}
