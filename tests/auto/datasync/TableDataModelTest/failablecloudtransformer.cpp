#include "failablecloudtransformer.h"

FailableCloudTransformer::FailableCloudTransformer(QObject *parent) :
	PlainCloudTransformer{parent}
{}

QJsonObject FailableCloudTransformer::transformUploadSync(const QVariantHash &data) const
{
	if (shouldFail)
		throw FailableCloudTransformer::Exception{};
	else
		return PlainCloudTransformer::transformUploadSync(data);
}

QVariantHash FailableCloudTransformer::transformDownloadSync(const QJsonObject &data) const
{
	if (shouldFail)
		throw FailableCloudTransformer::Exception{};
	else
		return PlainCloudTransformer::transformDownloadSync(data);
}

QString FailableCloudTransformer::Exception::qWhat() const
{
	return QStringLiteral("FailableCloudTransformer.shouldFail was set to true");
}

void FailableCloudTransformer::Exception::raise() const
{
	throw *this;
}

QtDataSync::ExceptionBase *FailableCloudTransformer::Exception::clone() const
{
	return new FailableCloudTransformer::Exception{*this};
}
