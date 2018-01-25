%modules = (
	"QtDataSync" => "$basedir/src/datasync",
);

# Force generation of camel case headers for classes inside QtDataSync namespaces
$publicclassregexp = "QtDataSync::(?!__helpertypes|JsonObject).+";
