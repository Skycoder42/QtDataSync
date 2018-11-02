%modules = (
	"QtDataSync" => "$basedir/src/datasync;$basedir/src/messages",
	"QtDataSyncAndroid" => "$basedir/src/datasyncandroid",
	"QtDataSyncIos" => "$basedir/src/datasyncios",
);

# Force generation of camel case headers for classes inside QtDataSync namespaces
# Exception must be excluded, because it generates conflicts with the std::exception class (case-insensitive)
$publicclassregexp = "QtDataSync::(?!__helpertypes|JsonObject|Exception).+";
