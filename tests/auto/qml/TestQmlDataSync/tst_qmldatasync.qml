import QtQuick 2.5
import de.skycoder42.QtDataSync 1.0
import QtTest 1.1

Item {
	id: root

	TestCase {
		name: "DataStore"

		DataStore {
			id: store
		}

		function test_valid() {
			verify(store.valid);
		}
	}

}
