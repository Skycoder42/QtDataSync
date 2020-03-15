%modules = (
	"QtDataSync" => "$basedir/src/datasync",
	"QtDataSyncCrypto" => "$basedir/src/datasynccrypto",
);

# Force generation of camel case headers for classes inside QtDataSync namespaces
$publicclassregexp = "QtDataSync::(?!__private::|Exception).+";
